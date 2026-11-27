#pragma once
// Mini framework de pruebas sin dependencias.
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace xt {
struct Case {
    const char* name;
    std::function<void()> fn;
};
inline std::vector<Case>& registry() {
    static std::vector<Case> r;
    return r;
}
inline int& failures() {
    static int f = 0;
    return f;
}
struct Reg {
    Reg(const char* n, std::function<void()> f) { registry().push_back({n, std::move(f)}); }
};
inline std::string str(const std::string& v) { return "\"" + v + "\""; }
template <class T>
std::string str(const T& v) { return std::to_string(v); }
inline int runAll() {
    int failedCases = 0;
    for (auto& c : registry()) {
        int before = failures();
        c.fn();
        bool ok = failures() == before;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", c.name);
        if (!ok) ++failedCases;
    }
    std::printf("%zu casos, %d fallidos\n", registry().size(), failedCases);
    return failedCases ? 1 : 0;
}
}  // namespace xt

#define XT_CAT2(a, b) a##b
#define XT_CAT(a, b) XT_CAT2(a, b)
#define TEST(name)                                                         \
    static void XT_CAT(test_fn_, __LINE__)();                              \
    static xt::Reg XT_CAT(test_reg_, __LINE__)(name, XT_CAT(test_fn_, __LINE__)); \
    static void XT_CAT(test_fn_, __LINE__)()
#define CHECK(cond)                                                                \
    do {                                                                           \
        if (!(cond)) {                                                             \
            std::printf("  %s:%d: fallo CHECK(%s)\n", __FILE__, __LINE__, #cond);  \
            ++xt::failures();                                                      \
        }                                                                          \
    } while (0)
#define CHECK_EQ(a, b)                                                                              \
    do {                                                                                            \
        auto _va = (a);                                                                             \
        auto _vb = (b);                                                                             \
        if (!(_va == _vb)) {                                                                        \
            std::printf("  %s:%d: fallo CHECK_EQ(%s, %s) -> %s vs %s\n", __FILE__, __LINE__, #a, #b, \
                        xt::str(_va).c_str(), xt::str(_vb).c_str());                  \
            ++xt::failures();                                                                       \
        }                                                                                           \
    } while (0)
#define TEST_MAIN() \
    int main() { return xt::runAll(); }
