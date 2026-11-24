/* Optional SDL3 playback device for sage_audio. */
#ifndef SAGE_AUDIO_SDL3_H
#define SAGE_AUDIO_SDL3_H

#include "sage_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SageAudioDevice SageAudioDevice;

/* Open after SDL audio has been initialized. Starts playback when successful. */
SageAudioDevice* sage_audio_sdl3_device_open(SageAudioMixer* mixer);
void sage_audio_sdl3_device_close(SageAudioDevice* device);

/*
 * The SDL callback mixes on its audio thread. Lock before changing mixer state
 * (clips, buses, parameters, stems, or events) and unlock after the changes.
 */
SageResult sage_audio_sdl3_device_lock(SageAudioDevice* device);
SageResult sage_audio_sdl3_device_unlock(SageAudioDevice* device);
SageResult sage_audio_sdl3_device_last_error(const SageAudioDevice* device);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SAGE_AUDIO_SDL3_H */
