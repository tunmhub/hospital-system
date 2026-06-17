# FindMySQL.cmake
# 查找 MySQL 客户端库和头文件
#
# 定义以下变量：
#   MYSQL_FOUND        - 是否找到 MySQL
#   MYSQL_INCLUDE_DIR  - MySQL 头文件路径
#   MYSQL_LIBRARIES    - MySQL 链接库

# 查找 mysql_config 工具来获取路径
find_program(MYSQL_CONFIG_EXECUTABLE NAMES mysql_config mysql_config.exe)

if(MYSQL_CONFIG_EXECUTABLE)
    # 使用 mysql_config 获取编译和链接参数
    execute_process(
        COMMAND ${MYSQL_CONFIG_EXECUTABLE} --include
        OUTPUT_VARIABLE MYSQL_CONFIG_INCLUDE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${MYSQL_CONFIG_EXECUTABLE} --libs
        OUTPUT_VARIABLE MYSQL_CONFIG_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # 从 mysql_config 输出中提取路径
    string(REGEX REPLACE "-I" "" MYSQL_INCLUDE_DIR "${MYSQL_CONFIG_INCLUDE}")
    string(STRIP "${MYSQL_INCLUDE_DIR}" MYSQL_INCLUDE_DIR)

    set(MYSQL_LIBRARIES ${MYSQL_CONFIG_LIBS})
else()
    # 回退：手动查找头文件和库
    find_path(MYSQL_INCLUDE_DIR
        NAMES mysql.h
        PATHS
            /usr/include/mysql
            /usr/local/include/mysql
            /usr/include/mariadb
            /usr/local/include/mariadb
            $ENV{MYSQL_INCLUDE_DIR}
    )

    find_library(MYSQL_LIBRARIES
        NAMES mysqlclient mysqlclient_r mariadb mariadbclient
        PATHS
            /usr/lib
            /usr/lib/mysql
            /usr/local/lib
            /usr/local/lib/mysql
            /usr/lib/x86_64-linux-gnu
            $ENV{MYSQL_LIBRARY_DIR}
    )
endif()

# 处理查找结果
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
    DEFAULT_MSG
    MYSQL_INCLUDE_DIR
    MYSQL_LIBRARIES
)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)
