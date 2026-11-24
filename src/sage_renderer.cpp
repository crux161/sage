#include "sage/sage_renderer.h"

#include "eshi/eshi.h"

#include <new>

struct SageRendererFrame {
    SageFragmentShaderFn shader;
    const void* user_data;
    SageRenderScene scene;
};

struct SageRenderer {
    EshiWorld* eshi_world;
    SageRendererFrame frame;
};

extern "C" void sage_eshi_material_shader(float out_rgba[4],
                                           float frag_x,
                                           float frag_y,
                                           float resolution_x,
                                           float resolution_y,
                                           float time_seconds,
                                           const void* uniforms) {
    const SageRendererFrame* frame =
        static_cast<const SageRendererFrame*>(uniforms);
    if (!frame || !frame->shader) {
        out_rgba[0] = 0.0f;
        out_rgba[1] = 0.0f;
        out_rgba[2] = 0.0f;
        out_rgba[3] = 1.0f;
        return;
    }

    frame->shader(out_rgba, frag_x, frag_y, resolution_x, resolution_y,
                  time_seconds, &frame->scene, frame->user_data);
}

extern "C" SageRendererConfig sage_renderer_config_default(void) {
    SageRendererConfig config;
    config.width = 960;
    config.height = 540;
    return config;
}

extern "C" SageRenderer* sage_renderer_create(const SageRendererConfig* input_config) {
    SageRendererConfig config = input_config ? *input_config : sage_renderer_config_default();
    SageRenderer* renderer;
    EshiConfig eshi_config;
    EshiMaterial material = {};

    if (config.width <= 0 || config.height <= 0 || config.width > INT32_MAX / 4)
        return NULL;

    renderer = new (std::nothrow) SageRenderer();
    if (!renderer) return NULL;

    eshi_config = eshi_config_default();
    eshi_config.width = config.width;
    eshi_config.height = config.height;
    eshi_config.grade = ESHI_GRADE_INK;
    renderer->eshi_world = eshi_world_create(&eshi_config);
    if (!renderer->eshi_world) {
        delete renderer;
        return NULL;
    }

    material.cpu_shader = sage_eshi_material_shader;
    material.uniform_data = &renderer->frame;
    material.uniform_size = sizeof(renderer->frame);
    if (eshi_material_set(renderer->eshi_world, &material) != ESHI_OK) {
        eshi_world_destroy(renderer->eshi_world);
        delete renderer;
        return NULL;
    }
    return renderer;
}

extern "C" void sage_renderer_destroy(SageRenderer* renderer) {
    if (!renderer) return;
    eshi_world_destroy(renderer->eshi_world);
    delete renderer;
}

extern "C" SageResult sage_renderer_set_shader(SageRenderer* renderer,
                                                SageFragmentShaderFn shader,
                                                const void* user_data) {
    if (!renderer || !shader) return SAGE_ERR_ARGUMENT;
    renderer->frame.shader = shader;
    renderer->frame.user_data = user_data;
    return SAGE_OK;
}

extern "C" SageResult sage_renderer_render(SageRenderer* renderer,
                                            SageWorld* world,
                                            uint8_t* rgba_pixels,
                                            int32_t stride_bytes,
                                            float time_seconds) {
    SageTransformView transforms;
    SageRenderBindingView bindings;
    SageResult result;

    if (!renderer || !world || !rgba_pixels || stride_bytes <= 0 ||
        !renderer->frame.shader)
        return SAGE_ERR_ARGUMENT;

    result = sage_ecs_transform_view(world, &transforms);
    if (result != SAGE_OK) return result;
    result = sage_ecs_render_binding_view(world, &bindings);
    if (result != SAGE_OK) return result;

    renderer->frame.scene.transform_entities = transforms.entities;
    renderer->frame.scene.transforms = transforms.components;
    renderer->frame.scene.transform_count = transforms.count;
    renderer->frame.scene.binding_entities = bindings.entities;
    renderer->frame.scene.bindings = bindings.components;
    renderer->frame.scene.binding_count = bindings.count;

    return eshi_render(renderer->eshi_world, rgba_pixels, stride_bytes, time_seconds) == ESHI_OK
               ? SAGE_OK
               : SAGE_ERR_RENDER;
}

