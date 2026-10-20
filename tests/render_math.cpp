#include "../src/RenderMath.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
static void check(bool condition) {
    if (!condition) throw std::runtime_error("Render math check failed");
}
int main() {
    auto hd = lsf::dimensions(1920.f, 1080.f);
    check(hd && hd->width == 1920 && hd->height == 1080);
    auto fractional = lsf::dimensions(1920.25f, 1080.5f);
    check(fractional && fractional->width == 1921 && fractional->height == 1081);
    check(!lsf::dimensions(0.f, 1080.f));
    check(!lsf::dimensions(-1.f, 1080.f));
    check(!lsf::dimensions(std::numeric_limits<float>::infinity(), 1080.f));
    check(!lsf::dimensions(std::numeric_limits<float>::quiet_NaN(), 1080.f));
    check(!lsf::dimensions(1e30f, 1080.f));
    check(lsf::pixelSize(1920.f, 2608.f, 1e30f) == 1920.f);
    for (float requested : {0.f, -10.f, 1.f, 10.f, 1e30f,
         std::numeric_limits<float>::quiet_NaN()}) {
        auto value = lsf::pixelSize(1920.f, 2608.f, requested);
        check(lsf::positive(value) && value <= 1920.f);
    }
    float value = 2.f;
    try {
        lsf::RestoreValue outer(value, 3.f);
        { lsf::RestoreValue inner(value, 4.f); check(value == 4.f); }
        check(value == 3.f);
        throw 1;
    } catch (int) {}
    check(value == 2.f);
}
