#include "harness.hpp"
#include "make_vessel.hpp"

#include "kitezh/shift.hpp"

#include <string>
#include <vector>

using kitezh::Direction;
using kitezh::Refusal;
using kitezh::ShiftReport;
using kitezh::Vessel;
using kitezh::VesselKind;
using kitezh::runNightShift;

namespace {

void expectClosed(const std::vector<Vessel>& queue, const ShiftReport& report) {
    EXPECT_EQ(static_cast<int>(report.passed.size() + report.refused.size()), static_cast<int>(queue.size()));
}

void expectRefused(const ShiftReport& report, const std::vector<Refusal>& expected) {
    EXPECT_EQ(static_cast<int>(report.refused.size()), static_cast<int>(expected.size()));
    const std::size_t count = report.refused.size() < expected.size() ? report.refused.size() : expected.size();
    for (std::size_t index = 0; index < count; ++index) {
        EXPECT_EQ(report.refused[index].call_sign, expected[index].call_sign);
        EXPECT_EQ(report.refused[index].reason, expected[index].reason);
    }
}

void emptyNightDoesNothing() {
    const ShiftReport report = runNightShift({});
    EXPECT(report.passed.empty());
    EXPECT(report.refused.empty());
    EXPECT_EQ(report.pump_cycles, 0);
}

void singleUpstreamCostsOneCycle() {
    const std::vector<Vessel> queue{makeVessel("Volga", 10, 20, Direction::Upstream)};
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"Volga"};
    EXPECT_EQ(report.passed, passed);
    EXPECT(report.refused.empty());
    EXPECT_EQ(report.pump_cycles, 1);
    expectClosed(queue, report);
}

void singleDownstreamCostsTwoCycles() {
    const std::vector<Vessel> queue{makeVessel("Don", 10, 20, Direction::Downstream)};
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"Don"};
    EXPECT_EQ(report.passed, passed);
    EXPECT_EQ(report.pump_cycles, 2);
}

void twoThatFitShareOneLift() {
    const std::vector<Vessel> queue{
        makeVessel("A", 17, 20, Direction::Upstream),
        makeVessel("B", 18, 20, Direction::Upstream),
    };
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"A", "B"};
    EXPECT_EQ(report.passed, passed);
    EXPECT_EQ(report.pump_cycles, 1);
}

void twoThatMissByTheGapGoSeparately() {
    const std::vector<Vessel> queue{
        makeVessel("A", 18, 20, Direction::Upstream),
        makeVessel("B", 18, 20, Direction::Upstream),
    };
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"A", "B"};
    EXPECT_EQ(report.passed, passed);
    EXPECT_EQ(report.pump_cycles, 3);
}

void fiveShortBoatsAreOneBatch() {
    std::vector<Vessel> queue;
    for (int index = 0; index < 5; ++index) {
        queue.push_back(makeVessel("S" + std::to_string(index + 1), 6, 20, Direction::Upstream));
    }
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"S1", "S2", "S3", "S4", "S5"};
    EXPECT_EQ(report.passed, passed);
    EXPECT_EQ(report.pump_cycles, 1);
}

void oversizedDoesNotTakeANeighborEvenWhenMetersAllow() {
    const std::vector<Vessel> queue{
        makeVessel("O", 20, 20, Direction::Upstream, VesselKind::Oversized),
        makeVessel("R", 10, 20, Direction::Upstream),
    };
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"O", "R"};
    EXPECT_EQ(report.passed, passed);
    EXPECT_EQ(report.pump_cycles, 3);
}

void readmeAmbulanceCutsIntoTheOpenBatch() {
    // Разбор из README: А вверх, Б вниз, В скорая вверх.
    const std::vector<Vessel> queue{
        makeVessel("А", 10, 20, Direction::Upstream),
        makeVessel("Б", 10, 20, Direction::Downstream),
        makeVessel("В", 10, 20, Direction::Upstream, VesselKind::Ambulance),
    };
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"А", "В", "Б"};
    EXPECT_EQ(report.passed, passed);
    EXPECT_EQ(report.pump_cycles, 2);
}

void ambulanceDoesNotPreemptTheHead() {
    const std::vector<Vessel> queue{
        makeVessel("Г", 10, 20, Direction::Downstream),
        makeVessel("Д", 10, 20, Direction::Upstream, VesselKind::Ambulance),
        makeVessel("Е", 10, 20, Direction::Upstream, VesselKind::Ambulance),
    };
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"Г", "Д", "Е"};
    EXPECT_EQ(report.passed, passed);
    EXPECT_EQ(report.pump_cycles, 3);
}

void regularDoesNotJumpAndATightAmbulanceDoesNotClearTheWay() {
    const std::vector<Vessel> queue{
        makeVessel("A", 20, 20, Direction::Upstream),
        makeVessel("B", 20, 20, Direction::Upstream, VesselKind::Ambulance),
        makeVessel("C", 10, 20, Direction::Upstream),
    };
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"A", "B", "C"};
    EXPECT_EQ(report.passed, passed);
    // B не влезла к A, но в своей партии забирает C: 20 + 1 + 10 = 31.
    EXPECT_EQ(report.pump_cycles, 3);
}

void struckOutVesselsDoNotSplitABatch() {
    const std::vector<Vessel> deep_between{
        makeVessel("A", 10, 20, Direction::Upstream),
        makeVessel("B", 10, 30, Direction::Downstream),
        makeVessel("C", 10, 20, Direction::Upstream),
    };
    const ShiftReport deep_report = runNightShift(deep_between);
    const std::vector<std::string> deep_passed{"A", "C"};
    EXPECT_EQ(deep_report.passed, deep_passed);
    EXPECT_EQ(deep_report.pump_cycles, 1);
    expectRefused(deep_report, {Refusal{"B", "draft"}});
    expectClosed(deep_between, deep_report);

    const std::vector<Vessel> invalid_between{
        makeVessel("A", 10, 20, Direction::Upstream),
        makeVessel("bad sign", 10, 20, Direction::Downstream),
        makeVessel("C", 10, 28, Direction::Upstream),
    };
    const ShiftReport invalid_report = runNightShift(invalid_between);
    const std::vector<std::string> invalid_passed{"A", "C"};
    EXPECT_EQ(invalid_report.passed, invalid_passed);
    EXPECT_EQ(invalid_report.pump_cycles, 1);
    expectRefused(invalid_report, {Refusal{"bad sign", "invalid"}});

    const std::vector<Vessel> duplicated{
        makeVessel("A", 10, 20, Direction::Upstream),
        makeVessel("A", 10, 20, Direction::Downstream),
        makeVessel("C", 10, 20, Direction::Upstream),
    };
    const ShiftReport duplicate_report = runNightShift(duplicated);
    const std::vector<std::string> duplicate_passed{"A", "C"};
    EXPECT_EQ(duplicate_report.passed, duplicate_passed);
    EXPECT_EQ(duplicate_report.pump_cycles, 1);
    expectRefused(duplicate_report, {Refusal{"A", "duplicate"}});
    expectClosed(duplicated, duplicate_report);
}

void callSignIsConsumedOnFirstSight() {
    const std::vector<Vessel> invalid_then_copy{
        makeVessel("bad sign", 10, 20, Direction::Upstream),
        makeVessel("bad sign", 12, 20, Direction::Upstream),
    };
    const ShiftReport invalid_report = runNightShift(invalid_then_copy);
    EXPECT(invalid_report.passed.empty());
    EXPECT_EQ(invalid_report.pump_cycles, 0);
    expectRefused(
        invalid_report,
        {Refusal{"bad sign", "invalid"}, Refusal{"bad sign", "duplicate"}}
    );

    const std::vector<Vessel> deep_then_shallow{
        makeVessel("X", 10, 40, Direction::Upstream),
        makeVessel("X", 10, 20, Direction::Downstream),
    };
    const ShiftReport deep_report = runNightShift(deep_then_shallow);
    EXPECT(deep_report.passed.empty());
    EXPECT_EQ(deep_report.pump_cycles, 0);
    expectRefused(deep_report, {Refusal{"X", "draft"}, Refusal{"X", "duplicate"}});
    expectClosed(deep_then_shallow, deep_report);
}

void draftBoundaryIsTheLowWater() {
    const std::vector<Vessel> queue{
        makeVessel("Fit", 10, 28, Direction::Downstream),
        makeVessel("Sink", 10, 29, Direction::Downstream),
    };
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"Fit"};
    EXPECT_EQ(report.passed, passed);
    EXPECT_EQ(report.pump_cycles, 2);
    expectRefused(report, {Refusal{"Sink", "draft"}});
}

void capstoneNight() {
    // Вычеркнутые не стоят в очереди. Скорая своего направления вклинивается,
    // обычное судно — нет. Негабарит никого с собой не берёт.
    const std::vector<Vessel> queue{
        makeVessel("R1", 10, 20, Direction::Upstream),
        makeVessel("DOWN", 12, 20, Direction::Downstream),
        makeVessel("AMB", 8, 20, Direction::Upstream, VesselKind::Ambulance),
        makeVessel("R2", 10, 20, Direction::Upstream),
        makeVessel("DEEP", 10, 40, Direction::Upstream),
        makeVessel("bad sign", 10, 20, Direction::Upstream),
        makeVessel("OVR", 20, 20, Direction::Upstream, VesselKind::Oversized),
        makeVessel("AMB2", 10, 20, Direction::Upstream, VesselKind::Ambulance),
        makeVessel("R1", 10, 20, Direction::Downstream),
    };
    const ShiftReport report = runNightShift(queue);
    const std::vector<std::string> passed{"R1", "AMB", "AMB2", "DOWN", "R2", "OVR"};
    EXPECT_EQ(report.passed, passed);
    EXPECT_EQ(report.pump_cycles, 5);
    expectRefused(
        report,
        {
            Refusal{"DEEP", "draft"},
            Refusal{"bad sign", "invalid"},
            Refusal{"R1", "duplicate"},
        }
    );
    expectClosed(queue, report);
}

}  // namespace

void registerShiftTests(std::vector<kitezh_test::TestCase>& tests) {
    tests.push_back({"shift / пустая ночь", emptyNightDoesNothing});
    tests.push_back({"shift / одно судно вверх — один такт", singleUpstreamCostsOneCycle});
    tests.push_back({"shift / одно судно вниз — два такта", singleDownstreamCostsTwoCycles});
    tests.push_back({"shift / 17 и 18 делят одну перекачку", twoThatFitShareOneLift});
    tests.push_back({"shift / 18 и 18 не входят вместе", twoThatMissByTheGapGoSeparately});
    tests.push_back({"shift / пять коротких — одна партия, не пять", fiveShortBoatsAreOneBatch});
    tests.push_back({"shift / негабарит идёт один, даже если метры есть", oversizedDoesNotTakeANeighborEvenWhenMetersAllow});
    tests.push_back({"shift / разбор из README: скорая вклинивается", readmeAmbulanceCutsIntoTheOpenBatch});
    tests.push_back({"shift / скорая не обгоняет уже первое судно", ambulanceDoesNotPreemptTheHead});
    tests.push_back({"shift / обычное не вклинивается, тесная скорая не открывает лазейку", regularDoesNotJumpAndATightAmbulanceDoesNotClearTheWay});
    tests.push_back({"shift / вычеркнутые не рвут партию", struckOutVesselsDoNotSplitABatch});
    tests.push_back({"shift / позывной занят с первого появления", callSignIsConsumedOnFirstSight});
    tests.push_back({"shift / осадка 28 проходит, 29 уже нет", draftBoundaryIsTheLowWater});
    tests.push_back({"shift / ночь целиком", capstoneNight});
}
