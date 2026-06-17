#pragma once

#include <mysql/mysql.h>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace hospital {

/// MySQL 连接池配置
struct MySQLConfig {
    std::string host = "127.0.0.1";
    unsigned int port = 3306;
    std::string user = "root";
    std::string password = "";
    std::string database = "hospital_db";
    size_t pool_size = 10;  ///< 连接池大小
};

/// MySQL 连接池
///
/// 设计要点：
/// 1. 构造时预创建 pool_size 个 MYSQL* 连接存入队列
/// 2. getConnection() 返回带自定义删除器的 shared_ptr<MYSQL>，
///    析构时自动归还连接到池中，而不是 close
/// 3. 使用 mutex + condition_variable 保证线程安全
///
/// 用法：
///   auto pool = std::make_shared<MySQLConnectionPool>(config);
///   auto conn = pool->getConnection();  // 获取连接
///   mysql_query(conn.get(), "SELECT ...");
///   // conn 离开作用域时自动归还
class MySQLConnectionPool {
public:
    /// 从配置构造连接池，初始化时创建所有连接
    explicit MySQLConnectionPool(const MySQLConfig& config);

    /// 析构时关闭所有连接
    ~MySQLConnectionPool();

    // 禁止拷贝和移动
    MySQLConnectionPool(const MySQLConnectionPool&) = delete;
    MySQLConnectionPool& operator=(const MySQLConnectionPool&) = delete;

    /// 获取一个连接（自动归还的 RAII 智能指针）
    ///
    /// 如果池中无可用连接，会阻塞等待直到有连接归还。
    /// 返回的 shared_ptr 离开作用域后连接自动归还池中。
    std::shared_ptr<MYSQL> getConnection();

    /// 获取当前可用连接数（用于监控和调试）
    size_t available() const;

    /// 获取池的总连接数
    size_t poolSize() const;

private:
    /// 将连接归还到池中（由自定义删除器调用）
    void returnConnection(MYSQL* conn);

    MySQLConfig config_;
    std::queue<MYSQL*> connections_;          ///< 可用连接队列
    mutable std::mutex mutex_;
    std::condition_variable cv_;              ///< 等待可用连接
    bool shutdown_ = false;                   ///< 关闭标志
};

} // namespace hospital
