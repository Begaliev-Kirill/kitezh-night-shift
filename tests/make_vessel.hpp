#pragma once

#include "kitezh/vessel.hpp"

#include <utility>

inline kitezh::Vessel makeVessel(
    std::string call_sign,
    int length_m,
    int draft_dm,
    kitezh::Direction direction,
    kitezh::VesselKind kind = kitezh::VesselKind::Regular
) {
    return kitezh::Vessel{std::move(call_sign), length_m, draft_dm, direction, kind};
}
