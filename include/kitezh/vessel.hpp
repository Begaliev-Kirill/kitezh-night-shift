#pragma once

#include "kitezh/geometry.hpp"

#include <optional>
#include <string>
#include <vector>

namespace kitezh {

enum class Direction {
    Upstream,
    Downstream,
};

enum class VesselKind {
    Regular,
    Ambulance,
    Oversized,
};

// Запись о судне: открытые данные без правил шлюза.
// Не добавляйте конструкторы и закрытые поля — это агрегат,
// тесты собирают его через фигурные скобки.
struct Vessel {
    std::string call_sign;
    int length_m;
    int draft_dm;
    Direction direction;
    VesselKind kind;
};

// nullopt — запись можно рассматривать дальше.
// Иначе одно из сообщений: "empty call sign", "bad call sign",
// "bad length", "bad draft". Порядок проверок — в README.
[[nodiscard]] std::optional<std::string> validateVessel(const Vessel& vessel);

// Влезает ли next к уже стоящим судам по метрам и одному зазору между соседями.
// Негабарит, направление и осадка здесь ни при чём: это правило камеры, не линейки.
[[nodiscard]] bool fitsInChamber(const std::vector<Vessel>& inside, const Vessel& next);

}  // namespace kitezh
