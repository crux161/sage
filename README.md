# SAGE

SAGE (Sumi Advanced Game Engine) is an early-stage C/C++ game engine built from
Eshi's shader and rendering engine, with a small data-oriented core, scoped
Bullet3 physics, and adaptive audio. The architecture and direction come from
[SEED.pdf](SEED.pdf).

## Current setup

- `vendor/eshi/` contains the Eshi source snapshot used as the rendering base.
- `vendor/libsumi/` contains the math library required by Eshi.
- `vendor/bullet3/` contains only Bullet3's `LinearMath`, `BulletCollision`,
  and `BulletDynamics` modules and their common umbrella headers.
- `vendor/README.md` records upstream revisions, license names, and the Bullet
  subset boundary.

The first SAGE core slice is in [sage_ecs.h](include/sage/sage_ecs.h) and
[sage_ecs.c](src/sage_ecs.c), built as the C99 `sage_ecs` target. It defines
meters, seconds, radians, a right-handed +Y-up world, generational entity
handles, and dense pools for transforms, render bindings, physics bodies, and
audio triggers. The scheduler runs input, zero or more fixed updates, events,
and rendering in that order; systems within a phase run in registration order.

`sage_renderer` bridges SAGE scene views into Eshi's C material callback and
renders through Eshi's CPU Ink backend. The optional SDL3 reference host is in
`examples/sdl3_demo.cpp`; it owns events and presentation, advances SAGE, then
passes transforms to Eshi for each frame. Eshi's existing host remains SDL2.

`sage_audio` is a C mixer for caller-owned float PCM clips. It supports a shared
music transport for looping stems, parameter-linked stem gains, bus fades,
one-shot named events, ECS audio-trigger helpers, and material-tag contact
profiles with cooldowns. `sage_audio_sdl3` feeds the mixer to an SDL3 playback
stream; lock the device around game-thread changes to mixer state while its
callback is running.

The SDL3 reference scene uses Bullet for the ball, paddle, and arena walls, then
routes contact impulses through those profiles to short impact sounds. W/S
moves the paddle; Escape exits.

Build the subsystems and SDL3 reference targets with:

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="$(brew --prefix sdl3)" \
  -DSAGE_BUILD_SDL3_DEMO=ON \
  -DSAGE_BUILD_SDL3_AUDIO=ON
cmake --build build --target sage_renderer sage_phys sage_audio_sdl3 sage_sdl3_demo
```

For a build without SDL3, leave both SDL3 options off and build the core targets:

```sh
cmake -S . -B build
cmake --build build --target sage_renderer sage_phys sage_audio
```

## Roadmap

1. **Foundation and ECS — complete.** Shared units, C-friendly dense component
   pools, fixed phase ordering, an Eshi material bridge, and an SDL3 reference
   host are in place. The host owns the window and frame loop while SAGE owns
   scene state.
2. **Scoped Bullet3 physics — first slice complete.** `sage_phys` uses only the
   vendored `LinearMath`, `BulletCollision`, and `BulletDynamics` modules. It
   supports primitive rigid bodies, fixed-step transform sync, contact points,
   normals, relative speeds, material tags, applied impulses, and raycasts.
   Constraints, shape queries, and additional Bullet modules can be added when
   a SAGE feature needs them.
3. **Adaptive audio and physics feedback — in progress.** The mixer, SDL3
   playback adapter, ECS audio-trigger helpers, and a Bullet contact-to-sound
   reference scene are in place. The mixer supports synchronized stems,
   smoothed parameters and buses, named one-shot events, and material-tag
   contact profiles driven by relative speed or impulse. Next, add a small
   runtime audio asset loader and drive stem parameters from gameplay state.
4. **Volumetric sky and reference sandbox — planned.** Add the cloud pass with
   multi-octave noise, wind advection, raymarched lighting, fractional
   resolution, and temporal reprojection; then grow the demo into a reference
   sandbox that combines physics, audio, and atmosphere.

Subsystem headers keep Bullet, Eshi, and SDL implementation types behind their
boundaries so game code can use plain C-compatible data.
