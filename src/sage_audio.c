#include "sage/sage_audio.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SAGE_AUDIO_DEFAULT_RATE 48000u
#define SAGE_AUDIO_DEFAULT_CLIPS 128u
#define SAGE_AUDIO_DEFAULT_VOICES 64u
#define SAGE_AUDIO_DEFAULT_STEMS 16u
#define SAGE_AUDIO_DEFAULT_EVENTS 64u
#define SAGE_AUDIO_DEFAULT_PARAMETERS 32u
#define SAGE_AUDIO_DEFAULT_BUSES 8u
#define SAGE_AUDIO_DEFAULT_CONTACTS 64u
#define SAGE_AUDIO_MAX_CLIPS 65535u
#define SAGE_AUDIO_MAX_VOICES 4096u
#define SAGE_AUDIO_MAX_STEMS 1024u
#define SAGE_AUDIO_MAX_EVENTS 4096u
#define SAGE_AUDIO_MAX_PARAMETERS 1024u
#define SAGE_AUDIO_MAX_BUSES 256u
#define SAGE_AUDIO_MAX_CONTACTS 4096u

typedef struct SageAudioClipSlot {
    const float* samples;
    uint32_t frame_count;
    uint32_t channels;
    int occupied;
} SageAudioClipSlot;

typedef struct SageAudioRamp {
    float value;
    float target;
    float step;
    uint64_t frames_left;
} SageAudioRamp;

typedef struct SageAudioBusSlot {
    char name[SAGE_AUDIO_NAME_CAPACITY];
    SageAudioRamp gain;
    int occupied;
} SageAudioBusSlot;

typedef struct SageAudioParameterSlot {
    char name[SAGE_AUDIO_NAME_CAPACITY];
    SageAudioRamp value;
    int occupied;
} SageAudioParameterSlot;

typedef struct SageAudioStemSlot {
    char name[SAGE_AUDIO_NAME_CAPACITY];
    SageAudioClip clip;
    SageAudioBus bus;
    SageAudioParameter parameter;
    float parameter_minimum;
    float parameter_maximum;
    SageAudioRamp gain;
    int occupied;
} SageAudioStemSlot;

typedef struct SageAudioEventSlot {
    char name[SAGE_AUDIO_NAME_CAPACITY];
    SageAudioClip clip;
    SageAudioBus bus;
    float minimum_intensity;
    float full_gain_intensity;
    float gain;
    float pitch_at_minimum;
    float pitch_at_full_gain;
    uint32_t max_instances;
    int occupied;
} SageAudioEventSlot;

typedef struct SageAudioContactSlot {
    SageAudioContactProfile profile;
    uint64_t last_emitted_frame;
    int has_emitted;
    int occupied;
} SageAudioContactSlot;

typedef struct SageAudioVoice {
    SageAudioClip clip;
    SageAudioEvent event;
    SageAudioBus bus;
    double position;
    float gain;
    float pitch;
    int active;
} SageAudioVoice;

struct SageAudioMixer {
    uint32_t sample_rate;
    uint32_t max_clips;
    uint32_t clip_count;
    uint32_t max_voices;
    uint32_t max_stems;
    uint32_t stem_count;
    uint32_t max_events;
    uint32_t event_count;
    uint32_t max_parameters;
    uint32_t parameter_count;
    uint32_t max_buses;
    uint32_t bus_count;
    uint32_t max_contact_profiles;
    uint32_t contact_count;
    SageAudioClipSlot* clips;
    SageAudioVoice* voices;
    SageAudioStemSlot* stems;
    SageAudioEventSlot* events;
    SageAudioParameterSlot* parameters;
    SageAudioBusSlot* buses;
    SageAudioContactSlot* contacts;
    uint64_t transport_frame;
    uint64_t rendered_frames;
    int transport_playing;
};

static int valid_name(const char* name) {
    size_t length;
    if (!name || !name[0]) return 0;
    length = strlen(name);
    return length < SAGE_AUDIO_NAME_CAPACITY;
}

static int valid_gain(float gain) {
    return isfinite(gain) && gain >= 0.0f && gain <= 4.0f;
}

static int valid_duration(float seconds) {
    return isfinite(seconds) && seconds >= 0.0f && seconds <= 3600.0f;
}

static void ramp_init(SageAudioRamp* ramp, float value) {
    ramp->value = value;
    ramp->target = value;
    ramp->step = 0.0f;
    ramp->frames_left = 0;
}

static void ramp_set(SageAudioMixer* mixer,
                     SageAudioRamp* ramp,
                     float target,
                     float seconds) {
    uint64_t frames = (uint64_t)ceil((double)seconds * mixer->sample_rate);
    ramp->target = target;
    if (frames == 0) {
        ramp_init(ramp, target);
        return;
    }
    ramp->frames_left = frames;
    ramp->step = (target - ramp->value) / (float)frames;
}

static float ramp_advance(SageAudioRamp* ramp) {
    if (ramp->frames_left) {
        ramp->value += ramp->step;
        ramp->frames_left--;
        if (!ramp->frames_left) {
            ramp->value = ramp->target;
            ramp->step = 0.0f;
        }
    }
    return ramp->value;
}

static float clamp_unit(float value) {
    if (!isfinite(value) || value <= 0.0f) return 0.0f;
    if (value >= 1.0f) return 1.0f;
    return value;
}

static int valid_clip_id(const SageAudioMixer* mixer, SageAudioClip clip) {
    return mixer && clip > 0 && clip <= mixer->clip_count &&
           mixer->clips[clip].occupied;
}

static int valid_bus_id(const SageAudioMixer* mixer, SageAudioBus bus) {
    return mixer && bus <= mixer->max_buses && mixer->buses[bus].occupied;
}

static int valid_parameter_id(const SageAudioMixer* mixer,
                              SageAudioParameter parameter) {
    return mixer && parameter > 0 && parameter <= mixer->parameter_count &&
           mixer->parameters[parameter].occupied;
}

static int valid_stem_id(const SageAudioMixer* mixer, SageAudioStem stem) {
    return mixer && stem > 0 && stem <= mixer->stem_count &&
           mixer->stems[stem].occupied;
}

static int valid_event_id(const SageAudioMixer* mixer, SageAudioEvent event) {
    return mixer && event > 0 && event <= mixer->event_count &&
           mixer->events[event].occupied;
}

static float bus_gain(const SageAudioMixer* mixer, SageAudioBus bus) {
    if (bus == 0) return mixer->buses[0].gain.value;
    return mixer->buses[bus].gain.value * mixer->buses[0].gain.value;
}

static void free_mixer(SageAudioMixer* mixer) {
    if (!mixer) return;
    free(mixer->clips);
    free(mixer->voices);
    free(mixer->stems);
    free(mixer->events);
    free(mixer->parameters);
    free(mixer->buses);
    free(mixer->contacts);
    free(mixer);
}

SageAudioConfig sage_audio_config_default(void) {
    SageAudioConfig config;
    config.sample_rate_hz = SAGE_AUDIO_DEFAULT_RATE;
    config.max_clips = SAGE_AUDIO_DEFAULT_CLIPS;
    config.max_voices = SAGE_AUDIO_DEFAULT_VOICES;
    config.max_stems = SAGE_AUDIO_DEFAULT_STEMS;
    config.max_events = SAGE_AUDIO_DEFAULT_EVENTS;
    config.max_parameters = SAGE_AUDIO_DEFAULT_PARAMETERS;
    config.max_buses = SAGE_AUDIO_DEFAULT_BUSES;
    config.max_contact_profiles = SAGE_AUDIO_DEFAULT_CONTACTS;
    return config;
}

SageAudioMixer* sage_audio_mixer_create(const SageAudioConfig* input_config) {
    SageAudioConfig config = input_config ? *input_config : sage_audio_config_default();
    SageAudioMixer* mixer;
    if (!config.sample_rate_hz) config.sample_rate_hz = SAGE_AUDIO_DEFAULT_RATE;
    if (!config.max_clips) config.max_clips = SAGE_AUDIO_DEFAULT_CLIPS;
    if (!config.max_voices) config.max_voices = SAGE_AUDIO_DEFAULT_VOICES;
    if (!config.max_stems) config.max_stems = SAGE_AUDIO_DEFAULT_STEMS;
    if (!config.max_events) config.max_events = SAGE_AUDIO_DEFAULT_EVENTS;
    if (!config.max_parameters) config.max_parameters = SAGE_AUDIO_DEFAULT_PARAMETERS;
    if (!config.max_buses) config.max_buses = SAGE_AUDIO_DEFAULT_BUSES;
    if (!config.max_contact_profiles)
        config.max_contact_profiles = SAGE_AUDIO_DEFAULT_CONTACTS;
    if (config.sample_rate_hz < 8000u || config.sample_rate_hz > 192000u ||
        config.max_clips > SAGE_AUDIO_MAX_CLIPS ||
        config.max_voices > SAGE_AUDIO_MAX_VOICES ||
        config.max_stems > SAGE_AUDIO_MAX_STEMS ||
        config.max_events > SAGE_AUDIO_MAX_EVENTS ||
        config.max_parameters > SAGE_AUDIO_MAX_PARAMETERS ||
        config.max_buses > SAGE_AUDIO_MAX_BUSES ||
        config.max_contact_profiles > SAGE_AUDIO_MAX_CONTACTS) return NULL;

    mixer = (SageAudioMixer*)calloc(1, sizeof(*mixer));
    if (!mixer) return NULL;
    mixer->sample_rate = config.sample_rate_hz;
    mixer->max_clips = config.max_clips;
    mixer->max_voices = config.max_voices;
    mixer->max_stems = config.max_stems;
    mixer->max_events = config.max_events;
    mixer->max_parameters = config.max_parameters;
    mixer->max_buses = config.max_buses;
    mixer->max_contact_profiles = config.max_contact_profiles;
    mixer->clips = (SageAudioClipSlot*)calloc((size_t)config.max_clips + 1u,
                                               sizeof(*mixer->clips));
    mixer->voices = (SageAudioVoice*)calloc(config.max_voices, sizeof(*mixer->voices));
    mixer->stems = (SageAudioStemSlot*)calloc((size_t)config.max_stems + 1u,
                                               sizeof(*mixer->stems));
    mixer->events = (SageAudioEventSlot*)calloc((size_t)config.max_events + 1u,
                                                 sizeof(*mixer->events));
    mixer->parameters = (SageAudioParameterSlot*)calloc(
        (size_t)config.max_parameters + 1u, sizeof(*mixer->parameters));
    mixer->buses = (SageAudioBusSlot*)calloc((size_t)config.max_buses + 1u,
                                              sizeof(*mixer->buses));
    mixer->contacts = (SageAudioContactSlot*)calloc(
        config.max_contact_profiles, sizeof(*mixer->contacts));
    if (!mixer->clips || !mixer->voices || !mixer->stems || !mixer->events ||
        !mixer->parameters || !mixer->buses || !mixer->contacts) {
        free_mixer(mixer);
        return NULL;
    }
    mixer->buses[0].occupied = 1;
    ramp_init(&mixer->buses[0].gain, 1.0f);
    mixer->bus_count = 1;
    return mixer;
}

void sage_audio_mixer_destroy(SageAudioMixer* mixer) {
    free_mixer(mixer);
}

uint32_t sage_audio_sample_rate(const SageAudioMixer* mixer) {
    return mixer ? mixer->sample_rate : 0;
}

SageResult sage_audio_clip_register(SageAudioMixer* mixer,
                                    const float* samples,
                                    uint32_t frame_count,
                                    uint32_t channels,
                                    SageAudioClip* out_clip) {
    size_t sample_count;
    size_t i;
    uint32_t id;
    if (!mixer || !samples || !frame_count || !out_clip ||
        (channels != 1u && channels != 2u)) return SAGE_ERR_ARGUMENT;
    if ((size_t)frame_count > ((size_t)-1) / channels) return SAGE_ERR_LIMIT;
    sample_count = (size_t)frame_count * channels;
    for (i = 0; i < sample_count; ++i) {
        if (!isfinite(samples[i]) || samples[i] < -1.0f || samples[i] > 1.0f)
            return SAGE_ERR_ARGUMENT;
    }
    if (mixer->clip_count >= mixer->max_clips) return SAGE_ERR_LIMIT;
    id = mixer->clip_count + 1u;
    mixer->clips[id].samples = samples;
    mixer->clips[id].frame_count = frame_count;
    mixer->clips[id].channels = channels;
    mixer->clips[id].occupied = 1;
    mixer->clip_count = id;
    *out_clip = id;
    return SAGE_OK;
}

SageResult sage_audio_bus_create(SageAudioMixer* mixer,
                                 const char* name,
                                 SageAudioBus* out_bus) {
    uint32_t id;
    uint32_t i;
    if (!mixer || !valid_name(name) || !out_bus) return SAGE_ERR_ARGUMENT;
    for (i = 1; i <= mixer->max_buses; ++i) {
        if (mixer->buses[i].occupied && strcmp(mixer->buses[i].name, name) == 0)
            return SAGE_ERR_ARGUMENT;
    }
    if (mixer->bus_count > mixer->max_buses) return SAGE_ERR_LIMIT;
    id = mixer->bus_count;
    mixer->bus_count++;
    (void)strcpy(mixer->buses[id].name, name);
    mixer->buses[id].occupied = 1;
    ramp_init(&mixer->buses[id].gain, 1.0f);
    *out_bus = id;
    return SAGE_OK;
}

SageResult sage_audio_bus_fade(SageAudioMixer* mixer,
                               SageAudioBus bus,
                               float gain,
                               float duration_seconds) {
    if (!valid_bus_id(mixer, bus) || !valid_gain(gain) ||
        !valid_duration(duration_seconds)) return SAGE_ERR_ARGUMENT;
    ramp_set(mixer, &mixer->buses[bus].gain, gain, duration_seconds);
    return SAGE_OK;
}

SageResult sage_audio_parameter_create(SageAudioMixer* mixer,
                                       const char* name,
                                       float initial_value,
                                       SageAudioParameter* out_parameter) {
    uint32_t id;
    uint32_t i;
    if (!mixer || !valid_name(name) || !isfinite(initial_value) || !out_parameter)
        return SAGE_ERR_ARGUMENT;
    for (i = 1; i <= mixer->parameter_count; ++i) {
        if (strcmp(mixer->parameters[i].name, name) == 0) return SAGE_ERR_ARGUMENT;
    }
    if (mixer->parameter_count >= mixer->max_parameters) return SAGE_ERR_LIMIT;
    id = mixer->parameter_count + 1u;
    (void)strcpy(mixer->parameters[id].name, name);
    mixer->parameters[id].occupied = 1;
    ramp_init(&mixer->parameters[id].value, initial_value);
    mixer->parameter_count = id;
    *out_parameter = id;
    return SAGE_OK;
}

SageResult sage_audio_parameter_fade(SageAudioMixer* mixer,
                                     SageAudioParameter parameter,
                                     float value,
                                     float duration_seconds) {
    if (!valid_parameter_id(mixer, parameter) || !isfinite(value) ||
        !valid_duration(duration_seconds)) return SAGE_ERR_ARGUMENT;
    ramp_set(mixer, &mixer->parameters[parameter].value, value, duration_seconds);
    return SAGE_OK;
}

SageResult sage_audio_stem_create(SageAudioMixer* mixer,
                                  const char* name,
                                  SageAudioClip clip,
                                  SageAudioBus bus,
                                  SageAudioStem* out_stem) {
    uint32_t id;
    uint32_t i;
    if (!mixer || !valid_name(name) || !valid_clip_id(mixer, clip) ||
        !valid_bus_id(mixer, bus) || !out_stem) return SAGE_ERR_ARGUMENT;
    for (i = 1; i <= mixer->stem_count; ++i) {
        if (strcmp(mixer->stems[i].name, name) == 0) return SAGE_ERR_ARGUMENT;
    }
    if (mixer->stem_count >= mixer->max_stems) return SAGE_ERR_LIMIT;
    id = mixer->stem_count + 1u;
    (void)strcpy(mixer->stems[id].name, name);
    mixer->stems[id].clip = clip;
    mixer->stems[id].bus = bus;
    mixer->stems[id].occupied = 1;
    ramp_init(&mixer->stems[id].gain, 0.0f);
    mixer->stem_count = id;
    *out_stem = id;
    return SAGE_OK;
}

SageResult sage_audio_stem_bind_parameter(SageAudioMixer* mixer,
                                          SageAudioStem stem,
                                          SageAudioParameter parameter,
                                          float minimum,
                                          float maximum) {
    if (!valid_stem_id(mixer, stem) || parameter > mixer->parameter_count ||
        (parameter && !valid_parameter_id(mixer, parameter))) return SAGE_ERR_ARGUMENT;
    if (parameter && (!isfinite(minimum) || !isfinite(maximum) || minimum >= maximum))
        return SAGE_ERR_ARGUMENT;
    mixer->stems[stem].parameter = parameter;
    mixer->stems[stem].parameter_minimum = minimum;
    mixer->stems[stem].parameter_maximum = maximum;
    return SAGE_OK;
}

SageResult sage_audio_stem_fade(SageAudioMixer* mixer,
                                SageAudioStem stem,
                                float gain,
                                float duration_seconds) {
    if (!valid_stem_id(mixer, stem) || !valid_gain(gain) ||
        !valid_duration(duration_seconds)) return SAGE_ERR_ARGUMENT;
    ramp_set(mixer, &mixer->stems[stem].gain, gain, duration_seconds);
    return SAGE_OK;
}

SageResult sage_audio_music_start(SageAudioMixer* mixer, int restart_transport) {
    if (!mixer || (restart_transport != 0 && restart_transport != 1))
        return SAGE_ERR_ARGUMENT;
    if (restart_transport) mixer->transport_frame = 0;
    mixer->transport_playing = 1;
    return SAGE_OK;
}

SageResult sage_audio_music_pause(SageAudioMixer* mixer) {
    if (!mixer) return SAGE_ERR_ARGUMENT;
    mixer->transport_playing = 0;
    return SAGE_OK;
}

SageResult sage_audio_event_create(SageAudioMixer* mixer,
                                   const char* name,
                                   SageAudioClip clip,
                                   SageAudioBus bus,
                                   float minimum_intensity,
                                   float full_gain_intensity,
                                   float gain,
                                   float pitch_at_minimum,
                                   float pitch_at_full_gain,
                                   uint32_t max_instances,
                                   SageAudioEvent* out_event) {
    uint32_t id;
    uint32_t i;
    if (!mixer || !valid_name(name) || !valid_clip_id(mixer, clip) ||
        !valid_bus_id(mixer, bus) || !out_event ||
        !isfinite(minimum_intensity) || !isfinite(full_gain_intensity) ||
        minimum_intensity < 0.0f || full_gain_intensity <= minimum_intensity ||
        !valid_gain(gain) || !isfinite(pitch_at_minimum) ||
        !isfinite(pitch_at_full_gain) || pitch_at_minimum <= 0.0f ||
        pitch_at_full_gain <= 0.0f || pitch_at_minimum > 8.0f ||
        pitch_at_full_gain > 8.0f) return SAGE_ERR_ARGUMENT;
    for (i = 1; i <= mixer->event_count; ++i) {
        if (strcmp(mixer->events[i].name, name) == 0) return SAGE_ERR_ARGUMENT;
    }
    if (mixer->event_count >= mixer->max_events) return SAGE_ERR_LIMIT;
    id = mixer->event_count + 1u;
    (void)strcpy(mixer->events[id].name, name);
    mixer->events[id].clip = clip;
    mixer->events[id].bus = bus;
    mixer->events[id].minimum_intensity = minimum_intensity;
    mixer->events[id].full_gain_intensity = full_gain_intensity;
    mixer->events[id].gain = gain;
    mixer->events[id].pitch_at_minimum = pitch_at_minimum;
    mixer->events[id].pitch_at_full_gain = pitch_at_full_gain;
    mixer->events[id].max_instances = max_instances;
    mixer->events[id].occupied = 1;
    mixer->event_count = id;
    *out_event = id;
    return SAGE_OK;
}

static SageResult event_emit_scaled(SageAudioMixer* mixer,
                                    SageAudioEvent event,
                                    float intensity,
                                    float gain_scale,
                                    float pitch_scale) {
    SageAudioEventSlot* descriptor;
    SageAudioVoice* voice = NULL;
    uint32_t i;
    uint32_t matching_voices = 0;
    float normalized;
    if (!valid_event_id(mixer, event) || !isfinite(intensity) || intensity < 0.0f ||
        !valid_gain(gain_scale) || !isfinite(pitch_scale) ||
        pitch_scale <= 0.0f || pitch_scale > 8.0f)
        return SAGE_ERR_ARGUMENT;
    descriptor = &mixer->events[event];
    if (intensity <= descriptor->minimum_intensity) return SAGE_OK;
    normalized = clamp_unit((intensity - descriptor->minimum_intensity) /
                            (descriptor->full_gain_intensity -
                             descriptor->minimum_intensity));
    for (i = 0; i < mixer->max_voices; ++i) {
        if (!mixer->voices[i].active) {
            if (!voice) voice = &mixer->voices[i];
        } else if (mixer->voices[i].event == event) {
            matching_voices++;
        }
    }
    if (descriptor->max_instances && matching_voices >= descriptor->max_instances)
        return SAGE_OK;
    if (!voice) return SAGE_ERR_LIMIT;
    voice->clip = descriptor->clip;
    voice->event = event;
    voice->bus = descriptor->bus;
    voice->position = 0.0;
    voice->gain = descriptor->gain * normalized * gain_scale;
    voice->pitch = (descriptor->pitch_at_minimum +
                    (descriptor->pitch_at_full_gain - descriptor->pitch_at_minimum) *
                        normalized) * pitch_scale;
    voice->active = 1;
    return SAGE_OK;
}

SageResult sage_audio_event_emit(SageAudioMixer* mixer,
                                 SageAudioEvent event,
                                 float intensity) {
    return event_emit_scaled(mixer, event, intensity, 1.0f, 1.0f);
}

SageResult sage_audio_trigger_emit(SageAudioMixer* mixer,
                                   const SageAudioTrigger* trigger,
                                   float intensity) {
    if (!mixer || !trigger || !valid_gain(trigger->gain) ||
        !isfinite(trigger->pitch) || trigger->pitch <= 0.0f || trigger->pitch > 8.0f)
        return SAGE_ERR_ARGUMENT;
    return event_emit_scaled(mixer, (SageAudioEvent)trigger->event_id,
                             intensity, trigger->gain, trigger->pitch);
}

SageResult sage_audio_entity_trigger_emit(SageAudioMixer* mixer,
                                          const SageWorld* world,
                                          SageEntity entity,
                                          float intensity) {
    const SageAudioTrigger* trigger;
    if (!mixer || !world) return SAGE_ERR_ARGUMENT;
    if (!sage_ecs_entity_alive(world, entity)) return SAGE_ERR_INVALID_ENTITY;
    trigger = sage_ecs_audio_trigger_get(world, entity);
    if (!trigger) return SAGE_ERR_NOT_FOUND;
    return sage_audio_trigger_emit(mixer, trigger, intensity);
}

SageResult sage_audio_contact_profile_set(
    SageAudioMixer* mixer,
    const SageAudioContactProfile* profile) {
    uint32_t i;
    uint32_t free_slot = UINT32_MAX;
    if (!mixer || !profile || !valid_event_id(mixer, profile->event) ||
        (profile->signal != SAGE_AUDIO_CONTACT_RELATIVE_SPEED &&
         profile->signal != SAGE_AUDIO_CONTACT_IMPULSE) ||
        !isfinite(profile->minimum_signal) || !isfinite(profile->full_gain_signal) ||
        profile->minimum_signal < 0.0f ||
        profile->full_gain_signal <= profile->minimum_signal ||
        !valid_duration(profile->cooldown_seconds)) return SAGE_ERR_ARGUMENT;
    for (i = 0; i < mixer->contact_count; ++i) {
        const SageAudioContactProfile* current = &mixer->contacts[i].profile;
        if ((current->material_tag_a == profile->material_tag_a &&
             current->material_tag_b == profile->material_tag_b) ||
            (current->material_tag_a == profile->material_tag_b &&
             current->material_tag_b == profile->material_tag_a)) {
            mixer->contacts[i].profile = *profile;
            mixer->contacts[i].has_emitted = 0;
            return SAGE_OK;
        }
    }
    if (mixer->contact_count >= mixer->max_contact_profiles) return SAGE_ERR_LIMIT;
    free_slot = mixer->contact_count++;
    mixer->contacts[free_slot].profile = *profile;
    mixer->contacts[free_slot].occupied = 1;
    return SAGE_OK;
}

SageResult sage_audio_contact_consume(SageAudioMixer* mixer,
                                      const SageContactEvent* contact) {
    uint32_t i;
    if (!mixer || !contact) return SAGE_ERR_ARGUMENT;
    for (i = 0; i < mixer->contact_count; ++i) {
        SageAudioContactSlot* slot = &mixer->contacts[i];
        const SageAudioContactProfile* profile = &slot->profile;
        float intensity;
        uint64_t cooldown_frames;
        if (!slot->occupied) continue;
        if (!((profile->material_tag_a == contact->material_tag_a &&
               profile->material_tag_b == contact->material_tag_b) ||
              (profile->material_tag_a == contact->material_tag_b &&
               profile->material_tag_b == contact->material_tag_a))) continue;
        intensity = profile->signal == SAGE_AUDIO_CONTACT_IMPULSE
                        ? contact->applied_impulse_ns
                        : contact->relative_speed_mps;
        if (!isfinite(intensity) || intensity < 0.0f) return SAGE_ERR_ARGUMENT;
        if (intensity <= profile->minimum_signal) return SAGE_OK;
        if (intensity > profile->full_gain_signal)
            intensity = profile->full_gain_signal;
        cooldown_frames = (uint64_t)ceil((double)profile->cooldown_seconds *
                                         mixer->sample_rate);
        if (slot->has_emitted &&
            mixer->rendered_frames - slot->last_emitted_frame < cooldown_frames)
            return SAGE_OK;
        {
            const SageResult result = sage_audio_event_emit(mixer, profile->event,
                                                             intensity);
            if (result == SAGE_OK) {
                slot->last_emitted_frame = mixer->rendered_frames;
                slot->has_emitted = 1;
            }
            return result;
        }
    }
    return SAGE_ERR_NOT_FOUND;
}

static void advance_ramps(SageAudioMixer* mixer) {
    uint32_t i;
    for (i = 0; i <= mixer->max_buses; ++i)
        if (mixer->buses[i].occupied) (void)ramp_advance(&mixer->buses[i].gain);
    for (i = 1; i <= mixer->parameter_count; ++i)
        (void)ramp_advance(&mixer->parameters[i].value);
    for (i = 1; i <= mixer->stem_count; ++i)
        (void)ramp_advance(&mixer->stems[i].gain);
}

static float parameter_multiplier(const SageAudioMixer* mixer,
                                  const SageAudioStemSlot* stem) {
    const float value = mixer->parameters[stem->parameter].value.value;
    return clamp_unit((value - stem->parameter_minimum) /
                      (stem->parameter_maximum - stem->parameter_minimum));
}

static void read_clip_frame(const SageAudioClipSlot* clip,
                            double position,
                            int loop,
                            float* left,
                            float* right) {
    uint32_t index_a = (uint32_t)position;
    uint32_t index_b = index_a + 1u;
    float fraction = (float)(position - (double)index_a);
    float left_a;
    float left_b;
    float right_a;
    float right_b;
    if (loop) index_b %= clip->frame_count;
    else if (index_b >= clip->frame_count) index_b = clip->frame_count - 1u;
    if (clip->channels == 1u) {
        left_a = clip->samples[index_a];
        left_b = clip->samples[index_b];
        right_a = left_a;
        right_b = left_b;
    } else {
        left_a = clip->samples[(size_t)index_a * 2u];
        left_b = clip->samples[(size_t)index_b * 2u];
        right_a = clip->samples[(size_t)index_a * 2u + 1u];
        right_b = clip->samples[(size_t)index_b * 2u + 1u];
    }
    *left = left_a + (left_b - left_a) * fraction;
    *right = right_a + (right_b - right_a) * fraction;
}

SageResult sage_audio_mix(SageAudioMixer* mixer,
                          float* output_interleaved_stereo,
                          uint32_t frame_count) {
    uint32_t frame;
    uint32_t i;
    if (!mixer || (!output_interleaved_stereo && frame_count)) return SAGE_ERR_ARGUMENT;
    for (frame = 0; frame < frame_count; ++frame) {
        float left = 0.0f;
        float right = 0.0f;
        advance_ramps(mixer);
        if (mixer->transport_playing) {
            for (i = 1; i <= mixer->stem_count; ++i) {
                const SageAudioStemSlot* stem = &mixer->stems[i];
                const SageAudioClipSlot* clip;
                float sample_left;
                float sample_right;
                float gain;
                if (!stem->occupied || stem->gain.value <= 0.0f) continue;
                clip = &mixer->clips[stem->clip];
                read_clip_frame(clip,
                                (double)(mixer->transport_frame % clip->frame_count),
                                1,
                                &sample_left,
                                &sample_right);
                gain = stem->gain.value * bus_gain(mixer, stem->bus);
                if (stem->parameter)
                    gain *= parameter_multiplier(mixer, stem);
                left += sample_left * gain;
                right += sample_right * gain;
            }
            mixer->transport_frame++;
        }
        for (i = 0; i < mixer->max_voices; ++i) {
            SageAudioVoice* voice = &mixer->voices[i];
            const SageAudioClipSlot* clip;
            float sample_left;
            float sample_right;
            float gain;
            if (!voice->active) continue;
            clip = &mixer->clips[voice->clip];
            if (voice->position >= clip->frame_count) {
                voice->active = 0;
                continue;
            }
            read_clip_frame(clip, voice->position, 0, &sample_left, &sample_right);
            gain = voice->gain * bus_gain(mixer, voice->bus);
            left += sample_left * gain;
            right += sample_right * gain;
            voice->position += voice->pitch;
            if (voice->position >= clip->frame_count) voice->active = 0;
        }
        if (left > 1.0f) left = 1.0f;
        if (left < -1.0f) left = -1.0f;
        if (right > 1.0f) right = 1.0f;
        if (right < -1.0f) right = -1.0f;
        output_interleaved_stereo[(size_t)frame * 2u] = left;
        output_interleaved_stereo[(size_t)frame * 2u + 1u] = right;
        mixer->rendered_frames++;
    }
    return SAGE_OK;
}
