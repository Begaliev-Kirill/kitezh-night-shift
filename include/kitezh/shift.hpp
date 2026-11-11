#pragma once

#include "kitezh/vessel.hpp"

#include <string>
#include <vector>

namespace kitezh {

struct Refusal {
    std::string call_sign;
    std::string reason;
};

struct ShiftReport {
    std::vector<std::string> passed;
    std::vector<Refusal> refused;
    int pump_cycles = 0;
};

// Проводит очередь за ночь.
// reason у отказа: "invalid", "duplicate" или "draft".
// pump_cycles — сколько раз успешно сработал насос реальной камеры.
// На корректной по правилам README очереди функция не бросает исключений:
// сомнительное судно попадает в refused до enter.
[[nodiscard]] ShiftReport runNightShift(const std::vector<Vessel>& queue);

}  // namespace kitezh
