#include "harness.hpp"
#include "make_vessel.hpp"

#include "kitezh/vessel.hpp"

#include <optional>
#include <string>
#include <vector>

using kitezh::Direction;
using kitezh::Vessel;
using kitezh::VesselKind;
using kitezh::fitsInChamber;
using kitezh::validateVessel;

namespace {

void expectError(const Vessel& vessel, const char* reason) {
    const std::optional<std::string> error = validateVessel(vessel);
    EXPECT(error.has_value());
    EXPECT_EQ(*error, reason);
}

void emptyCallSignWinsOverOtherGarbage() {
    expectError(makeVessel("", -5, -5, Direction::Upstream), "empty call sign");
}

void whitespaceCallSignIsRejectedBeforeLength() {
    expectError(makeVessel("A B", 0, 0, Direction::Downstream), "bad call sign");
    expectError(makeVessel("A\tB", 10, 20, Direction::Upstream), "bad call sign");
}

void badLengthAndBadDraftHavePriority() {
    expectError(makeVessel("AB", 0, -5, Direction::Upstream), "bad length");
    expectError(makeVessel("AB", -1, 10, Direction::Upstream), "bad length");
    expectError(makeVessel("AB", 37, 10, Direction::Upstream), "bad length");
    expectError(makeVessel("AB", 10, 0, Direction::Upstream), "bad draft");
}

void ordinaryCallSignsAreValid() {
    const Vessel volga = makeVessel("Volga", 36, 28, Direction::Upstream);
    EXPECT(!validateVessel(volga).has_value());
    const Vessel local = makeVessel("Китеж-1", 1, 1, Direction::Downstream, VesselKind::Ambulance);
    EXPECT(!validateVessel(local).has_value());
    const Vessel underscored = makeVessel("A_1", 18, 21, Direction::Upstream, VesselKind::Oversized);
    EXPECT(!validateVessel(underscored).has_value());
}

void singleVesselUsesNoGap() {
    EXPECT(fitsInChamber({}, makeVessel("A", 36, 20, Direction::Upstream)));
    EXPECT(!fitsInChamber({}, makeVessel("A", 37, 20, Direction::Upstream)));
}

void oneGapBetweenNeighborsAndNotAtTheWalls() {
    const std::vector<Vessel> eighteen{makeVessel("A", 18, 20, Direction::Upstream)};
    EXPECT(fitsInChamber(eighteen, makeVessel("B", 17, 20, Direction::Upstream)));
    EXPECT(!fitsInChamber(eighteen, makeVessel("B", 18, 20, Direction::Upstream)));
}

void threeVesselsUseTwoGaps() {
    const std::vector<Vessel> inside{
        makeVessel("A", 10, 20, Direction::Upstream),
        makeVessel("B", 10, 20, Direction::Upstream),
    };
    EXPECT(fitsInChamber(inside, makeVessel("C", 14, 20, Direction::Upstream)));
    EXPECT(!fitsInChamber(inside, makeVessel("C", 15, 20, Direction::Upstream)));
}

void metersIgnoreKindAndDirection() {
    const std::vector<Vessel> oversized{
        makeVessel("O", 20, 20, Direction::Downstream, VesselKind::Oversized),
    };
    // 20 + 1 + 10 = 31. Запрет «негабарит один» — правило камеры, не линейки.
    EXPECT(fitsInChamber(oversized, makeVessel("S", 10, 20, Direction::Upstream, VesselKind::Ambulance)));
}

}  // namespace

void registerVesselTests(std::vector<kitezh_test::TestCase>& tests) {
    tests.push_back({"vessel / пустой позывной важнее остальных ошибок", emptyCallSignWinsOverOtherGarbage});
    tests.push_back({"vessel / пробел в позывном важнее длины", whitespaceCallSignIsRejectedBeforeLength});
    tests.push_back({"vessel / длина проверяется раньше осадки", badLengthAndBadDraftHavePriority});
    tests.push_back({"vessel / обычный, русский и составной позывные годны", ordinaryCallSignsAreValid});
    tests.push_back({"vessel / одно судно не платит зазор у стен", singleVesselUsesNoGap});
    tests.push_back({"vessel / между двумя судами ровно один зазор", oneGapBetweenNeighborsAndNotAtTheWalls});
    tests.push_back({"vessel / три судна — два зазора, граница ровно 36", threeVesselsUseTwoGaps});
    tests.push_back({"vessel / линейка не знает про негабарит и направление", metersIgnoreKindAndDirection});
}
