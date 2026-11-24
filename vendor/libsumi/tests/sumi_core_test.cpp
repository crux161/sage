#include <cassert>
#include <cmath>

#include <sumi/sumi.h>
#include <sumi/sumi_engine.h>
#include <sumi/sumi_noise.h>

namespace {

bool near(float a, float b, float eps = 1.0e-4f) {
    return std::fabs(a - b) <= eps;
}

void expect_vec2(const sumi::vec2& v, const sumi::vec2& expected, float eps = 1.0e-4f) {
    assert(near(v.x, expected.x, eps));
    assert(near(v.y, expected.y, eps));
}

void expect_vec3(const sumi::vec3& v, const sumi::vec3& expected, float eps = 1.0e-4f) {
    assert(near(v.x, expected.x, eps));
    assert(near(v.y, expected.y, eps));
    assert(near(v.z, expected.z, eps));
}

void expect_vec4(const sumi::vec4& v, const sumi::vec4& expected, float eps = 1.0e-4f) {
    assert(near(v.x, expected.x, eps));
    assert(near(v.y, expected.y, eps));
    assert(near(v.z, expected.z, eps));
    assert(near(v.w, expected.w, eps));
}

void expect_mat4_identity(const sumi::mat4& m, float eps = 1.0e-3f) {
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float expected = col == row ? 1.0f : 0.0f;
            assert(near(m[col][row], expected, eps));
        }
    }
}

} // namespace

int main() {
    using namespace sumi;

    expect_vec2(clamp(vec2(-1.0f, 2.0f), vec2(0.0f), vec2(1.0f)), vec2(0.0f, 1.0f));
    expect_vec3(mix(vec3(0.0f), vec3(10.0f), vec3(0.0f, 0.5f, 1.0f)), vec3(0.0f, 5.0f, 10.0f));
    expect_vec4(step(vec4(0.0f, 1.0f, 2.0f, 3.0f), vec4(0.5f, 0.5f, 2.0f, 4.0f)), vec4(1.0f, 0.0f, 1.0f, 1.0f));
    assert(all(lessThan(vec3(1.0f, 2.0f, 3.0f), vec3(2.0f, 3.0f, 4.0f))));
    assert(any(notEqual(vec2(1.0f, 2.0f), vec2(1.0f, 3.0f))));

    expect_vec2(reflect(vec2(1.0f, -1.0f), vec2(0.0f, 1.0f)), vec2(1.0f, 1.0f));
    expect_vec3(faceforward(vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)), vec3(0.0f, 1.0f, 0.0f));

    mat2 m2(4.0f, 7.0f,
            2.0f, 6.0f);
    assert(near(determinant(m2), 10.0f));
    mat2 m2Identity = m2 * inverse(m2);
    assert(near(m2Identity[0][0], 1.0f));
    assert(near(m2Identity[1][1], 1.0f));
    assert(near(m2Identity[0][1], 0.0f));
    assert(near(m2Identity[1][0], 0.0f));

    mat3 m3(2.0f, 0.0f, 0.0f,
            0.0f, 3.0f, 0.0f,
            0.0f, 0.0f, 4.0f);
    assert(near(determinant(m3), 24.0f));
    expect_vec3((m3 * inverse(m3)) * vec3(1.0f, 2.0f, 3.0f), vec3(1.0f, 2.0f, 3.0f));

    mat4 transform = translate(vec3(3.0f, -2.0f, 5.0f))
                   * rotate(radians(90.0f), vec3(0.0f, 0.0f, 1.0f))
                   * scale(vec3(2.0f, 3.0f, 4.0f));
    expect_mat4_identity(transform * inverse(transform));

    mat4 projection = ortho(-2.0f, 2.0f, -4.0f, 4.0f, -1.0f, 1.0f);
    expect_vec4(projection * vec4(2.0f, 4.0f, -1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f));

    quat quarterTurn = angleAxis(radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
    expect_vec3(quarterTurn * vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    expect_vec3(mat3_cast(quarterTurn) * vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    expect_vec3(slerp(quat(), quarterTurn, 1.0f) * vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

    float h = hash(vec2(1.25f, -2.5f));
    assert(h >= 0.0f && h < 1.0f);
    assert(std::isfinite(noise(vec2(0.25f, 0.75f))));
    assert(std::isfinite(fbm(vec2(0.25f, 0.75f), 4)));

    Camera camera;
    expect_mat4_identity(camera.getViewMatrix() * inverse(camera.getViewMatrix()));
    assert(std::isfinite(camera.getProjectionMatrix()[0][0]));

    return 0;
}
