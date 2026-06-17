#pragma once

#include <vector>
#include <functional>
#include <optional>

namespace hospital {

/// 二分查找算法
///
/// 经典算法之一，时间复杂度 O(log n)
/// 适用于已排序的数组，通过不断缩小搜索范围快速定位目标值
///
/// 用法示例：
///   std::vector<int> sorted = {1, 3, 5, 7, 9};
///   auto result = BinarySearch::find(sorted, 5);
///   if (result) { std::cout << "找到，索引: " << *result; }
class BinarySearch {
public:
    /// 在已排序数组中查找目标值
    /// @param data 已排序的数组
    /// @param target 目标值
    /// @return 找到返回索引，未找到返回 std::nullopt
    template<typename T>
    static std::optional<size_t> find(const std::vector<T>& data, const T& target) {
        if (data.empty()) return std::nullopt;

        size_t left = 0;
        size_t right = data.size() - 1;

        while (left <= right) {
            // 防止溢出的中间值计算
            size_t mid = left + (right - left) / 2;

            if (data[mid] == target) {
                return mid;  // 找到目标
            } else if (data[mid] < target) {
                left = mid + 1;  // 目标在右半部分
            } else {
                if (mid == 0) break;  // 防止下溢
                right = mid - 1;  // 目标在左半部分
            }
        }

        return std::nullopt;  // 未找到
    }

    /// 在已排序数组中查找第一个大于等于目标值的位置
    /// @param data 已排序的数组
    /// @param target 目标值
    /// @return 第一个大于等于目标值的索引
    template<typename T>
    static size_t lowerBound(const std::vector<T>& data, const T& target) {
        size_t left = 0;
        size_t right = data.size();

        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (data[mid] < target) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        return left;
    }

    /// 使用自定义比较函数的二分查找
    /// @param data 数据数组
    /// @param target 目标值
    /// @param comp 比较函数，返回 true 表示第一个参数小于第二个参数
    /// @return 找到返回索引，未找到返回 std::nullopt
    template<typename T, typename Compare>
    static std::optional<size_t> find(const std::vector<T>& data, const T& target, Compare comp) {
        if (data.empty()) return std::nullopt;

        size_t left = 0;
        size_t right = data.size() - 1;

        while (left <= right) {
            size_t mid = left + (right - left) / 2;

            if (!comp(data[mid], target) && !comp(target, data[mid])) {
                return mid;  // 相等
            } else if (comp(data[mid], target)) {
                left = mid + 1;
            } else {
                if (mid == 0) break;
                right = mid - 1;
            }
        }

        return std::nullopt;
    }
};

} // namespace hospital
