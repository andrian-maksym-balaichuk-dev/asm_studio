#include <asmstudio/dsl/Expr.hpp>
#include <asmstudio/dsl/Function.hpp>
#include <asmstudio/dsl/Stmt.hpp>
#include <asmstudio/dsl/Variable.hpp>

#include <gtest/gtest.h>

using namespace asmstudio;

// ---------------------------------------------------------------------------
// Expr node construction
// ---------------------------------------------------------------------------

TEST(Expr, ConstInt)
{
    // Given / When
    auto e = expr(int64_t(42));

    // Then
    EXPECT_TRUE(std::holds_alternative<ConstExpr>(e));
    auto& ce = std::get<ConstExpr>(e);
    EXPECT_EQ(std::get<int64_t>(ce.value), 42);
}

TEST(Expr, ConstDouble)
{
    auto e = expr(3.14);
    EXPECT_TRUE(std::holds_alternative<ConstExpr>(e));
    EXPECT_DOUBLE_EQ(std::get<double>(std::get<ConstExpr>(e).value), 3.14);
}

TEST(Expr, ConstBool)
{
    auto e = expr(true);
    EXPECT_TRUE(std::holds_alternative<ConstExpr>(e));
    EXPECT_TRUE(std::get<bool>(std::get<ConstExpr>(e).value));
}

TEST(Expr, ConstUInt64)
{
    auto e = expr(uint64_t(99));
    EXPECT_TRUE(std::holds_alternative<ConstExpr>(e));
    EXPECT_EQ(std::get<uint64_t>(std::get<ConstExpr>(e).value), 99u);
}

TEST(Expr, VarRef)
{
    Variable v("x", DataType::Int32, InitValue{ int64_t(0) });
    auto e = expr(v);
    EXPECT_TRUE(std::holds_alternative<VarExpr>(e));
    EXPECT_EQ(std::get<VarExpr>(e).variable.get().name(), "x");
}

TEST(Expr, BinAdd)
{
    auto e = add(expr(int64_t(1)), expr(int64_t(2)));
    EXPECT_TRUE(std::holds_alternative<std::shared_ptr<BinExpr>>(e));
    auto& be = *std::get<std::shared_ptr<BinExpr>>(e);
    EXPECT_EQ(be.op, BinOp::Add);
}

TEST(Expr, BinSub)
{
    auto e = sub(expr(int64_t(5)), expr(int64_t(3)));
    auto& be = *std::get<std::shared_ptr<BinExpr>>(e);
    EXPECT_EQ(be.op, BinOp::Sub);
}

TEST(Expr, BinMul)
{
    auto e = mul(expr(int64_t(4)), expr(int64_t(4)));
    auto& be = *std::get<std::shared_ptr<BinExpr>>(e);
    EXPECT_EQ(be.op, BinOp::Mul);
}

TEST(Expr, BinDiv)
{
    auto e = div(expr(int64_t(10)), expr(int64_t(2)));
    auto& be = *std::get<std::shared_ptr<BinExpr>>(e);
    EXPECT_EQ(be.op, BinOp::Div);
}

TEST(Expr, BinMod)
{
    auto e = mod(expr(int64_t(10)), expr(int64_t(3)));
    auto& be = *std::get<std::shared_ptr<BinExpr>>(e);
    EXPECT_EQ(be.op, BinOp::Mod);
}

TEST(Expr, UnaryNeg)
{
    auto e = neg(expr(int64_t(7)));
    EXPECT_TRUE(std::holds_alternative<std::shared_ptr<UnaryExpr>>(e));
    auto& ue = *std::get<std::shared_ptr<UnaryExpr>>(e);
    EXPECT_EQ(ue.op, UnaryOp::Neg);
}

TEST(Expr, UnaryBitNot)
{
    auto e = bitnot(expr(int64_t(0xFF)));
    auto& ue = *std::get<std::shared_ptr<UnaryExpr>>(e);
    EXPECT_EQ(ue.op, UnaryOp::BitNot);
}

TEST(Expr, NestedBin)
{
    // Given / When: (1 + 2) * 3
    auto e = mul(add(expr(int64_t(1)), expr(int64_t(2))), expr(int64_t(3)));

    // Then
    EXPECT_TRUE(std::holds_alternative<std::shared_ptr<BinExpr>>(e));
    auto& outer = *std::get<std::shared_ptr<BinExpr>>(e);
    EXPECT_EQ(outer.op, BinOp::Mul);
    EXPECT_TRUE(std::holds_alternative<std::shared_ptr<BinExpr>>(outer.leftOperand));
}

// ---------------------------------------------------------------------------
// Stmt / DSL tests
// ---------------------------------------------------------------------------

TEST(Stmt, AssignCollected)
{
    // Given
    Function fn("test");
    Variable& x = fn.createInt("x", 0);

    // When
    fn.assign(x, expr(int64_t(42)));

    // Then
    EXPECT_EQ(fn.statements().size(), 1u);
    EXPECT_TRUE(std::holds_alternative<AssignStmt>(fn.statements()[0].var()));
}

TEST(Stmt, ReturnNoValue)
{
    Function fn("test");
    fn.ret();
    EXPECT_EQ(fn.statements().size(), 1u);
    auto& rs = std::get<ReturnStmt>(fn.statements()[0].var());
    EXPECT_FALSE(rs.value.has_value());
}

TEST(Stmt, ReturnWithValue)
{
    Function fn("test");
    fn.ret(expr(int64_t(0)));
    auto& rs = std::get<ReturnStmt>(fn.statements()[0].var());
    EXPECT_TRUE(rs.value.has_value());
}

TEST(Stmt, Call)
{
    Function fn("test");
    fn.call("helper", { expr(int64_t(1)) });
    auto& cs = std::get<CallStmt>(fn.statements()[0].var());
    EXPECT_EQ(cs.callee, "helper");
    EXPECT_EQ(cs.arguments.size(), 1u);
}

TEST(Stmt, WhileLoop)
{
    Function fn("test");
    Variable& i = fn.createInt("i", 0);
    fn.whileLoop(i < int64_t(10), [&] { fn.assign(i, add(expr(i), expr(int64_t(1)))); });
    EXPECT_EQ(fn.statements().size(), 1u);
    auto& ws = std::get<WhileStmt>(fn.statements()[0].var());
    EXPECT_EQ(ws.body.size(), 1u);
}

TEST(Stmt, IfStmt)
{
    Function fn("test");
    Variable& x = fn.createInt("x", 0);
    fn.ifStmt(x == int64_t(0), [&] { fn.assign(x, expr(int64_t(1))); });
    EXPECT_EQ(fn.statements().size(), 1u);
    auto& is = std::get<IfStmt>(fn.statements()[0].var());
    EXPECT_EQ(is.thenBody.size(), 1u);
    EXPECT_TRUE(is.elseBody.empty());
}

TEST(Stmt, IfElse)
{
    Function fn("test");
    Variable& x = fn.createInt("x", 5);
    fn.ifElse(x > int64_t(0), [&] { fn.assign(x, expr(int64_t(1))); }, [&] { fn.assign(x, expr(int64_t(-1))); });
    EXPECT_EQ(fn.statements().size(), 1u);
    auto& is = std::get<IfStmt>(fn.statements()[0].var());
    EXPECT_EQ(is.thenBody.size(), 1u);
    EXPECT_EQ(is.elseBody.size(), 1u);
}

TEST(Stmt, NestedWhile)
{
    Function fn("test");
    Variable& i = fn.createInt("i", 0);
    Variable& j = fn.createInt("j", 0);
    fn.whileLoop(i < int64_t(3), [&] {
        fn.whileLoop(j < int64_t(3), [&] { fn.assign(j, add(expr(j), expr(int64_t(1)))); });
        fn.assign(i, add(expr(i), expr(int64_t(1))));
    });
    EXPECT_EQ(fn.statements().size(), 1u);
    auto& outer = std::get<WhileStmt>(fn.statements()[0].var());
    EXPECT_EQ(outer.body.size(), 2u);
    EXPECT_TRUE(std::holds_alternative<WhileStmt>(outer.body[0].var()));
}

TEST(Stmt, VariantAccessorsExposeMutableAndConstViews)
{
    // Given
    Stmt statement{ ReturnStmt{ std::nullopt } };
    EXPECT_TRUE(std::holds_alternative<ReturnStmt>(statement.variant()));

    // When
    std::get<ReturnStmt>(statement.variant()).value = expr(int64_t(3));
    const Stmt& constStatement = statement;

    // Then
    ASSERT_TRUE(std::holds_alternative<ReturnStmt>(constStatement.variant()));
    EXPECT_TRUE(std::get<ReturnStmt>(constStatement.variant()).value.has_value());
}
