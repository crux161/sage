#include "sage/sage_ecs.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define SAGE_DEFAULT_MAX_ENTITIES 4096u
#define SAGE_DEFAULT_FIXED_STEP (1.0f / 60.0f)
#define SAGE_DEFAULT_MAX_STEPS 8u
#define SAGE_MAX_STEPS 64u

typedef enum SagePoolId {
    SAGE_POOL_TRANSFORM = 0,
    SAGE_POOL_RENDER_BINDING,
    SAGE_POOL_PHYSICS_BODY,
    SAGE_POOL_AUDIO_TRIGGER,
    SAGE_POOL_COUNT
} SagePoolId;

typedef struct SageComponentPool {
    SageEntity* entities;
    uint32_t* sparse;
    void* components;
    uint32_t count;
    uint32_t capacity;
    size_t component_size;
} SageComponentPool;

typedef struct SageSystemEntry {
    SageSystemFn function;
    void* user_data;
} SageSystemEntry;

typedef struct SageSystemList {
    SageSystemEntry* entries;
    uint32_t count;
    uint32_t capacity;
} SageSystemList;

struct SageWorld {
    uint32_t max_entities;
    uint32_t live_entities;
    uint16_t* generations;
    uint8_t* alive;
    uint32_t* free_indices;
    uint32_t free_count;

    SageComponentPool pools[SAGE_POOL_COUNT];
    SageSystemList systems[SAGE_PHASE_COUNT];

    float fixed_step_seconds;
    uint32_t max_fixed_steps_per_frame;
    double accumulator_seconds;
    uint64_t frame_index;
    uint64_t simulation_tick;
    int stepping;
};

static SageEntity make_entity(uint32_t index, uint16_t generation) {
    return ((SageEntity)generation << SAGE_ENTITY_INDEX_BITS) | index;
}

static int world_entity_valid(const SageWorld* world, SageEntity entity) {
    uint32_t index;
    uint32_t generation;

    if (!world || entity == SAGE_NULL_ENTITY) return 0;
    index = SAGE_ENTITY_INDEX(entity);
    generation = SAGE_ENTITY_GENERATION(entity);
    if (index == 0 || index > world->max_entities || generation == 0) return 0;
    return world->alive[index] && world->generations[index] == generation;
}

static void pool_destroy(SageComponentPool* pool) {
    free(pool->entities);
    free(pool->sparse);
    free(pool->components);
    memset(pool, 0, sizeof(*pool));
}

static SageResult pool_init(SageComponentPool* pool,
                            uint32_t capacity,
                            size_t component_size) {
    SageEntity* entities;
    uint32_t* sparse;
    void* components;

    if (pool->components) return SAGE_OK;
    if (!capacity || !component_size) return SAGE_ERR_ARGUMENT;
    if ((size_t)capacity > ((size_t)-1) / component_size ||
        (size_t)capacity > ((size_t)-1) / sizeof(*entities) ||
        (size_t)capacity + 1u > ((size_t)-1) / sizeof(*sparse))
        return SAGE_ERR_NOMEM;

    entities = (SageEntity*)malloc((size_t)capacity * sizeof(*entities));
    sparse = (uint32_t*)calloc((size_t)capacity + 1u, sizeof(*sparse));
    components = malloc((size_t)capacity * component_size);
    if (!entities || !sparse || !components) {
        free(entities);
        free(sparse);
        free(components);
        return SAGE_ERR_NOMEM;
    }

    pool->entities = entities;
    pool->sparse = sparse;
    pool->components = components;
    pool->capacity = capacity;
    pool->component_size = component_size;
    return SAGE_OK;
}

static uint32_t pool_dense_index(const SageComponentPool* pool, SageEntity entity) {
    uint32_t index = SAGE_ENTITY_INDEX(entity);
    uint32_t encoded;
    uint32_t dense;

    if (!pool->sparse || index > pool->capacity) return UINT32_MAX;
    encoded = pool->sparse[index];
    if (encoded == 0) return UINT32_MAX;
    dense = encoded - 1u;
    if (dense >= pool->count || pool->entities[dense] != entity) return UINT32_MAX;
    return dense;
}

static const void* pool_get(const SageComponentPool* pool, SageEntity entity) {
    uint32_t dense = pool_dense_index(pool, entity);
    if (dense == UINT32_MAX) return NULL;
    return (const unsigned char*)pool->components +
           (size_t)dense * pool->component_size;
}

static SageResult pool_set(SageComponentPool* pool,
                           uint32_t max_entities,
                           SageEntity entity,
                           const void* component,
                           size_t component_size,
                           int stepping) {
    uint32_t dense;
    uint32_t index = SAGE_ENTITY_INDEX(entity);
    SageResult result;

    if (!component) return SAGE_ERR_ARGUMENT;
    dense = pool_dense_index(pool, entity);
    if (dense != UINT32_MAX) {
        memcpy((unsigned char*)pool->components +
                   (size_t)dense * pool->component_size,
               component, pool->component_size);
        return SAGE_OK;
    }
    if (stepping) return SAGE_ERR_BUSY;

    result = pool_init(pool, max_entities, component_size);
    if (result != SAGE_OK) return result;
    if (pool->count >= pool->capacity) return SAGE_ERR_LIMIT;

    dense = pool->count++;
    pool->entities[dense] = entity;
    pool->sparse[index] = dense + 1u;
    memcpy((unsigned char*)pool->components +
               (size_t)dense * pool->component_size,
           component, pool->component_size);
    return SAGE_OK;
}

static SageResult pool_remove(SageComponentPool* pool, SageEntity entity, int stepping) {
    uint32_t dense = pool_dense_index(pool, entity);
    uint32_t last;
    uint32_t moved_index;

    if (dense == UINT32_MAX) return SAGE_ERR_NOT_FOUND;
    if (stepping) return SAGE_ERR_BUSY;

    last = pool->count - 1u;
    pool->sparse[SAGE_ENTITY_INDEX(entity)] = 0;
    if (dense != last) {
        SageEntity moved = pool->entities[last];
        moved_index = SAGE_ENTITY_INDEX(moved);
        pool->entities[dense] = moved;
        pool->sparse[moved_index] = dense + 1u;
        memcpy((unsigned char*)pool->components +
                   (size_t)dense * pool->component_size,
               (unsigned char*)pool->components +
                   (size_t)last * pool->component_size,
               pool->component_size);
    }
    pool->count--;
    return SAGE_OK;
}

static void pool_remove_if_present(SageComponentPool* pool, SageEntity entity) {
    uint32_t dense = pool_dense_index(pool, entity);
    uint32_t last;
    uint32_t moved_index;

    if (dense == UINT32_MAX) return;
    last = pool->count - 1u;
    pool->sparse[SAGE_ENTITY_INDEX(entity)] = 0;
    if (dense != last) {
        SageEntity moved = pool->entities[last];
        moved_index = SAGE_ENTITY_INDEX(moved);
        pool->entities[dense] = moved;
        pool->sparse[moved_index] = dense + 1u;
        memcpy((unsigned char*)pool->components +
                   (size_t)dense * pool->component_size,
               (unsigned char*)pool->components +
                   (size_t)last * pool->component_size,
               pool->component_size);
    }
    pool->count--;
}

static SageResult component_set(SageWorld* world,
                                SageEntity entity,
                                SagePoolId pool_id,
                                const void* component,
                                size_t component_size) {
    if (!world || !component) return SAGE_ERR_ARGUMENT;
    if (!world_entity_valid(world, entity)) return SAGE_ERR_INVALID_ENTITY;
    return pool_set(&world->pools[pool_id], world->max_entities, entity,
                    component, component_size, world->stepping);
}

static SageResult component_remove(SageWorld* world,
                                  SageEntity entity,
                                  SagePoolId pool_id) {
    if (!world) return SAGE_ERR_ARGUMENT;
    if (!world_entity_valid(world, entity)) return SAGE_ERR_INVALID_ENTITY;
    return pool_remove(&world->pools[pool_id], entity, world->stepping);
}

static const void* component_get(const SageWorld* world,
                                SageEntity entity,
                                SagePoolId pool_id) {
    if (!world_entity_valid(world, entity)) return NULL;
    return pool_get(&world->pools[pool_id], entity);
}

static SageResult component_view(SageWorld* world,
                                 SagePoolId pool_id,
                                 void* out_view,
                                 size_t entity_offset,
                                 size_t component_offset,
                                 size_t count_offset) {
    SageComponentPool* pool;
    const SageEntity* entities = NULL;
    void* components = NULL;
    uint32_t count = 0;

    if (!world || !out_view) return SAGE_ERR_ARGUMENT;
    pool = &world->pools[pool_id];
    if (pool->components) {
        entities = pool->entities;
        components = pool->components;
        count = pool->count;
    }
    memcpy((unsigned char*)out_view + entity_offset, &entities, sizeof(entities));
    memcpy((unsigned char*)out_view + component_offset, &components, sizeof(components));
    memcpy((unsigned char*)out_view + count_offset, &count, sizeof(count));
    return SAGE_OK;
}

SageWorldConfig sage_ecs_config_default(void) {
    SageWorldConfig config;
    config.max_entities = SAGE_DEFAULT_MAX_ENTITIES;
    config.fixed_step_seconds = SAGE_DEFAULT_FIXED_STEP;
    config.max_fixed_steps_per_frame = SAGE_DEFAULT_MAX_STEPS;
    return config;
}

SageWorld* sage_ecs_world_create(const SageWorldConfig* input_config) {
    SageWorldConfig config = input_config ? *input_config : sage_ecs_config_default();
    SageWorld* world;
    uint32_t i;

    if (config.max_entities == 0) config.max_entities = SAGE_DEFAULT_MAX_ENTITIES;
    if (config.fixed_step_seconds == 0.0f) config.fixed_step_seconds = SAGE_DEFAULT_FIXED_STEP;
    if (config.max_fixed_steps_per_frame == 0)
        config.max_fixed_steps_per_frame = SAGE_DEFAULT_MAX_STEPS;
    if (config.max_entities > SAGE_MAX_ENTITIES ||
        !isfinite(config.fixed_step_seconds) || config.fixed_step_seconds <= 0.0f ||
        config.max_fixed_steps_per_frame > SAGE_MAX_STEPS) {
        return NULL;
    }

    world = (SageWorld*)calloc(1, sizeof(*world));
    if (!world) return NULL;
    world->max_entities = config.max_entities;
    world->fixed_step_seconds = config.fixed_step_seconds;
    world->max_fixed_steps_per_frame = config.max_fixed_steps_per_frame;

    world->generations = (uint16_t*)calloc((size_t)config.max_entities + 1u,
                                           sizeof(*world->generations));
    world->alive = (uint8_t*)calloc((size_t)config.max_entities + 1u,
                                    sizeof(*world->alive));
    world->free_indices = (uint32_t*)malloc((size_t)config.max_entities *
                                            sizeof(*world->free_indices));
    if (!world->generations || !world->alive || !world->free_indices) {
        sage_ecs_world_destroy(world);
        return NULL;
    }

    for (i = 1; i <= config.max_entities; ++i) {
        world->generations[i] = 1;
        world->free_indices[i - 1u] = config.max_entities - i + 1u;
    }
    world->free_count = config.max_entities;
    return world;
}

void sage_ecs_world_destroy(SageWorld* world) {
    uint32_t i;
    if (!world) return;
    for (i = 0; i < SAGE_POOL_COUNT; ++i) pool_destroy(&world->pools[i]);
    for (i = 0; i < SAGE_PHASE_COUNT; ++i) free(world->systems[i].entries);
    free(world->generations);
    free(world->alive);
    free(world->free_indices);
    free(world);
}

SageEntity sage_ecs_entity_create(SageWorld* world) {
    uint32_t index;
    if (!world || world->stepping || world->free_count == 0) return SAGE_NULL_ENTITY;
    index = world->free_indices[--world->free_count];
    world->alive[index] = 1;
    world->live_entities++;
    return make_entity(index, world->generations[index]);
}

SageResult sage_ecs_entity_destroy(SageWorld* world, SageEntity entity) {
    uint32_t index;
    uint32_t generation;
    uint32_t i;

    if (!world) return SAGE_ERR_ARGUMENT;
    if (!world_entity_valid(world, entity)) return SAGE_ERR_INVALID_ENTITY;
    if (world->stepping) return SAGE_ERR_BUSY;

    index = SAGE_ENTITY_INDEX(entity);
    for (i = 0; i < SAGE_POOL_COUNT; ++i)
        pool_remove_if_present(&world->pools[i], entity);

    world->alive[index] = 0;
    generation = (world->generations[index] + 1u) & SAGE_ENTITY_GENERATION_MASK;
    if (generation == 0) generation = 1;
    world->generations[index] = (uint16_t)generation;
    world->free_indices[world->free_count++] = index;
    world->live_entities--;
    return SAGE_OK;
}

int sage_ecs_entity_alive(const SageWorld* world, SageEntity entity) {
    return world_entity_valid(world, entity);
}

uint32_t sage_ecs_entity_count(const SageWorld* world) {
    return world ? world->live_entities : 0;
}

int sage_ecs_world_is_advancing(const SageWorld* world) {
    return world ? world->stepping : 0;
}

void sage_transform_identity(SageTransform* transform) {
    if (!transform) return;
    memset(transform, 0, sizeof(*transform));
    transform->rotation_xyzw[3] = 1.0f;
    transform->scale[0] = 1.0f;
    transform->scale[1] = 1.0f;
    transform->scale[2] = 1.0f;
}

#define SAGE_DEFINE_COMPONENT_API(name, type, view_type, pool_id) \
    SageResult sage_ecs_##name##_set(SageWorld* world, SageEntity entity, \
                                     const type* component) { \
        return component_set(world, entity, pool_id, component, sizeof(*component)); \
    } \
    SageResult sage_ecs_##name##_remove(SageWorld* world, SageEntity entity) { \
        return component_remove(world, entity, pool_id); \
    } \
    const type* sage_ecs_##name##_get(const SageWorld* world, SageEntity entity) { \
        return (const type*)component_get(world, entity, pool_id); \
    } \
    SageResult sage_ecs_##name##_view(SageWorld* world, view_type* out_view) { \
        if (!out_view) return SAGE_ERR_ARGUMENT; \
        return component_view(world, pool_id, out_view, \
                             offsetof(view_type, entities), \
                             offsetof(view_type, components), \
                             offsetof(view_type, count)); \
    }

SAGE_DEFINE_COMPONENT_API(transform, SageTransform, SageTransformView,
                          SAGE_POOL_TRANSFORM)
SAGE_DEFINE_COMPONENT_API(render_binding, SageRenderBinding, SageRenderBindingView,
                          SAGE_POOL_RENDER_BINDING)
SAGE_DEFINE_COMPONENT_API(physics_body, SagePhysicsBody, SagePhysicsBodyView,
                          SAGE_POOL_PHYSICS_BODY)
SAGE_DEFINE_COMPONENT_API(audio_trigger, SageAudioTrigger, SageAudioTriggerView,
                          SAGE_POOL_AUDIO_TRIGGER)

#undef SAGE_DEFINE_COMPONENT_API

SageResult sage_ecs_system_register(SageWorld* world,
                                    SageSystemPhase phase,
                                    SageSystemFn function,
                                    void* user_data) {
    SageSystemList* list;
    SageSystemEntry* entries;
    uint32_t capacity;

    if (!world || !function || (unsigned)phase >= (unsigned)SAGE_PHASE_COUNT)
        return SAGE_ERR_ARGUMENT;
    if (world->stepping) return SAGE_ERR_BUSY;

    list = &world->systems[phase];
    if (list->count == list->capacity) {
        capacity = list->capacity ? list->capacity * 2u : 4u;
        if (capacity < list->capacity ||
            (size_t)capacity > ((size_t)-1) / sizeof(*entries))
            return SAGE_ERR_LIMIT;
        entries = (SageSystemEntry*)realloc(list->entries,
                                            (size_t)capacity * sizeof(*entries));
        if (!entries) return SAGE_ERR_NOMEM;
        list->entries = entries;
        list->capacity = capacity;
    }
    list->entries[list->count].function = function;
    list->entries[list->count].user_data = user_data;
    list->count++;
    return SAGE_OK;
}

SageResult sage_ecs_system_unregister(SageWorld* world,
                                      SageSystemPhase phase,
                                      SageSystemFn function,
                                      void* user_data) {
    SageSystemList* list;
    uint32_t i;

    if (!world || !function || (unsigned)phase >= (unsigned)SAGE_PHASE_COUNT)
        return SAGE_ERR_ARGUMENT;
    if (world->stepping) return SAGE_ERR_BUSY;

    list = &world->systems[phase];
    for (i = 0; i < list->count; ++i) {
        if (list->entries[i].function == function &&
            list->entries[i].user_data == user_data) {
            if (i + 1u < list->count) {
                memmove(&list->entries[i], &list->entries[i + 1u],
                        (size_t)(list->count - i - 1u) * sizeof(*list->entries));
            }
            list->count--;
            return SAGE_OK;
        }
    }
    return SAGE_ERR_NOT_FOUND;
}

static void run_phase(SageWorld* world,
                      SageSystemPhase phase,
                      float delta_seconds,
                      float interpolation) {
    SageSystemList* list = &world->systems[phase];
    SageSystemContext context;
    uint32_t i;

    context.phase = phase;
    context.delta_seconds = delta_seconds;
    context.interpolation = interpolation;
    context.frame_index = world->frame_index;
    context.simulation_tick = world->simulation_tick;
    for (i = 0; i < list->count; ++i)
        list->entries[i].function(world, &context, list->entries[i].user_data);
}

SageResult sage_ecs_advance(SageWorld* world, float frame_delta_seconds) {
    double fixed_step;
    double max_accumulated;
    double accepted_delta;
    uint32_t fixed_steps = 0;

    if (!world || !isfinite(frame_delta_seconds) || frame_delta_seconds < 0.0f)
        return SAGE_ERR_ARGUMENT;
    if (world->stepping) return SAGE_ERR_BUSY;

    world->stepping = 1;
    world->frame_index++;
    run_phase(world, SAGE_PHASE_INPUT, frame_delta_seconds, 0.0f);

    fixed_step = (double)world->fixed_step_seconds;
    max_accumulated = fixed_step * world->max_fixed_steps_per_frame;
    accepted_delta = (double)frame_delta_seconds;
    if (accepted_delta > max_accumulated) accepted_delta = max_accumulated;
    world->accumulator_seconds += accepted_delta;

    while (world->accumulator_seconds + fixed_step * 0.000001 >= fixed_step &&
           fixed_steps < world->max_fixed_steps_per_frame) {
        world->accumulator_seconds -= fixed_step;
        if (world->accumulator_seconds < 0.0) world->accumulator_seconds = 0.0;
        run_phase(world, SAGE_PHASE_FIXED_UPDATE,
                  world->fixed_step_seconds,
                  0.0f);
        world->simulation_tick++;
        fixed_steps++;
    }

    run_phase(world, SAGE_PHASE_EVENTS, frame_delta_seconds, 0.0f);
    run_phase(world, SAGE_PHASE_RENDER, frame_delta_seconds,
              (float)(world->accumulator_seconds / fixed_step));
    world->stepping = 0;
    return SAGE_OK;
}

uint64_t sage_ecs_frame_index(const SageWorld* world) {
    return world ? world->frame_index : 0;
}

uint64_t sage_ecs_simulation_tick(const SageWorld* world) {
    return world ? world->simulation_tick : 0;
}

float sage_ecs_interpolation(const SageWorld* world) {
    if (!world || world->fixed_step_seconds <= 0.0f) return 0.0f;
    return (float)(world->accumulator_seconds / world->fixed_step_seconds);
}

const char* sage_ecs_result_string(SageResult result) {
    switch (result) {
        case SAGE_OK: return "ok";
        case SAGE_ERR_ARGUMENT: return "invalid argument";
        case SAGE_ERR_LIMIT: return "configured limit reached";
        case SAGE_ERR_NOMEM: return "out of memory";
        case SAGE_ERR_INVALID_ENTITY: return "invalid or stale entity";
        case SAGE_ERR_NOT_FOUND: return "component not found";
        case SAGE_ERR_BUSY: return "world is advancing";
        case SAGE_ERR_RENDER: return "renderer failed";
        case SAGE_ERR_PHYSICS: return "physics operation failed";
        case SAGE_ERR_AUDIO: return "audio operation failed";
        default: return "unknown error";
    }
}
