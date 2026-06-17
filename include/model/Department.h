#pragma once

#include <string>

namespace hospital {

/// 科室数据模型
struct Department {
    int64_t id = 0;              ///< 科室 ID
    std::string name;            ///< 科室名称
    std::string location;        ///< 位置（楼层/区域）
    std::string description;     ///< 科室描述
};

} // namespace hospital
