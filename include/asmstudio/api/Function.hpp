#pragma once

#include <asmstudio/api/Variable.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace asmstudio {

class Function
{
public:
    explicit Function(std::string name);

    [[nodiscard]] std::string_view name() const noexcept;

    Variable& createInt(std::string name, int64_t init = 0);
    Variable& createInt64(std::string name, int64_t init = 0);

    [[nodiscard]] std::span<const std::unique_ptr<Variable>> variables() const noexcept;

    // whileLoop / ifStmt / returnValue — Milestone 2.

private:
    std::string                             name_;
    std::vector<std::unique_ptr<Variable>>  vars_;
};

} // namespace asmstudio
