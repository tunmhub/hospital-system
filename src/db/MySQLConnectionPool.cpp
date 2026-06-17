#include "db/MySQLConnectionPool.h"
#include "common/Exception.h"

#include <iostream>

namespace hospital {

MySQLConnectionPool::MySQLConnectionPool(const MySQLConfig& config)
    : config_(config) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::cout << "[DB] 正在初始化 MySQL 连接池 (" << config_.pool_size << " 个连接)..." << std::endl;

    for (size_t i = 0; i < config_.pool_size; ++i) {
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) {
            throw DatabaseException("mysql_init() 失败: 内存不足");
        }

        // 设置连接超时（秒）
        int timeout = 5;
        mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

        // 建立连接
        if (!mysql_real_connect(conn,
                                config_.host.c_str(),
                                config_.user.c_str(),
                                config_.password.c_str(),
                                config_.database.c_str(),
                                config_.port,
                                nullptr,
                                0)) {
            std::string err = mysql_error(conn);
            mysql_close(conn);
            throw DatabaseException("MySQL 连接失败: " + err);
        }

        // 设置字符集为 utf8mb4（支持中文和 emoji）
        mysql_set_character_set(conn, "utf8mb4");

        connections_.push(conn);
    }

    std::cout << "[DB] MySQL 连接池初始化完成，已创建 "
              << connections_.size() << " 个连接" << std::endl;
}

MySQLConnectionPool::~MySQLConnectionPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_ = true;

    while (!connections_.empty()) {
        mysql_close(connections_.front());
        connections_.pop();
    }

    std::cout << "[DB] MySQL 连接池已关闭" << std::endl;
}

std::shared_ptr<MYSQL> MySQLConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);

    // 等待有可用连接（或池被关闭）
    cv_.wait(lock, [this] {
        return !connections_.empty() || shutdown_;
    });

    if (shutdown_) {
        throw DatabaseException("连接池已关闭");
    }

    // 取出队首连接
    MYSQL* conn = connections_.front();
    connections_.pop();

    // 返回带自定义删除器的 shared_ptr
    // 删除器：将连接归还到池中，而不是 close
    return std::shared_ptr<MYSQL>(conn, [this](MYSQL* c) {
        returnConnection(c);
    });
}

void MySQLConnectionPool::returnConnection(MYSQL* conn) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            mysql_close(conn);
            return;
        }
        connections_.push(conn);
    }
    // 通知一个等待中的线程
    cv_.notify_one();
}

size_t MySQLConnectionPool::available() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}

size_t MySQLConnectionPool::poolSize() const {
    return config_.pool_size;
}

} // namespace hospital
