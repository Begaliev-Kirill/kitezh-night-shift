#include "harness.hpp"

#include <vector>

void registerVesselTests(std::vector<kitezh_test::TestCase>& tests);
void registerLockTests(std::vector<kitezh_test::TestCase>& tests);
void registerShiftTests(std::vector<kitezh_test::TestCase>& tests);

int main() {
    std::vector<kitezh_test::TestCase> tests;
    registerVesselTests(tests);
    registerLockTests(tests);
    registerShiftTests(tests);
    return kitezh_test::runAll(tests);
}
