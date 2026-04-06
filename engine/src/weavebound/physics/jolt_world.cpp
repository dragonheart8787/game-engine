#include "weavebound/physics/jolt_world.hpp"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>
#include <cstdio>
#include <memory>
#include <thread>

JPH_SUPPRESS_WARNINGS

using namespace JPH;

namespace weavebound::physics::jolt {
namespace {

#ifdef JPH_ENABLE_ASSERTS
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine) {
  std::fprintf(stderr, "%s:%u: (%s) %s\n", inFile, static_cast<unsigned>(inLine), inExpression,
               inMessage ? inMessage : "");
  return true;
}
#endif

static void TraceImpl(const char* inFMT, ...) {
  va_list list;
  va_start(list, inFMT);
  std::vfprintf(stderr, inFMT, list);
  va_end(list);
  std::fprintf(stderr, "\n");
}

namespace Layers {
static constexpr ObjectLayer NON_MOVING = 0;
static constexpr ObjectLayer MOVING = 1;
static constexpr ObjectLayer NUM_LAYERS = 2;
}  // namespace Layers

class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
 public:
  bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
    switch (inObject1) {
      case Layers::NON_MOVING:
        return inObject2 == Layers::MOVING;
      case Layers::MOVING:
        return true;
      default:
        JPH_ASSERT(false);
        return false;
    }
  }
};

namespace BroadPhaseLayers {
static constexpr BroadPhaseLayer NON_MOVING(0);
static constexpr BroadPhaseLayer MOVING(1);
static constexpr JPH::uint NUM_LAYERS(2);
}  // namespace BroadPhaseLayers

class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
 public:
  BPLayerInterfaceImpl() {
    mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
    mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
  }

  JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

  BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
    JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
    return mObjectToBroadPhase[inLayer];
  }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
    switch ((BroadPhaseLayer::Type)inLayer) {
      case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
        return "NON_MOVING";
      case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
        return "MOVING";
      default:
        JPH_ASSERT(false);
        return "INVALID";
    }
  }
#endif

 private:
  BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS]{};
};

class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
 public:
  bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
    switch (inLayer1) {
      case Layers::NON_MOVING:
        return inLayer2 == BroadPhaseLayers::MOVING;
      case Layers::MOVING:
        return true;
      default:
        JPH_ASSERT(false);
        return false;
    }
  }
};

bool g_inited = false;
BPLayerInterfaceImpl g_broad_phase_layer_interface;
ObjectVsBroadPhaseLayerFilterImpl g_object_vs_broadphase_layer_filter;
ObjectLayerPairFilterImpl g_object_vs_object_layer_filter;
PhysicsSystem g_physics_system;
std::unique_ptr<TempAllocatorImpl> g_temp_allocator;
std::unique_ptr<JobSystemThreadPool> g_job_system;
BodyID g_floor_id;
BodyID g_sphere_id;
bool g_have_floor = false;
bool g_have_sphere = false;

}  // namespace

bool init() {
  if (g_inited) {
    return true;
  }

  RegisterDefaultAllocator();
  Trace = TraceImpl;
  JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

  Factory::sInstance = new Factory();
  RegisterTypes();

  g_temp_allocator = std::make_unique<TempAllocatorImpl>(10 * 1024 * 1024);
  const int threads = static_cast<int>(std::thread::hardware_concurrency());
  const int workers = threads > 1 ? threads - 1 : 1;
  g_job_system = std::make_unique<JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, workers);

  const JPH::uint cMaxBodies = 1024;
  const JPH::uint cNumBodyMutexes = 0;
  const JPH::uint cMaxBodyPairs = 1024;
  const JPH::uint cMaxContactConstraints = 1024;

  g_physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, g_broad_phase_layer_interface,
                        g_object_vs_broadphase_layer_filter, g_object_vs_object_layer_filter);

  BodyInterface& body_interface = g_physics_system.GetBodyInterface();

  BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));
  floor_shape_settings.SetEmbedded();
  ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
  if (floor_shape_result.HasError()) {
    return false;
  }
  ShapeRefC floor_shape = floor_shape_result.Get();
  BodyCreationSettings floor_settings(floor_shape, RVec3(0.0f, -1.0f, 0.0f), Quat::sIdentity(), EMotionType::Static,
                                      Layers::NON_MOVING);
  Body* floor = body_interface.CreateBody(floor_settings);
  if (!floor) {
    return false;
  }
  g_floor_id = floor->GetID();
  body_interface.AddBody(g_floor_id, EActivation::DontActivate);
  g_have_floor = true;

  BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0f, 2.0f, 0.0f), Quat::sIdentity(),
                                       EMotionType::Dynamic, Layers::MOVING);
  g_sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);
  g_have_sphere = true;

  g_physics_system.OptimizeBroadPhase();

  g_inited = true;
  return true;
}

void shutdown() {
  if (!g_inited) {
    return;
  }
  BodyInterface& body_interface = g_physics_system.GetBodyInterface();
  if (g_have_sphere) {
    body_interface.RemoveBody(g_sphere_id);
    body_interface.DestroyBody(g_sphere_id);
    g_have_sphere = false;
  }
  if (g_have_floor) {
    body_interface.RemoveBody(g_floor_id);
    body_interface.DestroyBody(g_floor_id);
    g_have_floor = false;
  }

  UnregisterTypes();
  delete Factory::sInstance;
  Factory::sInstance = nullptr;

  g_job_system.reset();
  g_temp_allocator.reset();
  g_inited = false;
}

bool step(float dt_seconds) {
  if (!g_inited || !g_temp_allocator || !g_job_system) {
    return false;
  }
  g_physics_system.Update(dt_seconds, 1, g_temp_allocator.get(), g_job_system.get());
  return true;
}

bool raycast_down_hit_plane() {
  if (!g_inited) {
    return false;
  }
  RayCastResult hit{};
  // mDirection 為「方向 × 射線長度」，單位向量長度 1 時只掃 1m，須加長才能碰到地板。
  const RRayCast ray(RVec3(0.0f, 8.0f, 0.0f), Vec3(0.0f, -32.0f, 0.0f));
  return g_physics_system.GetNarrowPhaseQuery().CastRay(ray, hit);
}

}  // namespace weavebound::physics::jolt
