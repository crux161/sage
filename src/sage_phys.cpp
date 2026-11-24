#include "sage/sage_phys.h"

#include "btBulletDynamicsCommon.h"

#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace {

struct BodyRecord {
    SageEntity entity;
    uint64_t handle;
    uint32_t flags;
    uint32_t material_tag;
    std::unique_ptr<btCollisionShape> shape;
    std::unique_ptr<btDefaultMotionState> motion_state;
    std::unique_ptr<btRigidBody> rigid_body;
};

bool finite_transform(const SageTransform& transform) {
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(transform.position_m[i]) ||
            !std::isfinite(transform.scale[i])) return false;
    }
    for (int i = 0; i < 4; ++i) {
        if (!std::isfinite(transform.rotation_xyzw[i])) return false;
    }
    return true;
}

bool valid_body_desc(const SagePhysicsBody& body) {
    if (!std::isfinite(body.mass_kg) || body.mass_kg < 0.0f) return false;
    if (!std::isfinite(body.friction) || body.friction < 0.0f || body.friction > 10.0f ||
        !std::isfinite(body.restitution) || body.restitution < 0.0f ||
        body.restitution > 1.0f) return false;
    if ((body.flags & SAGE_PHYSICS_BODY_STATIC) &&
        (body.flags & SAGE_PHYSICS_BODY_KINEMATIC)) return false;
    switch (body.shape) {
        case SAGE_PHYSICS_SHAPE_BOX:
            return std::isfinite(body.dimensions_m[0]) &&
                   std::isfinite(body.dimensions_m[1]) &&
                   std::isfinite(body.dimensions_m[2]) &&
                   body.dimensions_m[0] > 0.0f &&
                   body.dimensions_m[1] > 0.0f &&
                   body.dimensions_m[2] > 0.0f;
        case SAGE_PHYSICS_SHAPE_SPHERE:
            return std::isfinite(body.dimensions_m[0]) && body.dimensions_m[0] > 0.0f;
        case SAGE_PHYSICS_SHAPE_CAPSULE:
            return std::isfinite(body.dimensions_m[0]) &&
                   std::isfinite(body.dimensions_m[1]) &&
                   body.dimensions_m[0] > 0.0f && body.dimensions_m[1] >= 0.0f;
        default:
            return false;
    }
}

btTransform to_bullet_transform(const SageTransform& transform) {
    btQuaternion rotation(transform.rotation_xyzw[0],
                          transform.rotation_xyzw[1],
                          transform.rotation_xyzw[2],
                          transform.rotation_xyzw[3]);
    if (rotation.length2() < SIMD_EPSILON) {
        rotation.setValue(0, 0, 0, 1);
    } else {
        rotation.normalize();
    }
    return btTransform(rotation,
                       btVector3(transform.position_m[0],
                                 transform.position_m[1],
                                 transform.position_m[2]));
}

void from_bullet_transform(const btTransform& source, SageTransform* destination) {
    const btVector3& position = source.getOrigin();
    const btQuaternion& rotation = source.getRotation();
    destination->position_m[0] = (float)position.x();
    destination->position_m[1] = (float)position.y();
    destination->position_m[2] = (float)position.z();
    destination->rotation_xyzw[0] = (float)rotation.x();
    destination->rotation_xyzw[1] = (float)rotation.y();
    destination->rotation_xyzw[2] = (float)rotation.z();
    destination->rotation_xyzw[3] = (float)rotation.w();
}

SageTransform* find_transform(SageTransformView* view, SageEntity entity) {
    for (uint32_t i = 0; i < view->count; ++i) {
        if (view->entities[i] == entity) return &view->components[i];
    }
    return NULL;
}

void remove_bullet_body(SagePhysicsWorld* physics, BodyRecord* record);
void fixed_update_impl(SageWorld* world, const SageSystemContext* context, void* user_data);

} // namespace

struct SagePhysicsWorld {
    std::unique_ptr<btDefaultCollisionConfiguration> collision_configuration;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDbvtBroadphase> broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btDiscreteDynamicsWorld> dynamics_world;
    std::map<uint64_t, std::unique_ptr<BodyRecord> > bodies;
    std::vector<SageContactEvent> contacts;
    size_t contact_cursor;
    SageWorld* ecs_world;
    uint64_t next_body_handle;
    SageResult last_error;

    SagePhysicsWorld()
        : contact_cursor(0),
          ecs_world(NULL),
          next_body_handle(1),
          last_error(SAGE_OK) {}
};

namespace {

void remove_bullet_body(SagePhysicsWorld* physics, BodyRecord* record) {
    if (physics && record && physics->dynamics_world && record->rigid_body)
        physics->dynamics_world->removeRigidBody(record->rigid_body.get());
}

void collect_contacts(SagePhysicsWorld* physics) {
    const int manifold_count = physics->dispatcher->getNumManifolds();
    for (int manifold_index = 0; manifold_index < manifold_count; ++manifold_index) {
        btPersistentManifold* manifold =
            physics->dispatcher->getManifoldByIndexInternal(manifold_index);
        const btCollisionObject* object_a =
            static_cast<const btCollisionObject*>(manifold->getBody0());
        const btCollisionObject* object_b =
            static_cast<const btCollisionObject*>(manifold->getBody1());
        const BodyRecord* record_a =
            static_cast<const BodyRecord*>(object_a->getUserPointer());
        const BodyRecord* record_b =
            static_cast<const BodyRecord*>(object_b->getUserPointer());
        if (!record_a || !record_b) continue;

        const btRigidBody* body_a = btRigidBody::upcast(object_a);
        const btRigidBody* body_b = btRigidBody::upcast(object_b);
        for (int point_index = 0; point_index < manifold->getNumContacts(); ++point_index) {
            const btManifoldPoint& point = manifold->getContactPoint(point_index);
            if (point.getDistance() > 0.0f) continue;

            SageContactEvent event;
            const btVector3& point_a = point.getPositionWorldOnA();
            const btVector3& point_b = point.getPositionWorldOnB();
            const btVector3& normal = point.m_normalWorldOnB;
            btVector3 velocity_a(0, 0, 0);
            btVector3 velocity_b(0, 0, 0);
            if (body_a) {
                velocity_a = body_a->getVelocityInLocalPoint(
                    point_a - body_a->getWorldTransform().getOrigin());
            }
            if (body_b) {
                velocity_b = body_b->getVelocityInLocalPoint(
                    point_b - body_b->getWorldTransform().getOrigin());
            }

            event.entity_a = record_a->entity;
            event.entity_b = record_b->entity;
            event.point_a_m[0] = (float)point_a.x();
            event.point_a_m[1] = (float)point_a.y();
            event.point_a_m[2] = (float)point_a.z();
            event.point_b_m[0] = (float)point_b.x();
            event.point_b_m[1] = (float)point_b.y();
            event.point_b_m[2] = (float)point_b.z();
            event.normal_world_on_b[0] = (float)normal.x();
            event.normal_world_on_b[1] = (float)normal.y();
            event.normal_world_on_b[2] = (float)normal.z();
            event.relative_speed_mps = (float)(velocity_a - velocity_b).length();
            event.applied_impulse_ns = (float)point.getAppliedImpulse();
            event.separation_m = (float)point.getDistance();
            event.material_tag_a = record_a->material_tag;
            event.material_tag_b = record_b->material_tag;
            physics->contacts.push_back(event);
        }
    }
}

void fixed_update_impl(SageWorld* world, const SageSystemContext* context, void* user_data) {
    SagePhysicsWorld* physics = static_cast<SagePhysicsWorld*>(user_data);
    if (!physics || !context || physics->last_error != SAGE_OK) return;

    try {
        SageTransformView transforms;
        if (sage_ecs_transform_view(world, &transforms) != SAGE_OK) {
            physics->last_error = SAGE_ERR_PHYSICS;
            return;
        }

        for (std::map<uint64_t, std::unique_ptr<BodyRecord> >::iterator it =
                 physics->bodies.begin(); it != physics->bodies.end();) {
            BodyRecord* record = it->second.get();
            const SagePhysicsBody* component =
                sage_ecs_physics_body_get(world, record->entity);
            SageTransform* transform = find_transform(&transforms, record->entity);
            if (!sage_ecs_entity_alive(world, record->entity) || !component ||
                !transform || component->backend_handle != record->handle) {
                if (component && component->backend_handle == record->handle && !transform) {
                    SagePhysicsBody detached = *component;
                    detached.backend_handle = 0;
                    sage_ecs_physics_body_set(world, record->entity, &detached);
                }
                remove_bullet_body(physics, record);
                it = physics->bodies.erase(it);
                continue;
            }

            const bool dynamic = record->rigid_body->getInvMass() > 0.0f;
            if (!dynamic) {
                const btTransform bullet_transform = to_bullet_transform(*transform);
                record->rigid_body->setWorldTransform(bullet_transform);
                record->motion_state->setWorldTransform(bullet_transform);
                physics->dynamics_world->updateSingleAabb(record->rigid_body.get());
            }
            ++it;
        }

        physics->dynamics_world->stepSimulation(context->delta_seconds, 0,
                                                 context->delta_seconds);
        collect_contacts(physics);

        for (std::map<uint64_t, std::unique_ptr<BodyRecord> >::iterator it =
                 physics->bodies.begin(); it != physics->bodies.end(); ++it) {
            BodyRecord* record = it->second.get();
            if (record->rigid_body->getInvMass() <= 0.0f) continue;
            SageTransform* transform = find_transform(&transforms, record->entity);
            if (!transform) continue;
            btTransform bullet_transform;
            record->motion_state->getWorldTransform(bullet_transform);
            from_bullet_transform(bullet_transform, transform);
        }
    } catch (const std::bad_alloc&) {
        physics->last_error = SAGE_ERR_NOMEM;
    } catch (...) {
        physics->last_error = SAGE_ERR_PHYSICS;
    }
}

} // namespace

extern "C" void sage_phys_fixed_system(SageWorld* world,
                                       const SageSystemContext* context,
                                       void* user_data) {
    fixed_update_impl(world, context, user_data);
}

extern "C" SagePhysicsConfig sage_phys_config_default(void) {
    SagePhysicsConfig config;
    config.gravity_mps2[0] = 0.0f;
    config.gravity_mps2[1] = -9.81f;
    config.gravity_mps2[2] = 0.0f;
    config.solver_iterations = 10;
    return config;
}

extern "C" SagePhysicsWorld* sage_phys_world_create(const SagePhysicsConfig* input_config) {
    SagePhysicsConfig config = input_config ? *input_config : sage_phys_config_default();
    SagePhysicsWorld* physics = NULL;
    if (config.solver_iterations == 0) config.solver_iterations = 10;
    if (config.solver_iterations > 128 ||
        !std::isfinite(config.gravity_mps2[0]) ||
        !std::isfinite(config.gravity_mps2[1]) ||
        !std::isfinite(config.gravity_mps2[2])) return NULL;

    try {
        physics = new (std::nothrow) SagePhysicsWorld();
    } catch (...) {
        return NULL;
    }
    if (!physics) return NULL;
    try {
        physics->collision_configuration.reset(new btDefaultCollisionConfiguration());
        physics->dispatcher.reset(new btCollisionDispatcher(physics->collision_configuration.get()));
        physics->broadphase.reset(new btDbvtBroadphase());
        physics->solver.reset(new btSequentialImpulseConstraintSolver());
        physics->dynamics_world.reset(new btDiscreteDynamicsWorld(
            physics->dispatcher.get(), physics->broadphase.get(),
            physics->solver.get(), physics->collision_configuration.get()));
        physics->dynamics_world->setGravity(btVector3(config.gravity_mps2[0],
                                                       config.gravity_mps2[1],
                                                       config.gravity_mps2[2]));
        physics->dynamics_world->getSolverInfo().m_numIterations =
            (int)config.solver_iterations;
    } catch (...) {
        delete physics;
        return NULL;
    }
    return physics;
}

extern "C" SageResult sage_phys_world_bind(SagePhysicsWorld* physics, SageWorld* world) {
    SageResult result;
    if (!physics || !world) return SAGE_ERR_ARGUMENT;
    if (physics->ecs_world == world) return SAGE_OK;
    if (physics->ecs_world || !physics->bodies.empty()) return SAGE_ERR_BUSY;
    result = sage_ecs_system_register(world, SAGE_PHASE_FIXED_UPDATE,
                                      sage_phys_fixed_system, physics);
    if (result != SAGE_OK) return result;
    physics->ecs_world = world;
    physics->last_error = SAGE_OK;
    return SAGE_OK;
}

extern "C" SageResult sage_phys_world_unbind(SagePhysicsWorld* physics) {
    SageResult result;
    if (!physics) return SAGE_ERR_ARGUMENT;
    if (!physics->ecs_world) return SAGE_OK;
    if (!physics->bodies.empty()) return SAGE_ERR_BUSY;
    result = sage_ecs_system_unregister(physics->ecs_world, SAGE_PHASE_FIXED_UPDATE,
                                        sage_phys_fixed_system, physics);
    if (result != SAGE_OK && result != SAGE_ERR_NOT_FOUND) return result;
    physics->ecs_world = NULL;
    return SAGE_OK;
}

extern "C" SageResult sage_phys_world_destroy(SagePhysicsWorld* physics) {
    if (!physics) return SAGE_OK;
    if (physics->ecs_world && sage_ecs_world_is_advancing(physics->ecs_world))
        return SAGE_ERR_BUSY;

    if (physics->ecs_world) {
        sage_ecs_system_unregister(physics->ecs_world, SAGE_PHASE_FIXED_UPDATE,
                                   sage_phys_fixed_system, physics);
        for (std::map<uint64_t, std::unique_ptr<BodyRecord> >::iterator it =
                 physics->bodies.begin(); it != physics->bodies.end(); ++it) {
            BodyRecord* record = it->second.get();
            const SagePhysicsBody* component =
                sage_ecs_physics_body_get(physics->ecs_world, record->entity);
            if (component && component->backend_handle == record->handle) {
                SagePhysicsBody updated = *component;
                updated.backend_handle = 0;
                sage_ecs_physics_body_set(physics->ecs_world, record->entity, &updated);
            }
            remove_bullet_body(physics, record);
        }
    }
    delete physics;
    return SAGE_OK;
}

extern "C" SageResult sage_phys_body_add(SagePhysicsWorld* physics, SageEntity entity) {
    if (!physics || !physics->ecs_world) return SAGE_ERR_ARGUMENT;
    if (sage_ecs_world_is_advancing(physics->ecs_world)) return SAGE_ERR_BUSY;
    if (!sage_ecs_entity_alive(physics->ecs_world, entity)) return SAGE_ERR_INVALID_ENTITY;

    const SagePhysicsBody* body = sage_ecs_physics_body_get(physics->ecs_world, entity);
    const SageTransform* transform = sage_ecs_transform_get(physics->ecs_world, entity);
    if (!body || !transform) return SAGE_ERR_NOT_FOUND;
    if (body->backend_handle != 0 || !valid_body_desc(*body) || !finite_transform(*transform))
        return SAGE_ERR_ARGUMENT;
    if (physics->next_body_handle == 0) return SAGE_ERR_LIMIT;

    try {
        std::unique_ptr<BodyRecord> record(new BodyRecord());
        record->entity = entity;
        record->handle = physics->next_body_handle++;
        record->flags = body->flags;
        record->material_tag = body->material_tag;

        switch (body->shape) {
            case SAGE_PHYSICS_SHAPE_BOX:
                record->shape.reset(new btBoxShape(btVector3(body->dimensions_m[0],
                                                              body->dimensions_m[1],
                                                              body->dimensions_m[2])));
                break;
            case SAGE_PHYSICS_SHAPE_SPHERE:
                record->shape.reset(new btSphereShape(body->dimensions_m[0]));
                break;
            case SAGE_PHYSICS_SHAPE_CAPSULE:
                record->shape.reset(new btCapsuleShape(body->dimensions_m[0],
                                                        body->dimensions_m[1] * 2.0f));
                break;
            default:
                return SAGE_ERR_ARGUMENT;
        }

        const bool kinematic = (body->flags & SAGE_PHYSICS_BODY_KINEMATIC) != 0;
        const bool is_static = (body->flags & SAGE_PHYSICS_BODY_STATIC) != 0 ||
                               body->mass_kg <= 0.0f;
        const btScalar mass = (kinematic || is_static) ? 0.0f : body->mass_kg;
        const btTransform initial_transform = to_bullet_transform(*transform);
        record->motion_state.reset(new btDefaultMotionState(initial_transform));
        btVector3 inertia(0, 0, 0);
        if (mass > 0.0f) record->shape->calculateLocalInertia(mass, inertia);
        btRigidBody::btRigidBodyConstructionInfo construction(
            mass, record->motion_state.get(), record->shape.get(), inertia);
        record->rigid_body.reset(new btRigidBody(construction));
        record->rigid_body->setFriction(body->friction);
        record->rigid_body->setRestitution(body->restitution);
        if (kinematic) {
            record->rigid_body->setCollisionFlags(
                record->rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
            record->rigid_body->setActivationState(DISABLE_DEACTIVATION);
        }
        if (body->flags & SAGE_PHYSICS_BODY_TRIGGER) {
            record->rigid_body->setCollisionFlags(
                record->rigid_body->getCollisionFlags() |
                btCollisionObject::CF_NO_CONTACT_RESPONSE);
        }

        BodyRecord* raw_record = record.get();
        const uint64_t handle = record->handle;
        physics->bodies.insert(std::make_pair(handle, std::move(record)));
        raw_record->rigid_body->setUserPointer(raw_record);
        physics->dynamics_world->addRigidBody(raw_record->rigid_body.get());

        SagePhysicsBody updated = *body;
        updated.backend_handle = handle;
        const SageResult result = sage_ecs_physics_body_set(
            physics->ecs_world, entity, &updated);
        if (result != SAGE_OK) {
            remove_bullet_body(physics, raw_record);
            physics->bodies.erase(handle);
            return result;
        }
        return SAGE_OK;
    } catch (const std::bad_alloc&) {
        return SAGE_ERR_NOMEM;
    } catch (...) {
        return SAGE_ERR_PHYSICS;
    }
}

extern "C" SageResult sage_phys_body_remove(SagePhysicsWorld* physics, SageEntity entity) {
    if (!physics || !physics->ecs_world) return SAGE_ERR_ARGUMENT;
    if (sage_ecs_world_is_advancing(physics->ecs_world)) return SAGE_ERR_BUSY;
    if (!sage_ecs_entity_alive(physics->ecs_world, entity)) return SAGE_ERR_INVALID_ENTITY;

    const SagePhysicsBody* body = sage_ecs_physics_body_get(physics->ecs_world, entity);
    if (!body || body->backend_handle == 0) return SAGE_ERR_NOT_FOUND;
    std::map<uint64_t, std::unique_ptr<BodyRecord> >::iterator found =
        physics->bodies.find(body->backend_handle);
    if (found == physics->bodies.end() || found->second->entity != entity)
        return SAGE_ERR_PHYSICS;

    remove_bullet_body(physics, found->second.get());
    physics->bodies.erase(found);
    SagePhysicsBody updated = *body;
    updated.backend_handle = 0;
    return sage_ecs_physics_body_set(physics->ecs_world, entity, &updated);
}

extern "C" SageResult sage_phys_body_set_velocity(
    SagePhysicsWorld* physics,
    SageEntity entity,
    const float linear_velocity_mps[3],
    const float angular_velocity_rps[3]) {
    if (!physics || !physics->ecs_world || !linear_velocity_mps || !angular_velocity_rps)
        return SAGE_ERR_ARGUMENT;
    if (sage_ecs_world_is_advancing(physics->ecs_world)) return SAGE_ERR_BUSY;
    if (!sage_ecs_entity_alive(physics->ecs_world, entity)) return SAGE_ERR_INVALID_ENTITY;
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(linear_velocity_mps[i]) ||
            !std::isfinite(angular_velocity_rps[i])) return SAGE_ERR_ARGUMENT;
    }

    const SagePhysicsBody* component = sage_ecs_physics_body_get(physics->ecs_world, entity);
    if (!component || !component->backend_handle) return SAGE_ERR_NOT_FOUND;
    std::map<uint64_t, std::unique_ptr<BodyRecord> >::iterator found =
        physics->bodies.find(component->backend_handle);
    if (found == physics->bodies.end() || found->second->entity != entity)
        return SAGE_ERR_PHYSICS;
    btRigidBody* body = found->second->rigid_body.get();
    if (!body || body->getInvMass() <= 0.0f) return SAGE_ERR_ARGUMENT;
    body->setLinearVelocity(btVector3(linear_velocity_mps[0],
                                      linear_velocity_mps[1],
                                      linear_velocity_mps[2]));
    body->setAngularVelocity(btVector3(angular_velocity_rps[0],
                                       angular_velocity_rps[1],
                                       angular_velocity_rps[2]));
    body->activate(true);
    return SAGE_OK;
}

extern "C" int32_t sage_phys_contacts_poll(SagePhysicsWorld* physics,
                                           SageContactEvent* output,
                                           int32_t capacity) {
    if (!physics || capacity < 0 || (capacity > 0 && !output)) return -1;
    const size_t available = physics->contacts.size() - physics->contact_cursor;
    const size_t count = available < (size_t)capacity ? available : (size_t)capacity;
    if (count) {
        std::memcpy(output, &physics->contacts[physics->contact_cursor],
                    count * sizeof(*output));
        physics->contact_cursor += count;
    }
    if (physics->contact_cursor == physics->contacts.size()) {
        physics->contacts.clear();
        physics->contact_cursor = 0;
    }
    return (int32_t)count;
}

extern "C" SageResult sage_phys_raycast(const SagePhysicsWorld* physics,
                                         const float from_m[3],
                                         const float to_m[3],
                                         SageRayHit* out_hit) {
    if (!physics || !physics->dynamics_world || !from_m || !to_m || !out_hit)
        return SAGE_ERR_ARGUMENT;
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(from_m[i]) || !std::isfinite(to_m[i]))
            return SAGE_ERR_ARGUMENT;
    }

    const btVector3 from(from_m[0], from_m[1], from_m[2]);
    const btVector3 to(to_m[0], to_m[1], to_m[2]);
    btCollisionWorld::ClosestRayResultCallback callback(from, to);
    physics->dynamics_world->rayTest(from, to, callback);
    if (!callback.hasHit()) return SAGE_ERR_NOT_FOUND;

    const BodyRecord* record =
        static_cast<const BodyRecord*>(callback.m_collisionObject->getUserPointer());
    if (!record) return SAGE_ERR_PHYSICS;
    out_hit->entity = record->entity;
    out_hit->point_m[0] = (float)callback.m_hitPointWorld.x();
    out_hit->point_m[1] = (float)callback.m_hitPointWorld.y();
    out_hit->point_m[2] = (float)callback.m_hitPointWorld.z();
    out_hit->normal[0] = (float)callback.m_hitNormalWorld.x();
    out_hit->normal[1] = (float)callback.m_hitNormalWorld.y();
    out_hit->normal[2] = (float)callback.m_hitNormalWorld.z();
    out_hit->fraction = (float)callback.m_closestHitFraction;
    return SAGE_OK;
}

extern "C" SageResult sage_phys_last_error(const SagePhysicsWorld* physics) {
    return physics ? physics->last_error : SAGE_ERR_ARGUMENT;
}

