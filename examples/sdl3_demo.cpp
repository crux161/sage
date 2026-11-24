#include "sage/sage_audio.h"
#include "sage/sage_audio_sdl3.h"
#include "sage/sage_ecs.h"
#include "sage/sage_phys.h"
#include "sage/sage_renderer.h"

#include <SDL3/SDL.h>

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <vector>

namespace {

const int kWidth = 960;
const int kHeight = 540;
const float kWorldWidth = 16.0f;
const float kWorldHeight = 9.0f;

struct DemoState {
    SageEntity paddle;
    bool move_up;
    bool move_down;
};

SageTransform* find_transform(SageWorld* world, SageEntity entity) {
    SageTransformView view;
    if (sage_ecs_transform_view(world, &view) != SAGE_OK) return NULL;
    for (uint32_t i = 0; i < view.count; ++i) {
        if (view.entities[i] == entity) return &view.components[i];
    }
    return NULL;
}

const SageRenderBinding* find_binding(const SageRenderScene* scene, SageEntity entity) {
    for (uint32_t i = 0; i < scene->binding_count; ++i) {
        if (scene->binding_entities[i] == entity) return &scene->bindings[i];
    }
    return NULL;
}

bool inside_shape(const SageTransform& transform,
                  const SageRenderBinding& binding,
                  float x_m,
                  float y_m) {
    const float half_x = 0.5f * std::fabs(transform.scale[0]);
    const float half_y = 0.5f * std::fabs(transform.scale[1]);
    const float dx = x_m - transform.position_m[0];
    const float dy = y_m - transform.position_m[1];
    if (half_x <= 0.0f || half_y <= 0.0f) return false;

    if (binding.shader_id == 1u) {
        return std::fabs(dx) <= half_x && std::fabs(dy) <= half_y;
    }
    const float nx = dx / half_x;
    const float ny = dy / half_y;
    return nx * nx + ny * ny <= 1.0f;
}

SagePhysicsBody make_body(SagePhysicsShape shape,
                          uint32_t flags,
                          uint32_t material_tag,
                          float mass_kg,
                          float friction,
                          float restitution,
                          float x_m,
                          float y_m,
                          float z_m) {
    SagePhysicsBody body = {};
    body.shape = shape;
    body.flags = flags;
    body.material_tag = material_tag;
    body.mass_kg = mass_kg;
    body.friction = friction;
    body.restitution = restitution;
    body.dimensions_m[0] = x_m;
    body.dimensions_m[1] = y_m;
    body.dimensions_m[2] = z_m;
    return body;
}

bool set_renderable_body(SageWorld* world,
                         SageEntity entity,
                         const SageTransform& transform,
                         const SageRenderBinding& binding,
                         const SagePhysicsBody& body) {
    return sage_ecs_transform_set(world, entity, &transform) == SAGE_OK &&
           sage_ecs_render_binding_set(world, entity, &binding) == SAGE_OK &&
           sage_ecs_physics_body_set(world, entity, &body) == SAGE_OK;
}

} // namespace

extern "C" void sage_demo_input_system(SageWorld* world,
                                       const SageSystemContext* context,
                                       void* user_data) {
    DemoState* state = static_cast<DemoState*>(user_data);
    SageTransform* paddle = find_transform(world, state->paddle);
    if (!paddle) return;

    const float direction = (state->move_up ? 1.0f : 0.0f) -
                            (state->move_down ? 1.0f : 0.0f);
    paddle->position_m[1] += direction * 6.0f * context->delta_seconds;
    if (paddle->position_m[1] > 3.2f) paddle->position_m[1] = 3.2f;
    if (paddle->position_m[1] < -3.2f) paddle->position_m[1] = -3.2f;
}

extern "C" void sage_demo_shader(float out_rgba[4],
                                 float frag_x,
                                 float frag_y,
                                 float resolution_x,
                                 float resolution_y,
                                 float,
                                 const SageRenderScene* scene,
                                 const void*) {
    const float pixels_per_meter = resolution_x / kWorldWidth;
    const float x_m = (frag_x - 0.5f * resolution_x) / pixels_per_meter;
    const float y_m = (frag_y - 0.5f * resolution_y) / pixels_per_meter;

    out_rgba[0] = 0.025f;
    out_rgba[1] = 0.035f + 0.025f * frag_y / resolution_y;
    out_rgba[2] = 0.055f;
    out_rgba[3] = 1.0f;

    for (uint32_t i = 0; i < scene->transform_count; ++i) {
        const SageEntity entity = scene->transform_entities[i];
        const SageRenderBinding* binding = find_binding(scene, entity);
        const SageTransform& transform = scene->transforms[i];
        if (!binding || !inside_shape(transform, *binding, x_m, y_m)) continue;

        if (binding->material_id == 2u) {
            out_rgba[0] = 1.0f;
            out_rgba[1] = 0.38f;
            out_rgba[2] = 0.12f;
        } else if (binding->material_id == 3u) {
            out_rgba[0] = 0.16f;
            out_rgba[1] = 0.34f;
            out_rgba[2] = 0.39f;
        } else {
            out_rgba[0] = 0.18f;
            out_rgba[1] = 0.78f;
            out_rgba[2] = 0.9f;
        }
    }
}

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = NULL;
    SDL_Renderer* display = NULL;
    SDL_Texture* texture = NULL;
    SageWorld* world = NULL;
    SagePhysicsWorld* physics = NULL;
    SageRenderer* renderer = NULL;
    SageAudioMixer* audio = NULL;
    SageAudioDevice* audio_device = NULL;
    std::vector<uint8_t> pixels;
    std::vector<float> impact_samples;

    const auto cleanup = [&]() {
        if (audio_device) sage_audio_sdl3_device_close(audio_device);
        if (audio) sage_audio_mixer_destroy(audio);
        if (physics) sage_phys_world_destroy(physics);
        if (renderer) sage_renderer_destroy(renderer);
        if (world) sage_ecs_world_destroy(world);
        if (texture) SDL_DestroyTexture(texture);
        if (display) SDL_DestroyRenderer(display);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    };

    if (!SDL_CreateWindowAndRenderer("SAGE · SDL3 + Eshi + Bullet", kWidth, kHeight,
                                     0, &window, &display)) {
        std::fprintf(stderr, "SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
        cleanup();
        return 1;
    }
    texture = SDL_CreateTexture(display, SDL_PIXELFORMAT_RGBA32,
                                SDL_TEXTUREACCESS_STREAMING, kWidth, kHeight);
    if (!texture) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        cleanup();
        return 1;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);

    SageWorldConfig world_config = sage_ecs_config_default();
    world_config.max_entities = 16;
    world = sage_ecs_world_create(&world_config);
    SageRendererConfig renderer_config = sage_renderer_config_default();
    renderer_config.width = kWidth;
    renderer_config.height = kHeight;
    renderer = sage_renderer_create(&renderer_config);
    SagePhysicsConfig physics_config = sage_phys_config_default();
    physics_config.gravity_mps2[0] = 0.0f;
    physics_config.gravity_mps2[1] = 0.0f;
    physics_config.gravity_mps2[2] = 0.0f;
    physics = sage_phys_world_create(&physics_config);
    audio = sage_audio_mixer_create(NULL);
    if (!world || !renderer || !physics || !audio) {
        std::fprintf(stderr, "Failed to create a SAGE subsystem.\n");
        cleanup();
        return 1;
    }

    const uint32_t sample_rate = sage_audio_sample_rate(audio);
    const uint32_t impact_frames = sample_rate / 8u;
    impact_samples.resize(impact_frames);
    for (uint32_t i = 0; i < impact_frames; ++i) {
        const float time = (float)i / (float)sample_rate;
        const float sweep = 1400.0f - 700.0f * time;
        const float envelope = std::exp(-34.0f * time);
        impact_samples[i] = 0.55f * envelope *
                            std::sin(6.28318530718f * sweep * time);
    }

    SageAudioClip impact_clip = 0;
    SageAudioBus effects_bus = 0;
    SageAudioEvent impact_event = 0;
    if (sage_audio_clip_register(audio, impact_samples.data(), impact_frames, 1,
                                 &impact_clip) != SAGE_OK ||
        sage_audio_bus_create(audio, "effects", &effects_bus) != SAGE_OK ||
        sage_audio_event_create(audio, "impact", impact_clip, effects_bus,
                                0.15f, 6.0f, 0.8f, 0.82f, 1.3f, 8,
                                &impact_event) != SAGE_OK) {
        std::fprintf(stderr, "Failed to initialize the SAGE mixer.\n");
        cleanup();
        return 1;
    }

    SageAudioContactProfile wall_profile = {};
    wall_profile.material_tag_a = 1u;
    wall_profile.material_tag_b = 2u;
    wall_profile.event = impact_event;
    wall_profile.signal = SAGE_AUDIO_CONTACT_IMPULSE;
    wall_profile.minimum_signal = 0.15f;
    wall_profile.full_gain_signal = 5.0f;
    wall_profile.cooldown_seconds = 0.12f;
    SageAudioContactProfile paddle_profile = wall_profile;
    paddle_profile.material_tag_a = 2u;
    paddle_profile.material_tag_b = 3u;
    if (sage_audio_contact_profile_set(audio, &wall_profile) != SAGE_OK ||
        sage_audio_contact_profile_set(audio, &paddle_profile) != SAGE_OK) {
        std::fprintf(stderr, "Failed to connect physics materials to audio events.\n");
        cleanup();
        return 1;
    }

    audio_device = sage_audio_sdl3_device_open(audio);
    if (!audio_device) {
        std::fprintf(stderr, "SDL3 audio device failed: %s\n", SDL_GetError());
        cleanup();
        return 1;
    }

    DemoState state = {};
    state.paddle = sage_ecs_entity_create(world);
    SageEntity ball = sage_ecs_entity_create(world);
    SageEntity walls[4];
    for (int i = 0; i < 4; ++i) walls[i] = sage_ecs_entity_create(world);
    if (state.paddle == SAGE_NULL_ENTITY || ball == SAGE_NULL_ENTITY ||
        walls[0] == SAGE_NULL_ENTITY || walls[1] == SAGE_NULL_ENTITY ||
        walls[2] == SAGE_NULL_ENTITY || walls[3] == SAGE_NULL_ENTITY) {
        std::fprintf(stderr, "Failed to allocate demo entities.\n");
        cleanup();
        return 1;
    }

    SageTransform transforms[6];
    SageRenderBinding bindings[6];
    SagePhysicsBody bodies[6];
    for (int i = 0; i < 6; ++i) {
        sage_transform_identity(&transforms[i]);
        bindings[i] = SageRenderBinding{1u, 3u, 0u};
    }
    transforms[0].position_m[0] = -6.5f;
    transforms[0].scale[0] = 0.38f;
    transforms[0].scale[1] = 1.8f;
    bindings[0].material_id = 1u;
    bodies[0] = make_body(SAGE_PHYSICS_SHAPE_BOX, SAGE_PHYSICS_BODY_KINEMATIC,
                          3u, 0.0f, 0.5f, 0.85f, 0.19f, 0.9f, 0.3f);

    transforms[1].scale[0] = 0.72f;
    transforms[1].scale[1] = 0.72f;
    bindings[1].material_id = 2u;
    bodies[1] = make_body(SAGE_PHYSICS_SHAPE_SPHERE, 0u,
                          2u, 1.0f, 0.12f, 0.96f, 0.36f, 0.0f, 0.0f);

    const float wall_x[4] = { -7.8f, 7.8f, 0.0f, 0.0f };
    const float wall_y[4] = { 0.0f, 0.0f, -4.3f, 4.3f };
    const float wall_half_x[4] = { 0.2f, 0.2f, 7.6f, 7.6f };
    const float wall_half_y[4] = { 4.5f, 4.5f, 0.2f, 0.2f };
    for (int i = 0; i < 4; ++i) {
        const int index = i + 2;
        transforms[index].position_m[0] = wall_x[i];
        transforms[index].position_m[1] = wall_y[i];
        transforms[index].scale[0] = wall_half_x[i] * 2.0f;
        transforms[index].scale[1] = wall_half_y[i] * 2.0f;
        bodies[index] = make_body(SAGE_PHYSICS_SHAPE_BOX, SAGE_PHYSICS_BODY_STATIC,
                                  1u, 0.0f, 0.4f, 0.9f,
                                  wall_half_x[i], wall_half_y[i], 0.5f);
    }

    if (!set_renderable_body(world, state.paddle, transforms[0], bindings[0], bodies[0]) ||
        !set_renderable_body(world, ball, transforms[1], bindings[1], bodies[1])) {
        std::fprintf(stderr, "Failed to initialize SAGE game entities.\n");
        cleanup();
        return 1;
    }
    for (int i = 0; i < 4; ++i) {
        if (!set_renderable_body(world, walls[i], transforms[i + 2],
                                 bindings[i + 2], bodies[i + 2])) {
            std::fprintf(stderr, "Failed to initialize SAGE arena walls.\n");
            cleanup();
            return 1;
        }
    }

    if (sage_ecs_system_register(world, SAGE_PHASE_INPUT,
                                 sage_demo_input_system, &state) != SAGE_OK ||
        sage_renderer_set_shader(renderer, sage_demo_shader, NULL) != SAGE_OK ||
        sage_phys_world_bind(physics, world) != SAGE_OK ||
        sage_phys_body_add(physics, state.paddle) != SAGE_OK ||
        sage_phys_body_add(physics, ball) != SAGE_OK) {
        std::fprintf(stderr, "Failed to initialize SAGE systems or physics bodies.\n");
        cleanup();
        return 1;
    }
    for (int i = 0; i < 4; ++i) {
        if (sage_phys_body_add(physics, walls[i]) != SAGE_OK) {
            std::fprintf(stderr, "Failed to initialize an arena body.\n");
            cleanup();
            return 1;
        }
    }
    const float initial_linear_velocity[3] = { 4.0f, 2.2f, 0.0f };
    const float initial_angular_velocity[3] = { 0.0f, 0.0f, 0.0f };
    if (sage_phys_body_set_velocity(physics, ball, initial_linear_velocity,
                                    initial_angular_velocity) != SAGE_OK) {
        std::fprintf(stderr, "Failed to set initial ball velocity.\n");
        cleanup();
        return 1;
    }

    pixels.resize((size_t)kWidth * kHeight * 4u);
    bool running = true;
    uint64_t previous_counter = SDL_GetPerformanceCounter();
    const uint64_t counter_frequency = SDL_GetPerformanceFrequency();
    double time_seconds = 0.0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                const bool down = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.scancode == SDL_SCANCODE_ESCAPE && down) running = false;
                if (event.key.scancode == SDL_SCANCODE_W) state.move_up = down;
                if (event.key.scancode == SDL_SCANCODE_S) state.move_down = down;
            }
        }
        if (!running) break;

        const uint64_t now = SDL_GetPerformanceCounter();
        const float delta_seconds = counter_frequency
                                        ? (float)((double)(now - previous_counter) /
                                                  (double)counter_frequency)
                                        : 0.0f;
        previous_counter = now;
        time_seconds += delta_seconds;

        if (sage_ecs_advance(world, delta_seconds) != SAGE_OK ||
            sage_phys_last_error(physics) != SAGE_OK) {
            std::fprintf(stderr, "SAGE simulation failed.\n");
            running = false;
            break;
        }

        SageContactEvent contacts[128];
        int32_t contact_count;
        while ((contact_count = sage_phys_contacts_poll(physics, contacts, 128)) > 0) {
            if (sage_audio_sdl3_device_lock(audio_device) != SAGE_OK) {
                std::fprintf(stderr, "Could not lock the SDL3 audio stream.\n");
                running = false;
                break;
            }
            for (int32_t i = 0; i < contact_count; ++i) {
                const SageResult audio_result = sage_audio_contact_consume(audio, &contacts[i]);
                if (audio_result != SAGE_OK && audio_result != SAGE_ERR_NOT_FOUND &&
                    audio_result != SAGE_ERR_LIMIT) {
                    std::fprintf(stderr, "Physics contact audio routing failed.\n");
                    running = false;
                    break;
                }
            }
            if (sage_audio_sdl3_device_unlock(audio_device) != SAGE_OK) {
                std::fprintf(stderr, "Could not unlock the SDL3 audio stream.\n");
                running = false;
            }
            if (!running) break;
        }
        if (!running) break;

        if (sage_renderer_render(renderer, world, pixels.data(), kWidth * 4,
                                 (float)time_seconds) != SAGE_OK) {
            std::fprintf(stderr, "SAGE rendering failed.\n");
            running = false;
            break;
        }
        if (!SDL_UpdateTexture(texture, NULL, pixels.data(), kWidth * 4) ||
            !SDL_RenderClear(display) ||
            !SDL_RenderTexture(display, texture, NULL, NULL) ||
            !SDL_RenderPresent(display)) {
            std::fprintf(stderr, "SDL presentation failed: %s\n", SDL_GetError());
            running = false;
        }
    }

    cleanup();
    return 0;
}
