#pragma once

#include <string>
#include <optional>
#include <stdexcept>

namespace hospital {

/// 统一业务结果封装
/// 用于 Service 层向 Controller 层返回操作结果，避免直接抛异常
///
/// 用法示例：
///   auto result = Result::success(patient);
///   if (result.ok()) { ... }
///
///   auto err = Result<Patient>::failure("患者不存在");
///   if (!err.ok()) { std::cout << err.errorMessage(); }
template<typename T = void>
class Result {
public:
    /// 构造成功结果
    static Result success(T value) {
        Result r;
        r.value_ = std::move(value);
        r.ok_ = true;
        return r;
    }

    /// 构造失败结果
    static Result failure(std::string message) {
        Result r;
        r.ok_ = false;
        r.error_message_ = std::move(message);
        return r;
    }

    /// 是否成功
    bool ok() const { return ok_; }

    /// 获取值（成功时调用）
    /// @throws std::runtime_error 如果结果是失败状态
    const T& value() const {
        if (!ok_) throw std::runtime_error("访问失败结果的值: " + error_message_);
        return value_.value();
    }
    T& value() {
        if (!ok_) throw std::runtime_error("访问失败结果的值: " + error_message_);
        return value_.value();
    }

    /// 获取错误信息（失败时调用）
    const std::string& errorMessage() const { return error_message_; }

private:
    bool ok_ = false;
    std::optional<T> value_;
    std::string error_message_;
};

/// void 特化：不携带返回值的 Result
template<>
class Result<void> {
public:
    static Result success() {
        Result r;
        r.ok_ = true;
        return r;
    }

    static Result failure(std::string message) {
        Result r;
        r.ok_ = false;
        r.error_message_ = std::move(message);
        return r;
    }

    bool ok() const { return ok_; }
    const std::string& errorMessage() const { return error_message_; }

private:
    bool ok_ = false;
    std::string error_message_;
};

} // namespace hospital
