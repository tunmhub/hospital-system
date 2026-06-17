#pragma once

#include "repository/IPatientRepository.h"
#include "repository/IDoctorRepository.h"
#include "repository/IAppointmentRepository.h"
#include "service/IAppointmentService.h"

#include <httplib.h>
#include <memory>

namespace hospital {

/// API 路由控制器
///
/// 职责：
/// 1. 注册所有 HTTP 路由
/// 2. 解析 JSON 请求体
/// 3. 调用 Service 层处理业务
/// 4. 组装 JSON 响应
/// 5. 统一异常捕获，返回 HTTP 400 + 错误 JSON
class ApiController {
public:
    ApiController(
        std::shared_ptr<IPatientRepository> patientRepo,
        std::shared_ptr<IDoctorRepository> doctorRepo,
        std::shared_ptr<IAppointmentRepository> appointmentRepo,
        std::shared_ptr<IAppointmentService> appointmentService
    );

    /// 将所有路由注册到 httplib::Server
    void registerRoutes(httplib::Server& server);

private:
    // ---- 患者相关 ----
    void handleCreatePatient(const httplib::Request& req, httplib::Response& res);
    void handleGetPatient(const httplib::Request& req, httplib::Response& res);
    void handleListPatients(const httplib::Request& req, httplib::Response& res);
    void handleSearchPatients(const httplib::Request& req, httplib::Response& res);

    // ---- 医生相关 ----
    void handleListDoctors(const httplib::Request& req, httplib::Response& res);

    // ---- 挂号相关 ----
    void handleMakeAppointment(const httplib::Request& req, httplib::Response& res);
    void handleCancelAppointment(const httplib::Request& req, httplib::Response& res);
    void handleGetPatientAppointments(const httplib::Request& req, httplib::Response& res);
    void handleGetDoctorQueue(const httplib::Request& req, httplib::Response& res);
    void handleAutoRouteAppointment(const httplib::Request& req, httplib::Response& res);
    void handleCallNextPatient(const httplib::Request& req, httplib::Response& res);
    void handleCompleteAppointment(const httplib::Request& req, httplib::Response& res);
    void handleEstimateWaitTime(const httplib::Request& req, httplib::Response& res);

    // ---- 工具方法 ----
    /// 设置 JSON 响应头并写入 body
    static void setJsonResponse(httplib::Response& res, int status, const std::string& json);
    /// 返回错误 JSON：{"error": "message"}
    static void setErrorResponse(httplib::Response& res, int status, const std::string& message);

    std::shared_ptr<IPatientRepository> patientRepo_;
    std::shared_ptr<IDoctorRepository> doctorRepo_;
    std::shared_ptr<IAppointmentRepository> appointmentRepo_;
    std::shared_ptr<IAppointmentService> appointmentService_;
};

} // namespace hospital
