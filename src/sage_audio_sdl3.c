#include "sage/sage_audio_sdl3.h"

#include <SDL3/SDL.h>

#include <stdlib.h>
#include <string.h>

#define SAGE_AUDIO_SDL3_CHUNK_FRAMES 2048u
#define SAGE_AUDIO_SDL3_FRAME_BYTES (sizeof(float) * 2u)

struct SageAudioDevice {
    SageAudioMixer* mixer;
    SDL_AudioStream* stream;
    float* scratch;
    SageResult last_error;
};

static void SDLCALL audio_request(void* user_data,
                                  SDL_AudioStream* stream,
                                  int additional_amount,
                                  int total_amount) {
    SageAudioDevice* device = (SageAudioDevice*)user_data;
    int remaining = additional_amount;
    (void)total_amount;
    if (!device || !stream || additional_amount <= 0) return;

    while (remaining >= (int)SAGE_AUDIO_SDL3_FRAME_BYTES) {
        uint32_t frames = (uint32_t)(remaining / (int)SAGE_AUDIO_SDL3_FRAME_BYTES);
        int byte_count;
        if (frames > SAGE_AUDIO_SDL3_CHUNK_FRAMES)
            frames = SAGE_AUDIO_SDL3_CHUNK_FRAMES;
        byte_count = (int)(frames * SAGE_AUDIO_SDL3_FRAME_BYTES);
        device->last_error = sage_audio_mix(device->mixer, device->scratch, frames);
        if (device->last_error != SAGE_OK)
            memset(device->scratch, 0, (size_t)byte_count);
        if (!SDL_PutAudioStreamData(stream, device->scratch, byte_count)) {
            device->last_error = SAGE_ERR_AUDIO;
            return;
        }
        remaining -= byte_count;
    }

    if (remaining > 0) {
        static const unsigned char silence[SAGE_AUDIO_SDL3_FRAME_BYTES] = {0};
        if (!SDL_PutAudioStreamData(stream, silence, remaining))
            device->last_error = SAGE_ERR_AUDIO;
    }
}

SageAudioDevice* sage_audio_sdl3_device_open(SageAudioMixer* mixer) {
    SageAudioDevice* device;
    SDL_AudioSpec spec;
    uint32_t sample_rate;
    if (!mixer) return NULL;
    sample_rate = sage_audio_sample_rate(mixer);
    if (sample_rate < 8000u || sample_rate > 192000u) return NULL;
    device = (SageAudioDevice*)calloc(1, sizeof(*device));
    if (!device) return NULL;
    device->scratch = (float*)calloc(
        (size_t)SAGE_AUDIO_SDL3_CHUNK_FRAMES * 2u, sizeof(float));
    if (!device->scratch) {
        free(device);
        return NULL;
    }
    device->mixer = mixer;
    device->last_error = SAGE_OK;
    SDL_zero(spec);
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = (int)sample_rate;
    device->stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_request, device);
    if (!device->stream || !SDL_ResumeAudioStreamDevice(device->stream)) {
        if (device->stream) SDL_DestroyAudioStream(device->stream);
        free(device->scratch);
        free(device);
        return NULL;
    }
    return device;
}

void sage_audio_sdl3_device_close(SageAudioDevice* device) {
    if (!device) return;
    SDL_DestroyAudioStream(device->stream);
    free(device->scratch);
    free(device);
}

SageResult sage_audio_sdl3_device_lock(SageAudioDevice* device) {
    if (!device || !device->stream) return SAGE_ERR_ARGUMENT;
    return SDL_LockAudioStream(device->stream) ? SAGE_OK : SAGE_ERR_AUDIO;
}

SageResult sage_audio_sdl3_device_unlock(SageAudioDevice* device) {
    if (!device || !device->stream) return SAGE_ERR_ARGUMENT;
    return SDL_UnlockAudioStream(device->stream) ? SAGE_OK : SAGE_ERR_AUDIO;
}

SageResult sage_audio_sdl3_device_last_error(const SageAudioDevice* device) {
    return device ? device->last_error : SAGE_ERR_ARGUMENT;
}
