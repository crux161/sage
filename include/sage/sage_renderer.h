/* Eshi-backed renderer boundary for SAGE scenes. */
#ifndef SAGE_RENDERER_H
#define SAGE_RENDERER_H

#include <stdint.h>
#include "sage_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SageRenderScene {
    const SageEntity* transform_entities;
    const SageTransform* transforms;
    uint32_t transform_count;

    const SageEntity* binding_entities;
    const SageRenderBinding* bindings;
    uint32_t binding_count;
} SageRenderScene;

typedef void (*SageFragmentShaderFn)(float out_rgba[4],
                                     float frag_x,
                                     float frag_y,
                                     float resolution_x,
                                     float resolution_y,
                                     float time_seconds,
                                     const SageRenderScene* scene,
                                     const void* user_data);

typedef struct SageRendererConfig {
    int32_t width;
    int32_t height;
} SageRendererConfig;

typedef struct SageRenderer SageRenderer;

SageRendererConfig sage_renderer_config_default(void);
SageRenderer* sage_renderer_create(const SageRendererConfig* config);
void sage_renderer_destroy(SageRenderer* renderer);

/*
 * The callback and its user_data must remain valid until replaced or
 * destroyed. A backend may call the shader concurrently for different pixels;
 * keep the callback read-only and thread-safe.
 */
SageResult sage_renderer_set_shader(SageRenderer* renderer,
                                    SageFragmentShaderFn shader,
                                    const void* user_data);

/*
 * Renders the current SAGE transform and render-binding views through Eshi's
 * Ink material backend into a caller-owned RGBA8 buffer. Fragment coordinates
 * use a bottom-left origin. Scene arrays are borrowed only for this synchronous
 * call; match components by entity handle.
 */
SageResult sage_renderer_render(SageRenderer* renderer,
                                SageWorld* world,
                                uint8_t* rgba_pixels,
                                int32_t stride_bytes,
                                float time_seconds);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SAGE_RENDERER_H */
