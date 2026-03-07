#include <asmstudio/api/Variable.hpp>

#include <utility>

namespace asmstudio {

Variable::Variable(std::string name, DataType type, int64_t init)
    : name_(std::move(name))
    , type_(type)
    , init_(init)
{
}

std::string_view Variable::name() const noexcept { return name_; }

DataType Variable::type() const noexcept { return type_; }

int64_t Variable::initialValue() const noexcept { return init_; }

} // namespace asmstudio
