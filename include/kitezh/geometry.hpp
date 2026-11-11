#pragma once

namespace kitezh {

// Полезная длина камеры. Швартовка к стенкам уже вычтена:
// зазор добавляется только между соседними судами.
inline constexpr int kChamberLengthM = 36;

inline constexpr int kGapM = 1;

// Глубина воды. Осадка судна сравнивается с этими числами, не с длиной.
inline constexpr int kLowDepthDm = 28;
inline constexpr int kHighDepthDm = 42;

}  // namespace kitezh
