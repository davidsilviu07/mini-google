#pragma once
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Micro test framework, fara dependinte externe.
//
// Fiecare TEST(nume) { ... } se inregistreaza automat intr-un registru global
// la incarcarea programului (prin constructorul unui obiect static). main-ul
// din test_main.cpp apoi le ruleaza pe toate si numara esecurile.
// ---------------------------------------------------------------------------

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}
inline int& failures() {
    static int f = 0;
    return f;
}

// Obiect care, cand e construit, inregistreaza un test in registru.
struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

// TEST(nume) declara functia, o inregistreaza printr-un Registrar static,
// apoi deschide corpul functiei.
#define TEST(name)                                    \
    static void name();                               \
    static Registrar reg_##name(#name, name);         \
    static void name()

// Verifica o conditie booleana.
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            ++failures();                                                 \
            std::cout << "    FAIL: " << #cond << "  (linia " << __LINE__ \
                      << ")\n";                                           \
        }                                                                 \
    } while (0)

// Egalitate pentru tipuri afisabile (int, size_t, double, string...).
#define CHECK_EQ(a, b)                                                      \
    do {                                                                    \
        auto _va = (a);                                                     \
        auto _vb = (b);                                                     \
        if (!(_va == _vb)) {                                                \
            ++failures();                                                   \
            std::cout << "    FAIL: " << #a << " == " << #b << "  [" << _va \
                      << " vs " << _vb << "]  (linia " << __LINE__ << ")\n"; \
        }                                                                   \
    } while (0)

// Egalitate aproximativa pentru numere in virgula mobila.
#define CHECK_NEAR(a, b, eps)                                              \
    do {                                                                   \
        double _da = (a);                                                  \
        double _db = (b);                                                  \
        if (std::fabs(_da - _db) > (eps)) {                                \
            ++failures();                                                  \
            std::cout << "    FAIL: |" << #a << " - " << #b << "| <= " << (eps) \
                      << "  [" << _da << " vs " << _db << "]  (linia "     \
                      << __LINE__ << ")\n";                                \
        }                                                                  \
    } while (0)

inline int run_all_tests() {
    int passed = 0;
    for (auto& t : registry()) {
        int before = failures();
        t.fn();
        if (failures() == before) {
            ++passed;
            std::cout << "[PASS] " << t.name << "\n";
        } else {
            std::cout << "[----] " << t.name << " -- vezi esecurile de mai sus\n";
        }
    }
    std::cout << "\n"
              << passed << "/" << registry().size() << " teste trecute, "
              << failures() << " assertion-uri esuate\n";
    return failures() == 0 ? 0 : 1;
}
