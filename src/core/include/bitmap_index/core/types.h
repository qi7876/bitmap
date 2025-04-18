#ifndef BITMAP_INDEX_CORE_TYPES_H_
#define BITMAP_INDEX_CORE_TYPES_H_

#include <cstdint>   // For uint32_t, uint64_t
#include <string>    // For std::string
#include <vector>    // For std::vector
#include <limits>    // For std::numeric_limits

// 引入 Roaring Bitmap 库的主头文件
// vcpkg 通常会将其添加到 include 路径中，所以可以直接包含
#include "roaring/roaring.hh" // 或者根据实际安装路径调整

namespace bitmap_index
{
    namespace core
    {
        // --- Core Identifiers ---

        /**
         * @brief 内部使用的对象/文档 ID 类型。
         * 使用 uint32_t 以便直接与 Roaring Bitmap 交互。
         * Roaring Bitmap 通常优化用于 32 位整数。
         * 这意味着最多支持约 42 亿 (2^32) 个唯一对象。
         */
        using DocId = uint32_t;

        /**
         * @brief 内部使用的标签 ID 类型。
         * 同样使用 uint32_t，假设标签数量也在 32 位范围内。
         */
        using TagId = uint32_t;

        /**
         * @brief 定义一个无效/未找到的 ID 值。
         * 通常 Roaring Bitmap 不存储 0xFFFFFFFF，可以安全使用。
         * 或者可以根据 Roaring 的具体实现选择一个合适的值。
         */
        constexpr DocId INVALID_DOC_ID = std::numeric_limits<DocId>::max();
        constexpr TagId INVALID_TAG_ID = std::numeric_limits<TagId>::max();


        // --- External Representations ---

        /**
         * @brief 外部（用户输入/输出）使用的对象 ID 类型 (通常是字符串)。
         */
        using StringId = std::string;

        /**
         * @brief 外部（用户输入/输出）使用的标签类型 (通常是字符串)。
         */
        using StringTag = std::string;


        // --- Collections ---

        /**
         * @brief 与单个对象 ID 关联的字符串标签集合。
         * 用于 CSV 解析的输出或正向查询的结果。
         */
        using StringTagSet = std::vector<StringTag>;

        /**
         * @brief 与单个内部 DocId 关联的内部 TagId 集合。
         * 用于存储在正向索引中。
         * 使用 std::vector 是一个简单通用的选择。
         * 如果需要快速检查某个 TagId 是否存在，可以考虑 std::unordered_set<TagId>。
         * 如果每个对象的标签数量非常多，甚至可以考虑用 roaring::Roaring 来存储 TagId 集合。
         */
        using TagIdSet = std::vector<TagId>;
        // using TagIdSet = roaring::Roaring; // 备选方案

        /**
         * @brief 用于存储给定 TagId 的所有 DocId 的 Roaring Bitmap。
         * 这是倒排索引的核心数据结构。
         */
        using DocIdBitmap = roaring::Roaring;

        /**
         * @brief 查询结果返回的字符串 ID 列表。
         */
        using StringIdList = std::vector<StringId>;


        // --- Query Operations ---

        /**
         * @brief 定义了在多个标签上执行查询时可以使用的操作类型。
         */
        enum class QueryOperation
        {
            AND, ///< 交集 (Intersection): 结果包含所有指定标签的对象 ID
            OR, ///< 并集 (Union): 结果包含至少有一个指定标签的对象 ID
            XOR, ///< 对称差 (Symmetric Difference): 结果包含只出现在奇数个指定标签中的对象 ID
            ANDNOT
            ///< 差集 (Difference): A AND NOT B, 需要指定哪个是 A，哪个是 B (通常用于两个 Bitmap)
                         ///< 对于多个标签，可能需要更复杂的逻辑或定义，例如 (Tag1 AND NOT Tag2) AND NOT Tag3 ...
                         ///< 或者定义为 A \ (B | C | ...)
        };


        // --- Other Useful Types ---

        /**
         * @brief 用于表示文件偏移量，支持增量加载。
         * 使用 64 位以支持大文件。
         */
        using FileOffset = uint64_t;
    } // namespace core
} // namespace bitmap_index

#endif // BITMAP_INDEX_CORE_TYPES_H_
