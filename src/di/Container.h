#pragma once

#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace hospital {

/// 轻量级依赖注入容器
///
/// 支持三种生命周期模式：
///   - bind<Interface, Impl>()            — 瞬态：每次 resolve 创建新实例
///   - bindSingleton<Interface, Impl>()   — 单例：首次 resolve 创建，后续复用
///   - bindSingletonFactory<Interface>(f)  — 单例工厂：首次 resolve 调用工厂，后续复用
///   - bindInstance<Interface>(ptr)        — 单例实例：直接绑定已有对象
///
/// 用法示例：
///   Container container;
///   container.bindSingleton<IPatientRepository, MySQLPatientRepository>();
///   auto repo = container.resolve<IPatientRepository>();
class Container {
public:
    /// 注册瞬态绑定：Interface → Implementation（每次 resolve 创建新实例）
    template<typename Interface, typename Implementation>
    void bind() {
        std::function<std::shared_ptr<Interface>()> factory = []() -> std::shared_ptr<Interface> {
            return std::make_shared<Implementation>();
        };
        bindings_[std::type_index(typeid(Interface))] = factory;
    }

    /// 注册单例绑定：首次 resolve 时创建，后续返回同一实例（线程安全）
    template<typename Interface, typename Implementation>
    void bindSingleton() {
        auto cache = std::make_shared<std::shared_ptr<Interface>>(nullptr);
        auto mtx = std::make_shared<std::mutex>();
        std::function<std::shared_ptr<Interface>()> factory =
            [cache, mtx]() -> std::shared_ptr<Interface> {
                std::lock_guard<std::mutex> lock(*mtx);
                if (!*cache) {
                    *cache = std::make_shared<Implementation>();
                }
                return *cache;
            };
        bindings_[std::type_index(typeid(Interface))] = factory;
    }

    /// 注册瞬态工厂函数（每次 resolve 调用工厂创建新实例）
    template<typename Interface>
    void bindFactory(std::function<std::shared_ptr<Interface>()> factory) {
        bindings_[std::type_index(typeid(Interface))] = std::move(factory);
    }

    /// 注册单例工厂函数（首次 resolve 调用工厂，后续返回缓存实例，线程安全）
    template<typename Interface>
    void bindSingletonFactory(std::function<std::shared_ptr<Interface>()> factory) {
        auto cache = std::make_shared<std::shared_ptr<Interface>>(nullptr);
        auto mtx = std::make_shared<std::mutex>();
        auto factoryPtr = std::make_shared<std::function<std::shared_ptr<Interface>()>>(std::move(factory));
        std::function<std::shared_ptr<Interface>()> wrapper =
            [cache, mtx, factoryPtr]() -> std::shared_ptr<Interface> {
                std::lock_guard<std::mutex> lock(*mtx);
                if (!*cache) {
                    *cache = (*factoryPtr)();
                }
                return *cache;
            };
        bindings_[std::type_index(typeid(Interface))] = wrapper;
    }

    /// 注册已有实例（单例语义：始终返回同一个 shared_ptr）
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
