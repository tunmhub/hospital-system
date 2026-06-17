#include "api/ApiController.h"
#include "common/Exception.h"

#include <json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace hospital {

ApiController::ApiController(
    std::shared_ptr<IPatientRepository> patientRepo,
    std::shared_ptr<IDoctorRepository> doctorRepo,
    std::shared_ptr<IAppointmentRepository> appointmentRepo,
    std::shared_ptr<IAppointmentService> appointmentService)
    : patientRepo_(std::move(patientRepo))
    , doctorRepo_(std::move(doctorRepo))
    , appointmentRepo_(std::move(appointmentRepo))
    , appointmentService_(std::move(appointmentService)) {}

void ApiController::registerRoutes(httplib::Server& server) {
    // ---- 患者 ----
    server.Post("/api/patients",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleCreatePatient(req, res);
        });

    server.Get("/api/patients",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleListPatients(req, res);
        });

    server.Get(R"(/api/patients/(\d+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetPatient(req, res);
        });

    server.Get(R"(/api/patients/(\d+)/appointments)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetPatientAppointments(req, res);
        });

    // ---- 医生 ----
    server.Get("/api/doctors",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleListDoctors(req, res);
        });

    server.Get(R"(/api/doctors/(\d+)/queue)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetDoctorQueue(req, res);
        });

    // ---- 挂号 ----
    server.Post("/api/appointments",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleMakeAppointment(req, res);
        });

    server.Post(R"(/api/appointments/(\d+)/cancel)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleCancelAppointment(req, res);
        });

    // ---- Step 4: 自动分流 & 叫号 & 等待时间 ----
    server.Post("/api/appointments/auto",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleAutoRouteAppointment(req, res);
        });

    server.Post(R"(/api/doctors/(\d+)/call_next)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleCallNextPatient(req, res);
        });

    server.Get(R"(/api/appointments/(\d+)/wait_time)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleEstimateWaitTime(req, res);
        });

    std::cout << "[API] 路由注册完成:" << std::endl;
    std::cout << "[API]   POST /api/patients              - 注册患者" << std::endl;
    std::cout << "[API]   GET  /api/patients               - 患者列表" << std::endl;
    std::cout << "[API]   GET  /api/patients/:id           - 查询患者" << std::endl;
    std::cout << "[API]   GET  /api/patients/:id/appointments - 患者挂号记录" << std::endl;
    std::cout << "[API]   GET  /api/doctors                - 医生列表" << std::endl;
    std::cout << "[API]   GET  /api/doctors/:id/queue      - 医生排队队列" << std::endl;
    std::cout << "[API]   POST /api/appointments           - 发起挂号" << std::endl;
    std::cout << "[API]   POST /api/appointments/:id/cancel - 取消挂号" << std::endl;
    std::cout << "[API]   POST /api/appointments/auto      - 自动分流挂号" << std::endl;
    std::cout << "[API]   POST /api/doctors/:id/call_next  - 呼叫下一位" << std::endl;
    std::cout << "[API]   GET  /api/appointments/:id/wait_time - 预估等待时间" << std::endl;
}

// ============================================================
// 患者接口
// ============================================================

void ApiController::handleCreatePatient(const httplib::Request& req, httplib::Response& res) {
    try {
        auto body = json::parse(req.body);

        // 参数校验
        if (!body.contains("name") || !body.contains("phone")) {
            setErrorResponse(res, 400, "缺少必填字段: name, phone");
            return;
        }

        Patient patient;
        patient.name    = body["name"].get<std::string>();
        patient.phone   = body["phone"].get<std::string>();
        patient.id_card = body.value("id_card", "");
        patient.age     = body.value("age", 0);
        patient.gender  = body.value("gender", "");

        if (!patientRepo_->save(patient)) {
            setErrorResponse(res, 500, "患者创建失败");
            return;
        }

        json result = {
            {"id",         patient.id},
            {"name",       patient.name},
            {"phone",      patient.phone},
            {"id_card",    patient.id_card},
            {"age",        patient.age},
            {"gender",     patient.gender},
            {"created_at", patient.created_at}
        };
        setJsonResponse(res, 201, result.dump());

    } catch (const json::exception& e) {
        setErrorResponse(res, 400, std::string("JSON 解析错误: ") + e.what());
    } catch (const HospitalException& e) {
        setErrorResponse(res, 400, e.what());
    } catch (const std::exception& e) {
        setErrorResponse(res, 500, std::string("服务器内部错误: ") + e.what());
    }
}

void ApiController::handleGetPatient(const httplib::Request& req, httplib::Response& res) {
    try {
        int64_t id = std::stoll(req.matches[1]);
        auto patient = patientRepo_->findById(id);

        if (!patient) {
            setErrorResponse(res, 404, "患者不存在");
            return;
        }

        json result = {
            {"id",         patient->id},
            {"name",       patient->name},
            {"phone",      patient->phone},
            {"id_card",    patient->id_card},
            {"age",        patient->age},
            {"gender",     patient->gender},
            {"created_at", patient->created_at}
        };
        setJsonResponse(res, 200, result.dump());

    } catch (const std::exception& e) {
        setErrorResponse(res, 400, e.what());
    }
}

void ApiController::handleListPatients(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
        auto patients = patientRepo_->findAll();

        json arr = json::array();
        for (const auto& p : patients) {
            arr.push_back({
                {"id",         p.id},
                {"name",       p.name},
                {"phone",      p.phone},
                {"age",        p.age},
                {"gender",     p.gender}
            });
        }
        setJsonResponse(res, 200, arr.dump());

    } catch (const std::exception& e) {
        setErrorResponse(res, 500, std::string("服务器内部错误: ") + e.what());
    }
}

void ApiController::handleGetPatientAppointments(const httplib::Request& req, httplib::Response& res) {
    try {
        int64_t patientId = std::stoll(req.matches[1]);
        auto result = appointmentService_->getAppointmentsByPatient(patientId);

        if (!result.ok()) {
            setErrorResponse(res, 400, result.errorMessage());
            return;
        }

        json arr = json::array();
        for (const auto& apt : result.value()) {
            arr.push_back({
                {"id",           apt.id},
                {"patient_id",   apt.patient_id},
                {"doctor_id",    apt.doctor_id},
                {"status",       statusToString(apt.status)},
                {"priority",     priorityToString(apt.priority)},
                {"queue_number", apt.queue_number},
                {"created_at",   apt.created_at}
            });
        }
        setJsonResponse(res, 200, arr.dump());

    } catch (const std::exception& e) {
        setErrorResponse(res, 400, e.what());
    }
}

// ============================================================
// 医生接口
// ============================================================

void ApiController::handleListDoctors(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
        auto doctors = doctorRepo_->findAll();

        json arr = json::array();
        for (const auto& doc : doctors) {
            arr.push_back({
                {"id",               doc.id},
                {"name",             doc.name},
                {"department",       doc.department},
                {"title",            doc.title},
                {"max_patients",     doc.max_patients},
                {"current_patients", doc.current_patients}
            });
        }
        setJsonResponse(res, 200, arr.dump());

    } catch (const std::exception& e) {
        setErrorResponse(res, 500, std::string("服务器内部错误: ") + e.what());
    }
}

void ApiController::handleGetDoctorQueue(const httplib::Request& req, httplib::Response& res) {
    try {
        int64_t doctorId = std::stoll(req.matches[1]);
        auto result = appointmentService_->getQueueByDoctor(doctorId);

        if (!result.ok()) {
            setErrorResponse(res, 400, result.errorMessage());
            return;
        }

        json arr = json::array();
        for (const auto& apt : result.value()) {
            arr.push_back({
                {"id",           apt.id},
                {"patient_id",   apt.patient_id},
                {"doctor_id",    apt.doctor_id},
                {"status",       statusToString(apt.status)},
                {"priority",     priorityToString(apt.priority)},
                {"queue_number", apt.queue_number},
                {"created_at",   apt.created_at}
            });
        }
        setJsonResponse(res, 200, arr.dump());

    } catch (const std::exception& e) {
        setErrorResponse(res, 400, e.what());
    }
}

// ============================================================
// 挂号接口
// ============================================================

void ApiController::handleMakeAppointment(const httplib::Request& req, httplib::Response& res) {
    try {
        auto body = json::parse(req.body);

        if (!body.contains("patient_id") || !body.contains("doctor_id")) {
            setErrorResponse(res, 400, "缺少必填字段: patient_id, doctor_id");
            return;
        }

        int64_t patientId = body["patient_id"].get<int64_t>();
        int64_t doctorId  = body["doctor_id"].get<int64_t>();

        // 解析可选的优先级字段
        Priority priority = Priority::Normal;
        if (body.contains("priority")) {
            std::string p = body["priority"].get<std::string>();
            if (p == "urgent")        priority = Priority::Urgent;
            else if (p == "emergency") priority = Priority::Emergency;
        }

        auto result = appointmentService_->makeAppointment(patientId, doctorId, priority);

        if (!result.ok()) {
            setErrorResponse(res, 400, result.errorMessage());
            return;
        }

        const auto& apt = result.value();
        json resp = {
            {"id",           apt.id},
            {"patient_id",   apt.patient_id},
            {"doctor_id",    apt.doctor_id},
            {"status",       statusToString(apt.status)},
            {"priority",     priorityToString(apt.priority)},
            {"queue_number", apt.queue_number},
            {"created_at",   apt.created_at}
        };
        setJsonResponse(res, 201, resp.dump());

    } catch (const json::exception& e) {
        setErrorResponse(res, 400, std::string("JSON 解析错误: ") + e.what());
    } catch (const HospitalException& e) {
        setErrorResponse(res, 400, e.what());
    } catch (const std::exception& e) {
        setErrorResponse(res, 500, std::string("服务器内部错误: ") + e.what());
    }
}

void ApiController::handleCancelAppointment(const httplib::Request& req, httplib::Response& res) {
    try {
        int64_t appointmentId = std::stoll(req.matches[1]);
        auto result = appointmentService_->cancelAppointment(appointmentId);

        if (!result.ok()) {
            setErrorResponse(res, 400, result.errorMessage());
            return;
        }

        json resp = {{"message", "挂号已取消"}};
        setJsonResponse(res, 200, resp.dump());

    } catch (const std::exception& e) {
        setErrorResponse(res, 400, e.what());
    }
}

// ============================================================
// Step 4: 自动分流 & 叫号 & 等待时间
// ============================================================

void ApiController::handleAutoRouteAppointment(const httplib::Request& req, httplib::Response& res) {
    try {
        auto body = json::parse(req.body);

        if (!body.contains("patient_id") || !body.contains("department")) {
            setErrorResponse(res, 400, "缺少必填字段: patient_id, department");
            return;
        }

        int64_t patientId = body["patient_id"].get<int64_t>();
        std::string department = body["department"].get<std::string>();

        Priority priority = Priority::Normal;
        if (body.contains("priority")) {
            std::string p = body["priority"].get<std::string>();
            if (p == "urgent")        priority = Priority::Urgent;
            else if (p == "emergency") priority = Priority::Emergency;
        }

        auto result = appointmentService_->autoRouteAppointment(patientId, department, priority);

        if (!result.ok()) {
            setErrorResponse(res, 400, result.errorMessage());
            return;
        }

        const auto& apt = result.value();
        json resp = {
            {"id",           apt.id},
            {"patient_id",   apt.patient_id},
            {"doctor_id",    apt.doctor_id},
            {"status",       statusToString(apt.status)},
            {"priority",     priorityToString(apt.priority)},
            {"queue_number", apt.queue_number},
            {"created_at",   apt.created_at}
        };
        setJsonResponse(res, 201, resp.dump());

    } catch (const json::exception& e) {
        setErrorResponse(res, 400, std::string("JSON 解析错误: ") + e.what());
    } catch (const std::exception& e) {
        setErrorResponse(res, 500, std::string("服务器内部错误: ") + e.what());
    }
}

void ApiController::handleCallNextPatient(const httplib::Request& req, httplib::Response& res) {
    try {
        int64_t doctorId = std::stoll(req.matches[1]);
        auto result = appointmentService_->callNextPatient(doctorId);

        if (!result.ok()) {
            setErrorResponse(res, 400, result.errorMessage());
            return;
        }

        const auto& apt = result.value();
        json resp = {
            {"id",           apt.id},
            {"patient_id",   apt.patient_id},
            {"doctor_id",    apt.doctor_id},
            {"status",       statusToString(apt.status)},
            {"priority",     priorityToString(apt.priority)},
            {"queue_number", apt.queue_number},
            {"created_at",   apt.created_at}
        };
        setJsonResponse(res, 200, resp.dump());

    } catch (const std::exception& e) {
        setErrorResponse(res, 400, e.what());
    }
}

void ApiController::handleEstimateWaitTime(const httplib::Request& req, httplib::Response& res) {
    try {
        int64_t appointmentId = std::stoll(req.matches[1]);
        auto result = appointmentService_->estimateWaitTime(appointmentId);

        if (!result.ok()) {
            setErrorResponse(res, 400, result.errorMessage());
            return;
        }

        json resp = {
            {"appointment_id", appointmentId},
            {"wait_minutes",   result.value()}
        };
        setJsonResponse(res, 200, resp.dump());

    } catch (const std::exception& e) {
        setErrorResponse(res, 400, e.what());
    }
}

// ============================================================
// 工具方法
// ============================================================

void ApiController::setJsonResponse(httplib::Response& res, int status, const std::string& json) {
    res.status = status;
    res.set_content(json, "application/json; charset=utf-8");
}

void ApiController::setErrorResponse(httplib::Response& res, int status, const std::string& message) {
    json err = {{"error", message}};
    setJsonResponse(res, status, err.dump());
}

} // namespace hospital
