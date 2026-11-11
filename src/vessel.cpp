#include "kitezh/vessel.hpp"

#include <stdexcept>

namespace kitezh {

std::optional<std::string> validateVessel(const Vessel&) {
    throw std::logic_error("validateVessel: реализуйте");
}

bool fitsInChamber(const std::vector<Vessel>&, const Vessel&) {
    throw std::logic_error("fitsInChamber: реализуйте");
}

}  // namespace kitezh
