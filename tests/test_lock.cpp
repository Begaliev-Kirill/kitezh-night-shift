#include "harness.hpp"
#include "make_vessel.hpp"

#include "kitezh/lock.hpp"

#include <string>
#include <vector>

using kitezh::Direction;
using kitezh::Gate;
using kitezh::GateState;
using kitezh::Level;
using kitezh::Lock;
using kitezh::LockError;
using kitezh::Vessel;
using kitezh::VesselKind;

namespace {

struct Snapshot {
    Level level;
    GateState lower;
    GateState upper;
    int cycles;
    std::vector<std::string> signs;
    std::vector<int> lengths;
};

Snapshot take(const Lock& lock) {
    Snapshot snapshot;
    snapshot.level = lock.level();
    snapshot.lower = lock.gate(Gate::Lower);
    snapshot.upper = lock.gate(Gate::Upper);
    snapshot.cycles = lock.pumpCycles();
    for (const Vessel& vessel : lock.inside()) {
        snapshot.signs.push_back(vessel.call_sign);
        snapshot.lengths.push_back(vessel.length_m);
    }
    return snapshot;
}

void expectSame(const Lock& lock, const Snapshot& before) {
    EXPECT(lock.level() == before.level);
    EXPECT(lock.gate(Gate::Lower) == before.lower);
    EXPECT(lock.gate(Gate::Upper) == before.upper);
    EXPECT_EQ(lock.pumpCycles(), before.cycles);
    const std::vector<Vessel> inside = lock.inside();
    EXPECT_EQ(static_cast<int>(inside.size()), static_cast<int>(before.signs.size()));
    for (std::size_t index = 0; index < inside.size() && index < before.signs.size(); ++index) {
        EXPECT_EQ(inside[index].call_sign, before.signs[index]);
        EXPECT_EQ(inside[index].length_m, before.lengths[index]);
    }
}

void initialStateIsObservedThroughConstLock() {
    const Lock lock;
    EXPECT(lock.level() == Level::Low);
    EXPECT(lock.gate(Gate::Lower) == GateState::Closed);
    EXPECT(lock.gate(Gate::Upper) == GateState::Closed);
    EXPECT_EQ(lock.pumpCycles(), 0);
    EXPECT(lock.inside().empty());
}

void oneUpstreamVesselCanBeWorkedByHand() {
    Lock lock;
    lock.open(Gate::Lower);
    lock.enter(makeVessel("Volga", 12, 21, Direction::Upstream));
    lock.close(Gate::Lower);
    lock.pump();
    lock.open(Gate::Upper);
    lock.exit("Volga");
    lock.close(Gate::Upper);

    EXPECT(lock.level() == Level::High);
    EXPECT(lock.inside().empty());
    EXPECT(lock.gate(Gate::Lower) == GateState::Closed);
    EXPECT(lock.gate(Gate::Upper) == GateState::Closed);
    EXPECT_EQ(lock.pumpCycles(), 1);
}

void oneDownstreamVesselNeedsAnEmptyPumpFirst() {
    Lock lock;
    lock.pump();
    lock.open(Gate::Upper);
    lock.enter(makeVessel("Don", 12, 21, Direction::Downstream));
    lock.close(Gate::Upper);
    lock.pump();
    lock.open(Gate::Lower);
    lock.exit("Don");
    lock.close(Gate::Lower);

    EXPECT(lock.level() == Level::Low);
    EXPECT(lock.inside().empty());
    EXPECT_EQ(lock.pumpCycles(), 2);
}

void gatesFollowTheLevelAndEachOther() {
    Lock lock;
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.open(Gate::Upper), LockError);
        expectSame(lock, before);
    }
    lock.open(Gate::Lower);
    lock.open(Gate::Lower);
    EXPECT(lock.gate(Gate::Lower) == GateState::Open);
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.open(Gate::Upper), LockError);
        expectSame(lock, before);
    }
    lock.close(Gate::Lower);
    lock.close(Gate::Lower);
    EXPECT(lock.gate(Gate::Lower) == GateState::Closed);

    lock.pump();
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.open(Gate::Lower), LockError);
        expectSame(lock, before);
    }
    lock.open(Gate::Upper);
    EXPECT(lock.gate(Gate::Upper) == GateState::Open);
}

void pumpRefusesAnOpenGateWithoutSpendingACycle() {
    Lock lock;
    lock.open(Gate::Lower);
    const Snapshot before = take(lock);
    EXPECT_THROW(lock.pump(), LockError);
    expectSame(lock, before);
    EXPECT_EQ(lock.pumpCycles(), 0);
}

void emptyPumpFlipsTheLevel() {
    Lock lock;
    lock.pump();
    EXPECT(lock.level() == Level::High);
    EXPECT_EQ(lock.pumpCycles(), 1);
    lock.pump();
    EXPECT(lock.level() == Level::Low);
    EXPECT_EQ(lock.pumpCycles(), 2);
}

void chamberKeepsItsOwnCopy() {
    Lock lock;
    Vessel vessel = makeVessel("A", 10, 20, Direction::Upstream);
    lock.open(Gate::Lower);
    lock.enter(vessel);
    vessel.length_m = 99;
    vessel.call_sign = "mutated";
    vessel.draft_dm = 1;

    EXPECT_EQ(lock.inside().at(0).call_sign, std::string("A"));
    EXPECT_EQ(lock.inside().at(0).length_m, 10);
    EXPECT_EQ(lock.inside().at(0).draft_dm, 20);
}

void rejectedEnterDoesNotKeepTheVessel() {
    Lock lock;
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.enter(makeVessel("A", 10, 20, Direction::Upstream)), LockError);
        expectSame(lock, before);
    }

    lock.pump();
    lock.open(Gate::Upper);
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.enter(makeVessel("A", 10, 20, Direction::Upstream)), LockError);
        expectSame(lock, before);
    }

    lock.close(Gate::Upper);
    lock.pump();
    lock.open(Gate::Lower);
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.enter(makeVessel("Deep", 10, 29, Direction::Upstream)), LockError);
        expectSame(lock, before);
    }
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.enter(makeVessel(" ", 10, 20, Direction::Upstream)), LockError);
        expectSame(lock, before);
    }

    lock.enter(makeVessel("A", 10, 20, Direction::Upstream));
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.enter(makeVessel("A", 10, 20, Direction::Upstream)), LockError);
        expectSame(lock, before);
    }
}

void aVesselThatDoesNotFitLeavesTheFirstOneInside() {
    Lock lock;
    lock.open(Gate::Lower);
    lock.enter(makeVessel("A", 30, 20, Direction::Upstream));
    const Snapshot before = take(lock);
    EXPECT_THROW(lock.enter(makeVessel("B", 10, 20, Direction::Upstream)), LockError);
    expectSame(lock, before);
    EXPECT(lock.gate(Gate::Lower) == GateState::Open);
}

void oversizedOccupiesTheChamberAlone() {
    Lock lock;
    lock.open(Gate::Lower);
    lock.enter(makeVessel("O", 10, 20, Direction::Upstream, VesselKind::Oversized));
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(
            lock.enter(makeVessel("R", 10, 20, Direction::Upstream)),
            LockError
        );
        expectSame(lock, before);
    }

    Lock other;
    other.open(Gate::Lower);
    other.enter(makeVessel("R", 10, 20, Direction::Upstream));
    const Snapshot before = take(other);
    EXPECT_THROW(
        other.enter(makeVessel("O", 10, 20, Direction::Upstream, VesselKind::Oversized)),
        LockError
    );
    expectSame(other, before);
}

void twoRegularsThatFitStayInEntryOrder() {
    Lock lock;
    lock.open(Gate::Lower);
    lock.enter(makeVessel("A", 18, 20, Direction::Upstream));
    lock.enter(makeVessel("B", 17, 20, Direction::Upstream));
    EXPECT_EQ(lock.inside().at(0).call_sign, std::string("A"));
    EXPECT_EQ(lock.inside().at(1).call_sign, std::string("B"));
    EXPECT_THROW(lock.enter(makeVessel("C", 1, 20, Direction::Upstream)), LockError);
}

void exitIsFifoAndNeedsTheFarGate() {
    Lock lock;
    lock.open(Gate::Lower);
    lock.enter(makeVessel("A", 10, 20, Direction::Upstream));
    lock.enter(makeVessel("B", 10, 20, Direction::Upstream));
    lock.close(Gate::Lower);

    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.exit("A"), LockError);
        expectSame(lock, before);
    }

    lock.pump();
    lock.open(Gate::Upper);
    {
        const Snapshot before = take(lock);
        EXPECT_THROW(lock.exit("B"), LockError);
        expectSame(lock, before);
    }
    lock.exit("A");
    EXPECT_EQ(lock.inside().at(0).call_sign, std::string("B"));
    lock.exit("B");
    EXPECT(lock.inside().empty());
}

void groundingPumpIsAtomic() {
    Lock lock;
    lock.pump();
    lock.open(Gate::Upper);
    lock.enter(makeVessel("Shallow", 10, 20, Direction::Downstream));
    lock.enter(makeVessel("Deep", 10, 30, Direction::Downstream));
    lock.close(Gate::Upper);

    const Snapshot before = take(lock);
    EXPECT_THROW(lock.pump(), LockError);
    expectSame(lock, before);
    EXPECT_THROW(lock.exit("Shallow"), LockError);
    EXPECT_THROW(lock.open(Gate::Lower), LockError);
    expectSame(lock, before);
}

void draftEqualToTheDestinationDepthMayPass() {
    Lock lock;
    lock.pump();
    lock.open(Gate::Upper);
    lock.enter(makeVessel("Ok", 10, 28, Direction::Downstream));
    lock.close(Gate::Upper);
    lock.pump();
    EXPECT(lock.level() == Level::Low);
    EXPECT_EQ(lock.pumpCycles(), 2);
    EXPECT_EQ(lock.inside().at(0).call_sign, std::string("Ok"));
}

}  // namespace

void registerLockTests(std::vector<kitezh_test::TestCase>& tests) {
    tests.push_back({"lock / пустая камера на нижнем уровне", initialStateIsObservedThroughConstLock});
    tests.push_back({"lock / одно судно вверх проходит руками", oneUpstreamVesselCanBeWorkedByHand});
    tests.push_back({"lock / одно судно вниз начинает с пустой перекачки", oneDownstreamVesselNeedsAnEmptyPumpFirst});
    tests.push_back({"lock / затворы согласованы с уровнем и друг с другом", gatesFollowTheLevelAndEachOther});
    tests.push_back({"lock / открытый затвор не даёт насосу сжечь такт", pumpRefusesAnOpenGateWithoutSpendingACycle});
    tests.push_back({"lock / пустая камера переключается туда и обратно", emptyPumpFlipsTheLevel});
    tests.push_back({"lock / внутрь попадает копия, исходник можно портить", chamberKeepsItsOwnCopy});
    tests.push_back({"lock / отказ на входе не оставляет след", rejectedEnterDoesNotKeepTheVessel});
    tests.push_back({"lock / второе судно не вытесняет первое", aVesselThatDoesNotFitLeavesTheFirstOneInside});
    tests.push_back({"lock / негабарит ни с кем не делит камеру", oversizedOccupiesTheChamberAlone});
    tests.push_back({"lock / 18 и 17 входят, лишний метр — нет", twoRegularsThatFitStayInEntryOrder});
    tests.push_back({"lock / выход только в порядке входа и только в свои ворота", exitIsFifoAndNeedsTheFarGate});
    tests.push_back({"lock / насос не сажает судно на грунт и не врёт соседу", groundingPumpIsAtomic});
    tests.push_back({"lock / осадка ровно в глубину ещё проходит", draftEqualToTheDestinationDepthMayPass});
}
