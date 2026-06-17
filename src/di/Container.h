#pragma once

#include <any>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace hospital {

/// 轻量级依赖注入容器
///
/// 使用 std::any 存储工厂函数，支持按类型注册和解析。
///
/// 用法示例：
///   Container container;
///
///   // 注册：接口 → 具体实现的工厂函数
///   container.bind<IPatientRepository>([]() {
///       return std::make_shared<MemoryPatientRepository>();
///   });
///
///   // 解析：获取接口的实例
///   auto repo = container.resolve<IPatientRepository>();
class Container {
public:
    /// 注册一个类型绑定：Interface → Implementation 的工厂函数
    ///
    /// @tparam Interface     接口类型（如 IPatientRepository）
    /// @tparam Implementation 具体实现类型（如 MemoryPatientRepository）
    template<typename Interface, typename Implementation>
    void bind() {
        // 必须显式转为 std::function，否则 lambda 存入 std::any 后
        // any_cast 无法匹配实际的 lambda 类型
        std::function<std::shared_ptr<Interface>()> factory = []() -> std::shared_ptr<Interface> {
            return std::make_shared<Implementation>();
        };
        bindings_[std::type_index(typeid(Interface))] = factory;
    }

    /// 注册一个类型绑定：使用自定义工厂函数
    ///
    /// @tparam Interface  接口类型
    /// @param factory     工厂函数，返回 shared_ptr<Interface>
    template<typename Interface>
    void bindFactory(std::function<std::shared_ptr<Interface>()> factory) {
        bindings_[std::type_index(typeid(Interface))] = std::move(factory);
    }

    /// 注册一个已有实例（单例模式）
    ///
    /// @tparam Interface  接口类型
    /// @param instance    已创建的实例
    template<typename Interface>
    void bindInstance(std::shared_ptr<Interface> instance) {
        std::function<std::shared_ptr<Interface>()> factory =
            [instance = std::move(instance)]() -> std::shared_ptr<Interface> {
                return instance;
            };
        bindings_[std::type_index(typeid(Interface))] = factory;
    }

    /// 解析一个类型，返回对应的实例
    ///
    /// @tparam Interface  接口类型
    /// @return 对应实现的 shared_ptr
    /// @throws std::runtime_error 如果未注册该类型
    template<typename Interface>
    std::shared_ptr<Interface> resolve() const {
        auto it = bindings_.find(std::type_index(typeid(Interface)));
        if (it == bindings_.end()) {
            throw std::runtime_error(
                std::string("Container: 类型未注册 -> ") + typeid(Interface).name()
            );
        }

        // 从 any 中取出工厂函数并调用
        auto factory = std::any_cast<std::function<std::shared_ptr<Interface>()>>(it->second);
        return factory();
    }

    /// 检查某个类型是否已注册
    template<typename Interface>
    bool has() const {
        return bindings_.find(std::type_index(typeid(Interface))) != bindings_.end();
    }

    /// 清除所有注册（主要用于测试）
    void clear() { bindings_.clear(); }

private:
    /// 存储：类型索引 → 工厂函数（用 std::any 包装不同签名的 function）
    std::unordered_map<std::type_index, std::any> bindings_;
};

} // namespace hospital
