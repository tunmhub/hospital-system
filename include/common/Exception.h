#pragma once

#include <stdexcept>
#include <string>

namespace hospital {

/// 自定义异常基类
class HospitalException : public std::exception {
public:
    explicit HospitalException(std::string message)
        : message_(std::move(message)) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

/// 数据库异常
class DatabaseException : public HospitalException {
public:
    explicit DatabaseException(std::string message)
        : HospitalException(std::move(message)) {}
};

/// 队列已满异常
class QueueFullException : public HospitalException {
public:
    explicit QueueFullException(std::string message)
        : HospitalException(std::move(message)) {}
};

/// 资源未找到异常
class NotFoundException : public HospitalException {
public:
    explicit NotFoundException(std::string message)
        : HospitalException(std::move(message)) {}
};

/// 参数校验异常
class ValidationException : public HospitalException {
public:
    explicit ValidationException(std::string message)
        : HospitalException(std::move(message)) {}
};

} // namespace hospital
