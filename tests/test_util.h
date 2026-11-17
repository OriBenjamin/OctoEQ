#pragma once
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
struct TestCase { std::string name; std::function<void()> fn; };
inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
inline int& failures() { static int f = 0; return f; }
struct Reg { Reg(const char* n, std::function<void()> f) { registry().push_back({n, f}); } };
#define TEST(name) static void name(); static Reg reg_##name(#name, name); static void name()
#define CHECK(cond) do { if (!(cond)) { std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures(); } } while (0)
#define CHECK_NEAR(a, b, tol) do { double _a = (a), _b = (b); if (!(std::fabs(_a - _b) <= (tol))) { std::printf("  FAIL %s:%d: %s = %g, expected %g +/- %g\n", __FILE__, __LINE__, #a, _a, _b, (double)(tol)); ++failures(); } } while (0)
inline int runAll() {
    for (auto& t : registry()) { int before = failures(); t.fn(); std::printf("%s %s\n", failures() == before ? "ok  " : "FAIL", t.name.c_str()); }
    if (failures()) { std::printf("%d check(s) failed\n", failures()); return 1; }
    std::printf("All %zu tests passed\n", registry().size()); return 0;
}
