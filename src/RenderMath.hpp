#pragma once
#include <algorithm>
#include <cmath>
#include <optional>

namespace lsf {
inline bool positive(float value) { return std::isfinite(value) && value > 0.f; }
struct Dimensions { int width; int height; };
inline std::optional<Dimensions> dimensions(float width, float height) {
    // Bound before converting float to integer.
    if (!positive(width) || !positive(height) || width > 32768.f || height > 32768.f)
        return std::nullopt;
    return Dimensions{std::max(1, static_cast<int>(std::ceil(width))),
        std::max(1, static_cast<int>(std::ceil(height)))};
}
inline float pixelSize(float targetPixels, float logicalPixels, float requestedPixels) {
    if (!positive(targetPixels) || !positive(logicalPixels)) return 1.f;
    if (!positive(requestedPixels)) requestedPixels = 1.f;
    float cells = std::max(1.f, std::round(logicalPixels / std::max(1.f, requestedPixels)));
    return targetPixels / cells;
}
template<class T> class RestoreValue {
    T& m_target;
    T m_saved;
public:
    RestoreValue(T& target, T value) : m_target(target), m_saved(target) { target = value; }
    RestoreValue(RestoreValue const&) = delete;
    RestoreValue& operator=(RestoreValue const&) = delete;
    ~RestoreValue() { m_target = m_saved; }
};
}
