# Vendored sources

These source snapshots are copied into SAGE without upstream Git history. Keep
the upstream license files with their code and update the revision below when
refreshing a snapshot.

| Directory | Upstream | Revision | License |
| --- | --- | --- | --- |
| `eshi/` | <https://github.com/crux161/eshi> | `aefcb49d8c3c601f959ffc9b9e5891749644d35d` | MIT |
| `libsumi/` | <https://github.com/crux161/libsumi> | `717a8dcab5bb1786984c80a1d7934fb291908216` | MIT |
| `bullet3/` | <https://github.com/bulletphysics/bullet3> | `d3afe76132fc76b0f9b2359cbaef47a3766e7fbb` (3.27) | zlib |

The Bullet3 snapshot is intentionally limited to `src/LinearMath`,
`src/BulletCollision`, `src/BulletDynamics`, and the `btBulletCollisionCommon.h`
and `btBulletDynamicsCommon.h` umbrella headers. Bullet examples, demos,
extras, soft-body code, and other modules are not included.

Eshi expects libsumi at a relative path when building. From `vendor/eshi`, the
top-level `vendor/libsumi` checkout is `../libsumi`; the upstream build supports
passing this path with `-Dsumi-path=../libsumi` for Zig builds or
`SUMI_PATH=../libsumi` for Make builds.
