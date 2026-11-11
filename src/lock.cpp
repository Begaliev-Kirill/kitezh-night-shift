#include "kitezh/lock.hpp"

#include <stdexcept>

namespace kitezh {

Lock::Lock() {
    throw std::logic_error("Lock: реализуйте");
}

Level Lock::level() const {
    throw std::logic_error("Lock::level: реализуйте");
}

GateState Lock::gate(Gate) const {
    throw std::logic_error("Lock::gate: реализуйте");
}

int Lock::pumpCycles() const {
    throw std::logic_error("Lock::pumpCycles: реализуйте");
}

std::vector<Vessel> Lock::inside() const {
    throw std::logic_error("Lock::inside: реализуйте");
}

void Lock::open(Gate) {
    // Недопустимое открытие бросает LockError и не меняет камеру.
    throw std::logic_error("Lock::open: реализуйте");
}

void Lock::close(Gate) {
    throw std::logic_error("Lock::close: реализуйте");
}

void Lock::pump() {
    // Отказ насоса не тратит такт и не сажает уже вошедшее судно на грунт.
    throw std::logic_error("Lock::pump: реализуйте");
}

void Lock::enter(const Vessel&) {
    // Внутри должна оказаться копия. Входной затвор сам не открывается.
    throw std::logic_error("Lock::enter: реализуйте");
}

void Lock::exit(const std::string&) {
    // Выходит только голова очереди и только в свои ворота.
    throw std::logic_error("Lock::exit: реализуйте");
}

}  // namespace kitezh
