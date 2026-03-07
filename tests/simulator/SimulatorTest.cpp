#include <asmstudio/api/Expr.hpp>
#include <asmstudio/api/Function.hpp>
#include <asmstudio/core/Diagnostic.hpp>
#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/lowering/Lowering.hpp>
#include <asmstudio/simulator/Simulator.hpp>

#include <gtest/gtest.h>
#include <utility>

using namespace asmstudio;

// Helper: lower a single Function to an IRModule.
static IRModule lowerFn(std::string_view name, auto&& setup)
{
    auto fn = std::make_unique<Function>(std::string(name));
    setup(*fn);
    std::vector<std::unique_ptr<Function>> fns;
    fns.push_back(std::move(fn));
    DiagnosticBag diags;
    return lower(name, fns, diags);
}

TEST(Simulator, UnknownFunctionError)
{
    auto mod = lowerFn("m", [](Function& fn) { fn.ret(); });
    Simulator sim(mod);
    auto result = sim.run("nonexistent");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SimError::UnknownFunction);
}

TEST(Simulator, ReturnVoid)
{
    auto mod = lowerFn("m", [](Function& fn) { fn.ret(); });
    Simulator sim(mod);
    auto result = sim.run("m");
    ASSERT_TRUE(result.has_value());
    // void return — no value
    EXPECT_FALSE(result->has_value());
}

TEST(Simulator, ReturnConstant)
{
    auto mod = lowerFn("m", [](Function& fn) { fn.ret(expr(int64_t(42))); });
    Simulator sim(mod);
    auto result = sim.run("m");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 42);
}

TEST(Simulator, AssignAndReturn)
{
    auto mod = lowerFn("m", [](Function& fn) {
        Variable& x = fn.createInt("x", 0);
        fn.assign(x, expr(int64_t(7)));
        fn.ret(expr(x));
    });
    Simulator sim(mod);
    auto result = sim.run("m");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 7);
}

TEST(Simulator, SimpleAddition)
{
    auto mod = lowerFn("m", [](Function& fn) {
        Variable& a = fn.createInt("a", 0);
        Variable& b = fn.createInt("b", 0);
        fn.assign(a, expr(int64_t(3)));
        fn.assign(b, expr(int64_t(4)));
        fn.ret(add(expr(a), expr(b)));
    });
    Simulator sim(mod);
    auto result = sim.run("m");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 7);
}

TEST(Simulator, WhileLoopSum)
{
    // Sum 0+1+2+...+9 = 45
    auto mod = lowerFn("sum", [](Function& fn) {
        Variable& i = fn.createInt("i", 0);
        Variable& total = fn.createInt("total", 0);
        fn.assign(i, expr(int64_t(0)));
        fn.assign(total, expr(int64_t(0)));
        fn.whileLoop(i < int64_t(10), [&] {
            fn.assign(total, add(expr(total), expr(i)));
            fn.assign(i, add(expr(i), expr(int64_t(1))));
        });
        fn.ret(expr(total));
    });
    Simulator sim(mod);
    auto result = sim.run("sum");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 45);
}

TEST(Simulator, IfBranchTaken)
{
    auto mod = lowerFn("m", [](Function& fn) {
        Variable& x = fn.createInt("x", 0);
        fn.assign(x, expr(int64_t(1)));
        fn.ifStmt(x > int64_t(0), [&] { fn.assign(x, expr(int64_t(100))); });
        fn.ret(expr(x));
    });
    Simulator sim(mod);
    auto result = sim.run("m");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 100);
}

TEST(Simulator, IfBranchNotTaken)
{
    auto mod = lowerFn("m", [](Function& fn) {
        Variable& x = fn.createInt("x", 0);
        fn.assign(x, expr(int64_t(-1)));
        fn.ifStmt(x > int64_t(0), [&] { fn.assign(x, expr(int64_t(100))); });
        fn.ret(expr(x));
    });
    Simulator sim(mod);
    auto result = sim.run("m");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), -1);
}

TEST(Simulator, IfElseFalseArm)
{
    auto mod = lowerFn("m", [](Function& fn) {
        Variable& x = fn.createInt("x", 0);
        fn.assign(x, expr(int64_t(0)));
        fn.ifElse(x > int64_t(0), [&] { fn.assign(x, expr(int64_t(1))); }, [&] { fn.assign(x, expr(int64_t(-1))); });
        fn.ret(expr(x));
    });
    Simulator sim(mod);
    auto result = sim.run("m");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), -1);
}

TEST(Simulator, RewindRestoresState)
{
    auto mod = lowerFn("m", [](Function& fn) { fn.ret(expr(int64_t(1))); });
    Simulator sim(mod);
    std::ignore = sim.run("m");
    EXPECT_TRUE(sim.done()); // run() finished
    // Rewind restores to a prior snapshot → execution is no longer complete.
    sim.rewind(1);
    EXPECT_FALSE(sim.done()); // rewound to mid-execution state
}

TEST(Simulator, Reset)
{
    auto mod = lowerFn("m", [](Function& fn) { fn.ret(expr(int64_t(5))); });
    Simulator sim(mod);
    std::ignore = sim.run("m");
    sim.reset();
    // After reset, run again should work.
    auto result = sim.run("m");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 5);
}
