/* C-compatible stem mixer and event boundary. */
#ifndef SAGE_AUDIO_H
#define SAGE_AUDIO_H

#include "sage_phys.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAGE_AUDIO_NAME_CAPACITY 64u

typedef uint32_t SageAudioClip;
typedef uint32_t SageAudioBus;
typedef uint32_t SageAudioParameter;
typedef uint32_t SageAudioStem;
typedef uint32_t SageAudioEvent;

typedef enum SageAudioContactSignal {
    SAGE_AUDIO_CONTACT_RELATIVE_SPEED = 0,
    SAGE_AUDIO_CONTACT_IMPULSE = 1
} SageAudioContactSignal;

typedef struct SageAudioConfig {
    uint32_t sample_rate_hz;
    uint32_t max_clips;
    uint32_t max_voices;
    uint32_t max_stems;
    uint32_t max_events;
    uint32_t max_parameters;
    uint32_t max_buses; /* Additional buses; bus 0 is the master bus. */
    uint32_t max_contact_profiles;
} SageAudioConfig;

typedef struct SageAudioMixer SageAudioMixer;

typedef struct SageAudioContactProfile {
    uint32_t material_tag_a;
    uint32_t material_tag_b;
    SageAudioEvent event;
    SageAudioContactSignal signal;
    float minimum_signal;
    float full_gain_signal;
    float cooldown_seconds;
} SageAudioContactProfile;

SageAudioConfig sage_audio_config_default(void);
SageAudioMixer* sage_audio_mixer_create(const SageAudioConfig* config);
void sage_audio_mixer_destroy(SageAudioMixer* mixer);
uint32_t sage_audio_sample_rate(const SageAudioMixer* mixer);

/*
 * Clips use interleaved float PCM at the mixer's sample rate and remain owned
 * by the caller until the mixer is destroyed. Only mono and stereo clips are
 * accepted. Samples must contain finite values in [-1, 1].
 */
SageResult sage_audio_clip_register(SageAudioMixer* mixer,
                                    const float* samples,
                                    uint32_t frame_count,
                                    uint32_t channels,
                                    SageAudioClip* out_clip);

/* Bus 0 is the master bus; created buses route directly to it. */
SageResult sage_audio_bus_create(SageAudioMixer* mixer,
                                 const char* name,
                                 SageAudioBus* out_bus);
SageResult sage_audio_bus_fade(SageAudioMixer* mixer,
                               SageAudioBus bus,
                               float gain,
                               float duration_seconds);

SageResult sage_audio_parameter_create(SageAudioMixer* mixer,
                                       const char* name,
                                       float initial_value,
                                       SageAudioParameter* out_parameter);
SageResult sage_audio_parameter_fade(SageAudioMixer* mixer,
                                     SageAudioParameter parameter,
                                     float value,
                                     float duration_seconds);

/*
 * Stems share one sample-accurate transport. Each stem loops its clip and can
 * fade independently; an optional parameter maps [minimum, maximum] to a
 * linear stem gain multiplier of [0, 1].
 */
SageResult sage_audio_stem_create(SageAudioMixer* mixer,
                                  const char* name,
                                  SageAudioClip clip,
                                  SageAudioBus bus,
                                  SageAudioStem* out_stem);
SageResult sage_audio_stem_bind_parameter(SageAudioMixer* mixer,
                                          SageAudioStem stem,
                                          SageAudioParameter parameter,
                                          float minimum,
                                          float maximum);
SageResult sage_audio_stem_fade(SageAudioMixer* mixer,
                                SageAudioStem stem,
                                float gain,
                                float duration_seconds);
SageResult sage_audio_music_start(SageAudioMixer* mixer, int restart_transport);
SageResult sage_audio_music_pause(SageAudioMixer* mixer);

/* Events map names to one-shot clips; event names are unique in a mixer. */
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
                                   SageAudioEvent* out_event);
SageResult sage_audio_event_emit(SageAudioMixer* mixer,
                                 SageAudioEvent event,
                                 float intensity);
SageResult sage_audio_trigger_emit(SageAudioMixer* mixer,
                                   const SageAudioTrigger* trigger,
                                   float intensity);
SageResult sage_audio_entity_trigger_emit(SageAudioMixer* mixer,
                                          const SageWorld* world,
                                          SageEntity entity,
                                          float intensity);

/* Routes contacts from sage_phys to configured impact events by material tags. */
SageResult sage_audio_contact_profile_set(
    SageAudioMixer* mixer,
    const SageAudioContactProfile* profile);
SageResult sage_audio_contact_consume(SageAudioMixer* mixer,
                                      const SageContactEvent* contact);

/* Renders interleaved float stereo frames. Call from the audio thread or pump. */
SageResult sage_audio_mix(SageAudioMixer* mixer,
                          float* output_interleaved_stereo,
                          uint32_t frame_count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SAGE_AUDIO_H */
