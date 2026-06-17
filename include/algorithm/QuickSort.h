#pragma once

#include <vector>
#include <functional>
#include <algorithm>

namespace hospital {

/// 快速排序算法
///
/// 经典算法之一，平均时间复杂度 O(n log n)
/// 采用分治策略，选择基准元素将数组分为两部分，递归排序
///
/// 用法示例：
///   std::vector<int> data = {3, 1, 4, 1, 5, 9, 2, 6};
///   QuickSort::sort(data);  // 升序排序
///   QuickSort::sort(data, std::greater<int>());  // 降序排序
class QuickSort {
public:
    /// 对数组进行升序排序
    /// @param data 待排序数组
    template<typename T>
    static void sort(std::vector<T>& data) {
        if (data.size() <= 1) return;
        quickSort(data, 0, data.size() - 1);
    }

    /// 使用自定义比较函数排序
    /// @param data 待排序数组
    /// @param comp 比较函数，返回 true 表示第一个参数应排在第二个参数前面
    template<typename T, typename Compare>
    static void sort(std::vector<T>& data, Compare comp) {
        if (data.size() <= 1) return;
        quickSortComp(data, 0, data.size() - 1, comp);
    }

    /// 对数组的指定范围进行排序
    /// @param data 待排序数组
    /// @param left 起始索引
    /// @param right 结束索引
    template<typename T>
    static void sortRange(std::vector<T>& data, size_t left, size_t right) {
        if (left >= right || right >= data.size()) return;
        quickSort(data, left, right);
    }

private:
    /// 快速排序核心实现（升序）
    template<typename T>
    static void quickSort(std::vector<T>& data, size_t low, size_t high) {
        if (low >= high) return;

        size_t pivotIndex = partition(data, low, high);
        if (pivotIndex > 0) {
            quickSort(data, low, pivotIndex - 1);
        }
        quickSort(data, pivotIndex + 1, high);
    }

    /// 分区函数（升序）
    /// 选择最后一个元素作为基准，将小于基准的元素放到左边，大于基准的放到右边
    template<typename T>
    static size_t partition(std::vector<T>& data, size_t low, size_t high) {
        // 三数取中法选择基准，避免最坏情况
        size_t mid = low + (high - low) / 2;
        if (data[mid] < data[low]) std::swap(data[low], data[mid]);
        if (data[high] < data[low]) std::swap(data[low], data[high]);
        if (data[high] < data[mid]) std::swap(data[mid], data[high]);

        // 将中位数放到 high-1 位置作为基准
        std::swap(data[mid], data[high]);
        T pivot = data[high];

        size_t i = low;
        for (size_t j = low; j < high; ++j) {
            if (data[j] <= pivot) {
                std::swap(data[i], data[j]);
                ++i;
            }
        }
        std::swap(data[i], data[high]);
        return i;
    }

    /// 快速排序核心实现（自定义比较）
    template<typename T, typename Compare>
    static void quickSortComp(std::vector<T>& data, size_t low, size_t high, Compare comp) {
        if (low >= high) return;

        size_t pivotIndex = partitionComp(data, low, high, comp);
        if (pivotIndex > 0) {
            quickSortComp(data, low, pivotIndex - 1, comp);
        }
        quickSortComp(data, pivotIndex + 1, high, comp);
    }

    /// 分区函数（自定义比较）
    template<typename T, typename Compare>
    static size_t partitionComp(std::vector<T>& data, size_t low, size_t high, Compare comp) {
        // 三数取中法选择基准
        size_t mid = low + (high - low) / 2;
        if (comp(data[mid], data[low])) std::swap(data[low], data[mid]);
        if (comp(data[high], data[low])) std::swap(data[low], data[high]);
        if (comp(data[high], data[mid])) std::swap(data[mid], data[high]);

        std::swap(data[mid], data[high]);
        T pivot = data[high];

        size_t i = low;
        for (size_t j = low; j < high; ++j) {
            if (!comp(pivot, data[j])) {  // data[j] <= pivot
                std::swap(data[i], data[j]);
                ++i;
            }
        }
        std::swap(data[i], data[high]);
        return i;
    }
};

} // namespace hospital
