#include <iostream>
#include <memory>
#include <cstdlib>

// 公共工具
#include "common/Exception.h"

// 数据模型
#include "model/Patient.h"
#include "model/Doctor.h"
#include "model/Appointment.h"

// 接口层
#include "repository/IPatientRepository.h"
#include "repository/IDoctorRepository.h"
#include "repository/IAppointmentRepository.h"
#include "service/IAppointmentService.h"

// MySQL 连接池
#include "db/MySQLConnectionPool.h"

// MySQL 实现
#include "repository/mysql/MySQLPatientRepository.h"
#include "repository/mysql/MySQLDoctorRepository.h"
#include "repository/mysql/MySQLAppointmentRepository.h"

// Service 实现
#include "service/AppointmentService.h"

// API 控制器
#include "api/ApiController.h"

// DI 容器
#include "di/Container.h"

// HTTP 框架
#include <httplib.h>

using namespace hospital;

/// 注册所有依赖绑定
void registerDependencies(Container& container, std::shared_ptr<MySQLConnectionPool> pool) {
    // 连接池（单例）
    container.bindInstance<MySQLConnectionPool>(pool);

    // Repository 层
    container.bindFactory<IPatientRepository>([pool]() {
        return std::make_shared<MySQLPatientRepository>(pool);
    });
    container.bindFactory<IDoctorRepository>([pool]() {
        return std::make_shared<MySQLDoctorRepository>(pool);
    });
    container.bindFactory<IAppointmentRepository>([pool]() {
        return std::make_shared<MySQLAppointmentRepository>(pool);
    });

    // Service 层：注入三个 Repository
    container.bindFactory<IAppointmentService>([&container]() {
        auto patientRepo     = container.resolve<IPatientRepository>();
        auto doctorRepo      = container.resolve<IDoctorRepository>();
        auto appointmentRepo = container.resolve<IAppointmentRepository>();
        return std::make_shared<AppointmentService>(
            patientRepo, doctorRepo, appointmentRepo);
    });
}

/// 打印启动横幅
void printBanner() {
    std::cout << R"(
  ╔═══════════════════════════════════════════╗
  ║       🏥 医院挂号系统 v1.0.0             ║
  ║       Hospital Registration System        ║
  ╠═══════════════════════════════════════════╣
  ║  技术栈: C++17 + cpp-httplib + MySQL      ║
  ║  架构:   前后端分离 · 三层架构            ║
  ╚═══════════════════════════════════════════╝
)" << std::endl;
}

int main() {
    printBanner();

    try {
        // ---- 初始化 MySQL 连接池 ----
        MySQLConfig config;
        config.host      = std::getenv("MYSQL_HOST")     ? std::getenv("MYSQL_HOST")     : "127.0.0.1";
        config.port      = std::getenv("MYSQL_PORT")     ? std::atoi(std::getenv("MYSQL_PORT")) : 3306;
        config.user      = std::getenv("MYSQL_USER")     ? std::getenv("MYSQL_USER")     : "root";
        config.password  = std::getenv("MYSQL_PASSWORD") ? std::getenv("MYSQL_PASSWORD") : "";
        config.database  = std::getenv("MYSQL_DATABASE") ? std::getenv("MYSQL_DATABASE") : "hospital_db";
        config.pool_size = 10;

        auto pool = std::make_shared<MySQLConnectionPool>(config);

        // ---- 初始化 DI 容器 ----
        Container container;
        registerDependencies(container, pool);

        std::cout << "[INFO] 依赖注入容器初始化完成" << std::endl;
        std::cout << "[INFO]   - MySQLConnectionPool     -> " << pool->poolSize() << " 个连接" << std::endl;
        std::cout << "[INFO]   - IPatientRepository       -> MySQLPatientRepository" << std::endl;
        std::cout << "[INFO]   - IDoctorRepository        -> MySQLDoctorRepository" << std::endl;
        std::cout << "[INFO]   - IAppointmentRepository   -> MySQLAppointmentRepository" << std::endl;
        std::cout << "[INFO]   - IAppointmentService      -> AppointmentService" << std::endl;

        // ---- 解析依赖 ----
        auto patientRepo     = container.resolve<IPatientRepository>();
        auto doctorRepo      = container.resolve<IDoctorRepository>();
        auto appointmentRepo = container.resolve<IAppointmentRepository>();
        auto appointmentSvc  = container.resolve<IAppointmentService>();

        std::cout << "[INFO] 所有依赖解析成功" << std::endl;

        // ---- 初始化 API 控制器 ----
        ApiController apiController(
            patientRepo, doctorRepo, appointmentRepo, appointmentSvc);

        // ---- 启动 HTTP 服务器 ----
        httplib::Server server;

        // 注册 API 路由
        apiController.registerRoutes(server);

        // 健康检查
        server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(R"({"status":"ok","service":"hospital_system"})", "application/json");
        });

        // 统一异常处理中间件
        server.set_exception_handler([](const httplib::Request& /*req*/, httplib::Response& res, std::exception_ptr ep) {
            try {
                if (ep) std::rethrow_exception(ep);
            } catch (const ValidationException& e) {
                res.status = 400;
                res.set_content(R"({"error":")" + std::string(e.what()) + R"("})", "application/json");
            } catch (const NotFoundException& e) {
                res.status = 404;
                res.set_content(R"({"error":")" + std::string(e.what()) + R"("})", "application/json");
            } catch (const DatabaseException& e) {
                std::cerr << "[DB ERROR] " << e.what() << std::endl;
                res.status = 500;
                res.set_content(R"({"error":"数据库错误"})", "application/json");
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] " << e.what() << std::endl;
                res.status = 500;
                res.set_content(R"({"error":"服务器内部错误"})", "application/json");
            } catch (...) {
                std::cerr << "[ERROR] 未知异常" << std::endl;
                res.status = 500;
                res.set_content(R"({"error":"服务器内部错误"})", "application/json");
            }
        });

        // 托管前端静态文件
        if (server.set_mount_point("/", "./public")) {
            std::cout << "[INFO] 静态文件托管: ./public/" << std::endl;
        } else {
            std::cout << "[WARN] 静态文件目录 ./public 不存在，前端页面无法访问" << std::endl;
            std::cout << "[WARN] 请确保在项目根目录运行，或创建 public/ 目录的符号链接" << std::endl;
        }

        int port = std::getenv("SERVER_PORT") ? std::atoi(std::getenv("SERVER_PORT")) : 8080;
        std::cout << std::endl;
        std::cout << "[INFO] HTTP 服务器启动中..." << std::endl;
        std::cout << "[INFO] 监听地址: http://0.0.0.0:" << port << std::endl;
        std::cout << "[INFO] 按 Ctrl+C 停止服务器" << std::endl;
        std::cout << std::endl;

        if (!server.listen("0.0.0.0", port)) {
            std::cerr << "[ERROR] 服务器启动失败，端口 " << port << " 可能被占用" << std::endl;
            return 1;
        }

    } catch (const DatabaseException& e) {
        std::cerr << "[DB ERROR] " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
