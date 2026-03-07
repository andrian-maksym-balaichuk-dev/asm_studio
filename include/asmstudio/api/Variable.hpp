#pragma once

#include <asmstudio/core/Types.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace asmstudio {

class Variable
{
public:
    Variable(std::string name, DataType type, int64_t init);

    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] DataType         type() const noexcept;
    [[nodiscard]] int64_t          initialValue() const noexcept;

private:
    std::string name_;
    DataType    type_;
    int64_t     init_;
};

} // namespace asmstudio
