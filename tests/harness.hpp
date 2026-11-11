#pragma once

#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace kitezh_test {

struct Failure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct TestCase {
    const char* name;
    void (*run)();
};

inline std::string show(int value) {
    return std::to_string(value);
}

inline std::string show(const std::string& value) {
    return value;
}

inline std::string show(const char* value) {
    return value == nullptr ? "<null>" : value;
}

inline std::string show(const std::vector<std::string>& values) {
    std::string text = "[";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            text += ", ";
        }
        text += values[index];
    }
    text += "]";
    return text;
}

template <typename T>
std::string show(const T& value) {
    std::ostringstream out;
    out << value;
    return out.str();
}

template <typename Actual, typename Expected>
void expectEq(
    const Actual& actual,
    const Expected& expected,
    const char* expression,
    const char* file,
    int line
) {
    if (actual == expected) {
        return;
    }
    std::ostringstream out;
    out << file << ":" << line << "  " << expression << "\n    получили: " << show(actual)
        << "\n    ожидали:  " << show(expected);
    throw Failure(out.str());
}

inline int runAll(const std::vector<TestCase>& tests) {
    int passed = 0;
    int failed = 0;
    std::string current_group;
    int group_passed = 0;
    int group_failed = 0;
    std::vector<std::pair<std::string, std::pair<int, int>>> groups;

    for (const TestCase& test : tests) {
        const std::string name = test.name;
        const auto split = name.find(" / ");
        const std::string group = split == std::string::npos ? std::string("tests") : name.substr(0, split);
        if (group != current_group) {
            if (!current_group.empty()) {
                groups.push_back({current_group, {group_passed, group_failed}});
            }
            current_group = group;
            group_passed = 0;
            group_failed = 0;
            std::cout << "\n== " << group << " ==\n";
        }

        try {
            test.run();
            std::cout << "[ OK ] " << name << "\n";
            ++passed;
            ++group_passed;
        } catch (const std::exception& ex) {
            std::cout << "[FAIL] " << name << "\n    " << ex.what() << "\n";
            ++failed;
            ++group_failed;
        } catch (...) {
            std::cout << "[FAIL] " << name << "\n    неизвестное исключение\n";
            ++failed;
            ++group_failed;
        }
    }
    if (!current_group.empty()) {
        groups.push_back({current_group, {group_passed, group_failed}});
    }

    std::cout << "\n";
    for (const auto& group : groups) {
        const int ok = group.second.first;
        const int bad = group.second.second;
        std::cout << group.first << ": " << ok << "/" << (ok + bad) << "\n";
    }
    std::cout << "итого: " << passed << "/" << (passed + failed) << "\n";
    return failed == 0 ? 0 : 1;
}

}  // namespace kitezh_test

#define EXPECT(condition)                                                                          \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            throw kitezh_test::Failure(                                                            \
                std::string(__FILE__) + ":" + std::to_string(__LINE__) +                           \
                "  условие не выполнено: " #condition                                              \
            );                                                                                     \
        }                                                                                          \
    } while (0)

#define EXPECT_EQ(actual, expected)                                                                \
    kitezh_test::expectEq(                                                                         \
        (actual), (expected), #actual " == " #expected, __FILE__, __LINE__                        \
    )

#define EXPECT_THROW(expression, exception_type)                                                   \
    do {                                                                                           \
        bool kitezh_threw = false;                                                                 \
        try {                                                                                      \
            expression;                                                                            \
        } catch (const exception_type&) {                                                          \
            kitezh_threw = true;                                                                   \
        } catch (const std::exception& kitezh_ex) {                                                \
            throw kitezh_test::Failure(                                                            \
                std::string(__FILE__) + ":" + std::to_string(__LINE__) +                           \
                "  ждали " #exception_type ", а пришло: " + kitezh_ex.what()                       \
            );                                                                                     \
        }                                                                                          \
        if (!kitezh_threw) {                                                                       \
            throw kitezh_test::Failure(                                                            \
                std::string(__FILE__) + ":" + std::to_string(__LINE__) +                           \
                "  ждали исключение " #exception_type                                               \
            );                                                                                     \
        }                                                                                          \
    } while (0)
