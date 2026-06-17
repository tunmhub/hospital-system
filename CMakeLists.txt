cmake_minimum_required(VERSION 3.10)
project(hospital_system VERSION 1.0.0 LANGUAGES CXX)

# ============================================================
# C++17 标准（强制）
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 生成 compile_commands.json（方便 IDE 和 clangd 使用）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# 自定义 CMake 模块路径
# ============================================================
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# ============================================================
# 依赖查找
# ============================================================

# 查找 MySQL 客户端库
find_package(MySQL REQUIRED)

# 查找 pthread（Linux 多线程）
find_package(Threads REQUIRED)

# cpp-httplib：单头文件 HTTP 框架
# 如果系统未安装，尝试用 FetchContent 下载
find_path(HTTPLIB_INCLUDE_DIR "httplib.h")
if(NOT HTTPLIB_INCLUDE_DIR)
    include(FetchContent)
    FetchContent_Declare(
        cpp-httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG        v0.14.3
    )
    FetchContent_MakeAvailable(cpp-httplib)
    set(HTTPLIB_INCLUDE_DIR ${cpp-httplib_SOURCE_DIR})
endif()

# ============================================================
# 源文件收集
# ============================================================
set(SOURCES
    src/main.cpp
    src/di/Container.cpp
    src/db/MySQLConnectionPool.cpp
    src/repository/memory/MemoryPatientRepository.cpp
    src/repository/memory/MemoryDoctorRepository.cpp
    src/repository/memory/MemoryAppointmentRepository.cpp
    src/repository/mysql/MySQLPatientRepository.cpp
    src/repository/mysql/MySQLDoctorRepository.cpp
    src/repository/mysql/MySQLAppointmentRepository.cpp
    src/service/AppointmentService.cpp
    src/api/ApiController.cpp
    src/algorithm/PriorityQueueStrategy.cpp
)

# ============================================================
# 可执行目标
# ============================================================
add_executable(hospital_system ${SOURCES})

target_include_directories(hospital_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${MYSQL_INCLUDE_DIR}
    ${HTTPLIB_INCLUDE_DIR}
)

target_link_libraries(hospital_system PRIVATE
    ${MYSQL_LIBRARIES}
    Threads::Threads
)

# ============================================================
# 编译选项
# ============================================================
target_compile_options(hospital_system PRIVATE
    -Wall -Wextra -Wpedantic
)

# ============================================================
# 安装规则
# ============================================================
install(TARGETS hospital_system DESTINATION bin)
