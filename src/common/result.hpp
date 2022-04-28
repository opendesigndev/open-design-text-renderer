#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <variant>

template <typename ValueType,
          typename ErrorType,
          typename = std::enable_if_t<!std::is_same<ValueType, ErrorType>::value>>
class Result
{
public:
    template <typename T>
    static Result<ValueType, ErrorType> ok(T&& value)
    {
        return Result<ValueType, ErrorType>(std::forward<T>(value));
    }

    template <typename T>
    static Result<ValueType, ErrorType> fail(T&& value)
    {
        return Result<ValueType, ErrorType>(std::forward<T>(value));
    }

    /* implicit */ Result(ValueType&& value) : result(std::move(value)) {}

    /* implicit */ Result(const ValueType& value) : result(value) {}

    /* implicit */ Result(ErrorType&& value) : result(std::move(value)) {}

    /* implicit */ Result(const ErrorType& value) : result(value) {}

    /* implicit */ operator bool() const { return std::holds_alternative<ValueType>(result); }

    const ValueType& value() const { return std::get<ValueType>(result); }

    ValueType&& moveValue() { return std::move(std::get<ValueType>(result)); }

    const ErrorType& error() const { return std::get<ErrorType>(result); }

private:
    using HolderType = std::variant<ValueType, ErrorType>;

    HolderType result;
};
