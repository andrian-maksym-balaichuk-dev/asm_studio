#pragma once

#include <cstdint>

// Stub — Milestone 2 will flesh this out with comparison operators
// and integration with Variable to build typed Condition objects
// usable in Function::whileLoop and Function::ifStmt.

namespace asmstudio {

class Variable;

enum class ConditionKind
{
    LessThan,
    LessEqual,
    Equal,
    NotEqual,
    GreaterEqual,
    GreaterThan,
};

struct Condition
{
    const Variable* lhs{};
    ConditionKind   kind{};
    const Variable* rhs{};
    int64_t         rhsImm{};
    bool            rhsIsImmediate{false};
};

} // namespace asmstudio
