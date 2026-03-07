#include <asmstudio/api/Function.hpp>

#include <utility>

namespace asmstudio {

Function::Function(std::string name)
    : name_(std::move(name))
{
}

std::string_view Function::name() const noexcept { return name_; }

Variable& Function::createInt(std::string name, int64_t init)
{
    vars_.push_back(std::make_unique<Variable>(std::move(name), DataType::Int32, init));
    return *vars_.back();
}

Variable& Function::createInt64(std::string name, int64_t init)
{
    vars_.push_back(std::make_unique<Variable>(std::move(name), DataType::Int64, init));
    return *vars_.back();
}

std::span<const std::unique_ptr<Variable>> Function::variables() const noexcept { return vars_; }

} // namespace asmstudio
