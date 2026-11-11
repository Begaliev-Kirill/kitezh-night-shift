#include "kitezh/shift.hpp"

#include "kitezh/lock.hpp"

#include <stdexcept>

namespace kitezh {

ShiftReport runNightShift(const std::vector<Vessel>&) {
    // 1. Слева направо вычеркнуть duplicate, затем invalid, затем draft.
    //    Вычеркнутые не стоят в очереди и не рвут партию.
    // 2. Пока кто-то ждёт, собрать одну партию по правилам README.
    // 3. Провести её объектом Lock. Следующую начинать с закрытых затворов.
    // 4. pump_cycles взять из lock.pumpCycles() в конце, не из отдельного счётчика.
    throw std::logic_error("runNightShift: реализуйте");
}

}  // namespace kitezh
