#include <iostream>
#include <fstream>
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
#include "service/IQueueManagementService.h"
#include "service/IAutoRoutingService.h"
#include "algorithm/IQueueStrategy.h"

// MySQL 连接池
#include "db/MySQLConnectionPool.h"

// MySQL 实现
#include "repository/mysql/MySQLPatientRepository.h"
#include "repository/mysql/MySQLDoctorRepository.h"
#include "repository/mysql/MySQLAppointmentRepository.h"

// Service 实现
#include "service/AppointmentService.h"
#include "service/QueueManagementService.h"
#include "service/AutoRoutingService.h"
#include "service/InsuranceService.h"
#include "service/DoctorLockManager.h"

// 算法策略
#include "algorithm/PriorityQueueStrategy.h"

// API 控制器
#include "api/ApiController.h"

// DI 容器
#include "di/Container.h"

// HTTP 框架
#include <httplib.h>

using namespace hospital;

/// 注册所有依赖绑定
///
/// 使用 shared_ptr<Container> 捕获，避免工厂 lambda 悬垂引用风险。
/// 所有 Service 均注册为单例，保证全局唯一实例。
///
/// 依赖拓扑序：
///   DoctorLockManager → IQueueStrategy
///     → IAutoRoutingService → IQueueManagementService
///       → IAppointmentService → IInsuranceService
void registerDependencies(std::shared_ptr<Container> container, std::shared_ptr<MySQLConnectionPool> pool) {
    // ---- 基础设施 ----
    container->bindInstance<MySQLConnectionPool>(pool);

    // 共享锁管理器（所有需要医生级锁的 Service 共享同一实例）
    container->bindSingleton<DoctorLockManager, DoctorLockManager>();

    // ---- Repository 层 ----
    container->bindSingletonFactory<IPatientRepository>([pool]() {
        return std::make_shared<MySQLPatientRepository>(pool);
    });
    container->bindSingletonFactory<IDoctorRepository>([pool]() {
        return std::make_shared<MySQLDoctorRepository>(pool);
    });
    container->bindSingletonFactory<IAppointmentRepository>([pool]() {
        return std::make_shared<MySQLAppointmentRepository>(pool);
    });

    // ---- 算法策略 ----
    container->bindSingletonFactory<IQueueStrategy>([]() {
        return std::make_shared<PriorityQueueStrategy>();
    });

    // ---- 自动分流服务（纯选医生逻辑，无循环依赖）----
    container->bindSingletonFactory<IAutoRoutingService>([container]() {
        auto doctorRepo      = container->resolve<IDoctorRepository>();
        auto appointmentRepo = container->resolve<IAppointmentRepository>();
        return std::make_shared<AutoRoutingService>(doctorRepo, appointmentRepo);
    });

    // ---- 排队管理服务 ----
    container->bindSingletonFactory<IQueueManagementService>([container]() {
        auto appointmentRepo = container->resolve<IAppointmentRepository>();
        auto doctorRepo      = container->resolve<IDoctorRepository>();
        auto queueStrategy   = container->resolve<IQueueStrategy>();
        auto lockManager     = container->resolve<DoctorLockManager>();
        return std::make_shared<QueueManagementService>(
            appointmentRepo, doctorRepo, queueStrategy, lockManager);
    });

    // ---- 挂号核心服务 ----
    container->bindSingletonFactory<IAppointmentService>([container]() {
        auto patientRepo     = container->resolve<IPatientRepository>();
        auto doctorRepo      = container->resolve<IDoctorRepository>();
        auto appointmentRepo = container->resolve<IAppointmentRepository>();
        auto routingService  = container->resolve<IAutoRoutingService>();
        auto lockManager     = container->resolve<DoctorLockManager>();

        auto svc = std::make_shared<AppointmentService>(
            patientRepo, doctorRepo, appointmentRepo, routingService, lockManager);

        // 延迟注入排队管理服务（打破 AppointmentService ↔ IQueueManagementService 循环依赖）
        svc->setQueueManagementService(container->resolve<IQueueManagementService>());

        return svc;
    });

    // ---- 医保结算服务 ----
    container->bindSingletonFactory<IInsuranceService>([container]() {
        auto patientRepo     = container->resolve<IPatientRepository>();
        auto appointmentRepo = container->resolve<IAppointmentRepository>();
        return std::make_shared<InsuranceService>(
            patientRepo, appointmentRepo);
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

        // ---- 初始化 DI 容器（shared_ptr 管理生命周期，避免工厂 lambda 悬垂引用）----
        auto container = std::make_shared<Container>();
        registerDependencies(container, pool);

        std::cout << "[INFO] 依赖注入容器初始化完成" << std::endl;
        std::cout << "[INFO]   - MySQLConnectionPool     -> " << pool->poolSize() << " 个连接" << std::endl;
        std::cout << "[INFO]   - DoctorLockManager        -> DoctorLockManager (singleton)" << std::endl;
        std::cout << "[INFO]   - IPatientRepository       -> MySQLPatientRepository (singleton)" << std::endl;
        std::cout << "[INFO]   - IDoctorRepository        -> MySQLDoctorRepository (singleton)" << std::endl;
        std::cout << "[INFO]   - IAppointmentRepository   -> MySQLAppointmentRepository (singleton)" << std::endl;
        std::cout << "[INFO]   - IQueueStrategy           -> PriorityQueueStrategy (singleton)" << std::endl;
        std::cout << "[INFO]   - IAutoRoutingService      -> AutoRoutingService (singleton)" << std::endl;
        std::cout << "[INFO]   - IQueueManagementService  -> QueueManagementService (singleton)" << std::endl;
        std::cout << "[INFO]   - IAppointmentService      -> AppointmentService (singleton)" << std::endl;
        std::cout << "[INFO]   - IInsuranceService        -> InsuranceService (singleton)" << std::endl;

        // ---- 解析依赖 ----
        auto patientRepo     = container->resolve<IPatientRepository>();
        auto doctorRepo      = container->resolve<IDoctorRepository>();
        auto appointmentRepo = container->resolve<IAppointmentRepository>();
        auto appointmentSvc  = container->resolve<IAppointmentService>();
        auto queueSvc        = container->resolve<IQueueManagementService>();
        auto insuranceSvc    = container->resolve<IInsuranceService>();

        std::cout << "[INFO] 所有依赖解析成功" << std::endl;

        // ---- 初始化 API 控制器 ----
        ApiController apiController(
            patientRepo, doctorRepo, appointmentRepo, appointmentSvc, queueSvc, insuranceSvc);

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

        // SPA fallback：非 API 请求且文件不存在时，返回 index.html（支持 Vue Router History 模式）
        server.set_error_handler([](const httplib::Request& req, httplib::Response& res) {
            if (res.status == 404 && req.path.find("/api/") == std::string::npos) {
                std::ifstream file("./public/index.html");
                if (file.is_open()) {
                    std::string content((std::istreambuf_iterator<char>(file)),
                                         std::istreambuf_iterator<char>());
                    res.set_content(content, "text/html");
                    res.status = 200;
                }
            }
        });

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
