/*
 * SAGE's C-compatible world, component pools, and deterministic frame phases.
 * A world is single-thread owned; borrowed component pointers and views are
 * valid only until a structural change to the corresponding component pool.
 *
 * Units: positions and dimensions are meters; time is seconds; angles are
 * radians; mass is kilograms. The world is right-handed with +Y up. Quaternions
 * use x, y, z, w order and scales are dimensionless.
 */
#ifndef SAGE_ECS_H
#define SAGE_ECS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t SageEntity;

#define SAGE_NULL_ENTITY ((SageEntity)0)
#define SAGE_ENTITY_INDEX_BITS 20u
#define SAGE_ENTITY_INDEX_MASK 0x000FFFFFu
#define SAGE_ENTITY_GENERATION_MASK 0x00000FFFu
#define SAGE_MAX_ENTITIES SAGE_ENTITY_INDEX_MASK
#define SAGE_ENTITY_INDEX(entity) \
    ((uint32_t)((entity) & SAGE_ENTITY_INDEX_MASK))
#define SAGE_ENTITY_GENERATION(entity) \
    ((uint32_t)(((entity) >> SAGE_ENTITY_INDEX_BITS) & SAGE_ENTITY_GENERATION_MASK))

typedef enum SageResult {
    SAGE_OK = 0,
    SAGE_ERR_ARGUMENT = -1,
    SAGE_ERR_LIMIT = -2,
    SAGE_ERR_NOMEM = -3,
    SAGE_ERR_INVALID_ENTITY = -4,
    SAGE_ERR_NOT_FOUND = -5,
    SAGE_ERR_BUSY = -6,
    SAGE_ERR_RENDER = -7,
    SAGE_ERR_PHYSICS = -8,
    SAGE_ERR_AUDIO = -9
} SageResult;

typedef struct SageWorld SageWorld;

typedef struct SageWorldConfig {
    uint32_t max_entities; /* 0 selects 4096; maximum is SAGE_MAX_ENTITIES. */
    float fixed_step_seconds; /* 0 selects 1/60 second. */
    uint32_t max_fixed_steps_per_frame; /* 0 selects 8; maximum is 64. */
} SageWorldConfig;

SageWorldConfig sage_ecs_config_default(void);
SageWorld* sage_ecs_world_create(const SageWorldConfig* config);
void sage_ecs_world_destroy(SageWorld* world);

/* Returns SAGE_NULL_ENTITY if the world is full or is currently advancing. */
SageEntity sage_ecs_entity_create(SageWorld* world);
SageResult sage_ecs_entity_destroy(SageWorld* world, SageEntity entity);
int sage_ecs_entity_alive(const SageWorld* world, SageEntity entity);
uint32_t sage_ecs_entity_count(const SageWorld* world);
int sage_ecs_world_is_advancing(const SageWorld* world);

typedef struct SageTransform {
    float position_m[3];
    float rotation_xyzw[4];
    float scale[3];
} SageTransform;

void sage_transform_identity(SageTransform* transform);

typedef struct SageRenderBinding {
    uint32_t shader_id;
    uint32_t material_id;
    uint32_t flags;
} SageRenderBinding;

typedef enum SagePhysicsShape {
    SAGE_PHYSICS_SHAPE_BOX = 1,
    SAGE_PHYSICS_SHAPE_SPHERE = 2,
    SAGE_PHYSICS_SHAPE_CAPSULE = 3
} SagePhysicsShape;

typedef enum SagePhysicsBodyFlags {
    SAGE_PHYSICS_BODY_STATIC = 1u << 0,
    SAGE_PHYSICS_BODY_KINEMATIC = 1u << 1,
    SAGE_PHYSICS_BODY_TRIGGER = 1u << 2
} SagePhysicsBodyFlags;

typedef struct SagePhysicsBody {
    SagePhysicsShape shape;
    uint32_t flags;
    uint32_t material_tag;
    float mass_kg;
    float friction; /* Non-negative friction coefficient. */
    float restitution; /* Bounciness in [0, 1]. */
    /* Box half-extents; sphere radius in x; capsule radius in x and half-height in y. */
    /* Physics dimensions are explicit and do not inherit transform scale. */
    float dimensions_m[3];
    /* Opaque adapter handle. SAGE headers do not expose Bullet types. */
    uint64_t backend_handle;
} SagePhysicsBody;

typedef struct SageAudioTrigger {
    uint32_t event_id;
    uint32_t flags;
    float gain;
    float pitch;
} SageAudioTrigger;

/* Views are borrowed dense arrays. Structural changes invalidate their pointers. */
typedef struct SageTransformView {
    const SageEntity* entities;
    SageTransform* components;
    uint32_t count;
} SageTransformView;

typedef struct SageRenderBindingView {
    const SageEntity* entities;
    SageRenderBinding* components;
    uint32_t count;
} SageRenderBindingView;

typedef struct SagePhysicsBodyView {
    const SageEntity* entities;
    SagePhysicsBody* components;
    uint32_t count;
} SagePhysicsBodyView;

typedef struct SageAudioTriggerView {
    const SageEntity* entities;
    SageAudioTrigger* components;
    uint32_t count;
} SageAudioTriggerView;

/* Getters return borrowed read-only pointers; views allow dense iteration. */
#define SAGE_DECLARE_COMPONENT_API(name, type, view_type) \
    SageResult sage_ecs_##name##_set(SageWorld* world, SageEntity entity, \
                                     const type* component); \
    SageResult sage_ecs_##name##_remove(SageWorld* world, SageEntity entity); \
    const type* sage_ecs_##name##_get(const SageWorld* world, SageEntity entity); \
    SageResult sage_ecs_##name##_view(SageWorld* world, view_type* out_view)

SAGE_DECLARE_COMPONENT_API(transform, SageTransform, SageTransformView);
SAGE_DECLARE_COMPONENT_API(render_binding, SageRenderBinding, SageRenderBindingView);
SAGE_DECLARE_COMPONENT_API(physics_body, SagePhysicsBody, SagePhysicsBodyView);
SAGE_DECLARE_COMPONENT_API(audio_trigger, SageAudioTrigger, SageAudioTriggerView);

#undef SAGE_DECLARE_COMPONENT_API

typedef enum SageSystemPhase {
    SAGE_PHASE_INPUT = 0,
    SAGE_PHASE_FIXED_UPDATE = 1,
    SAGE_PHASE_EVENTS = 2,
    SAGE_PHASE_RENDER = 3,
    SAGE_PHASE_COUNT = 4
} SageSystemPhase;

typedef struct SageSystemContext {
    SageSystemPhase phase;
    float delta_seconds;
    float interpolation;
    uint64_t frame_index;
    uint64_t simulation_tick;
} SageSystemContext;

typedef void (*SageSystemFn)(SageWorld* world,
                             const SageSystemContext* context,
                             void* user_data);

/* Systems run in registration order within each phase. */
SageResult sage_ecs_system_register(SageWorld* world,
                                   SageSystemPhase phase,
                                   SageSystemFn function,
                                   void* user_data);
SageResult sage_ecs_system_unregister(SageWorld* world,
                                     SageSystemPhase phase,
                                     SageSystemFn function,
                                     void* user_data);

/*
 * Runs input once, fixed update zero or more times, then events and render
 * once. Frame-level phases receive frame_delta_seconds; fixed update receives
 * the configured fixed step. Excess catch-up time beyond the configured step
 * limit is discarded to avoid an unbounded spiral of updates. Structural
 * changes and system registration from inside callbacks return SAGE_ERR_BUSY;
 * update existing component values directly or queue structural changes for
 * the next frame.
 */
SageResult sage_ecs_advance(SageWorld* world, float frame_delta_seconds);
uint64_t sage_ecs_frame_index(const SageWorld* world);
uint64_t sage_ecs_simulation_tick(const SageWorld* world);
float sage_ecs_interpolation(const SageWorld* world);

const char* sage_ecs_result_string(SageResult result);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SAGE_ECS_H */
