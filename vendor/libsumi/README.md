# libsumi

libsumi is a lightweight graphics math library that aims for GLM-style ergonomics while staying friendly to CPU, CUDA, and shader-like code.

## Current Surface

- Float vectors: `vec2`, `vec3`, `vec4`
- Boolean vectors and relational helpers: `bvec2`, `bvec3`, `bvec4`, `all`, `any`, `equal`, `lessThan`, and related predicates
- Matrices: `mat2`, `mat3`, `mat4` with column-major storage, arithmetic, transpose, determinant, inverse, component multiplication, and outer products
- Common GLSL/GLM-style math: `radians`, `degrees`, `clamp`, `mix`, `step`, `smoothstep`, `fract`, `mod`, `round`, `trunc`, `inversesqrt`, `pow`, `atan`, `fma`, and vector overloads
- Geometry helpers: `dot`, `cross`, `length`, `distance`, `normalize`, `faceforward`, `reflect`, and `refract`
- Transform helpers: `translate`, `scale`, `rotate`, `perspective`, `frustum`, `ortho`, `lookAt`, plus right/left-handed aliases where available
- Quaternions: `quat`, Hamilton product, `angleAxis`, `mat3_cast`, `mat4_cast`, `toMat4`, `slerp`, and quaternion-vector rotation
- Texture and procedural helpers: `Sampler2D`, `texture`, `hash`, `noise`, and `fbm`

## Build And Test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
