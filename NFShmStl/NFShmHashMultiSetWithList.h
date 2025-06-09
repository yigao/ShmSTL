// -------------------------------------------------------------------------
//    @FileName         :    NFShmHashMultiSetWithList.h
//    @Author           :    gaoyi
//    @Date             :    23-2-11
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmHashMultiSetWithList
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmHashTable.h"
#include <set>
#include <unordered_set>

/**
 * @file NFShmHashMultiSetWithList.h
 * @brief 基于共享内存的无序多重集合容器实现（链表增强版），与STL std::unordered_multiset高度兼容
 * 
 * @section overview 概述
 * 
 * NFShmHashMultiSetWithList 是一个专为共享内存环境设计的无序多重集合容器，在NFShmHashMultiSet
 * 基础上增加了双向链表支持，提供LRU（最近最少使用）缓存语义和顺序遍历功能。它允许存储重复
 * 元素，同时支持基于访问模式的自动淘汰机制。在API设计上与STL std::unordered_multiset高度
 * 兼容，但在内存管理、容量控制和访问模式方面针对共享内存场景进行了深度优化。
 * 
 * @section features 核心特性
 * 
 * 1. **多重集合语义**：
 *    - 允许相同元素的多个副本存在
 *    - 相同元素在迭代时保持相邻性
 *    - equal_range()提供相同元素的范围
 *    - count()返回指定元素的数量
 * 
 * 2. **双向链表增强**：
 *    - 内建双向链表，维护元素访问/插入顺序
 *    - list_begin()、list_end()提供顺序遍历
 *    - LRU缓存语义，自动淘汰最久未使用元素
 *    - enable_lru()/disable_lru()动态控制LRU行为
 * 
 * 3. **STL高度兼容**：
 *    - 完整的std::unordered_multiset API支持
 *    - 标准的迭代器接口和类型定义
 *    - find()、count()、equal_range()等常用操作
 *    - 支持范围for循环和STL算法
 * 
 * 4. **共享内存优化**：
 *    - 固定大小内存布局，避免动态分配
 *    - 元素直接存储，无额外包装
 *    - 支持CREATE/RESUME两阶段初始化
 *    - 无内存碎片，高效的内存使用
 * 
 * 5. **缓存友好设计**：
 *    - O(1)平均时间复杂度的哈希操作
 *    - 链表维护访问顺序，适合LRU淘汰策略
 *    - 预分配节点池，快速内存分配
 *    - 固定桶数量，无rehash开销
 * 
 * @section stl_comparison 与STL std::unordered_multiset对比
 * 
 * | 特性 | std::unordered_multiset | NFShmHashMultiSetWithList |
 * |------|------------------------|--------------------------|
 * | **内存管理** | 动态堆内存分配 | 固定共享内存预分配 |
 * | **容量限制** | 无限制，动态扩容 | 固定容量MAX_SIZE |
 * | **扩容机制** | 自动rehash扩容 | **不支持扩容** |
 * | **元素存储** | 直接存储元素值 | 直接存储元素值 |
 * | **重复元素** | 完全支持 | 完全支持 |
 * | **相同元素相邻性** | 保证 | 保证 |
 * | **插入语义** | insert总是成功 | insert总是成功（除非满） |
 * | **查找性能** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 |
 * | **count操作** | 返回元素数量 | 返回元素数量 |
 * | **equal_range** | 完全支持 | 完全支持 |
 * | **访问顺序** | 无保证，依赖哈希顺序 | **双向链表维护顺序** |
 * | **LRU支持** | 不支持 | **原生LRU缓存语义** |
 * | **顺序遍历** | 仅哈希桶顺序 | **链表顺序遍历** |
 * | **缓存淘汰** | 不支持 | **自动LRU淘汰机制** |
 * | **进程共享** | 不支持 | **原生支持** |
 * | **初始化方式** | 构造函数 | CreateInit/ResumeInit |
 * | **负载因子** | 动态调整 | 固定结构 |
 * | **异常安全** | 强异常安全保证 | 无异常，错误码返回 |
 * | **内存碎片** | 可能产生 | **无碎片** |
 * | **性能稳定性** | rehash时性能抖动 | **性能稳定** |
 * | **迭代器稳定性** | rehash时失效 | **始终稳定** |
 * 
 * @section api_compatibility API兼容性
 * 
 * **完全兼容的接口**：
 * - **容器属性**：size(), empty(), max_size()
 * - **迭代器**：begin(), end(), cbegin(), cend()
 * - **查找访问**：find(), count(), equal_range()
 * - **修改操作**：insert(), emplace(), erase(), clear()
 * - **容器操作**：swap()
 * - **桶接口**：bucket_count(), max_bucket_count(), elems_in_bucket()
 * 
 * **扩展的接口（新增）**：
 * - **容量检查**：full(), left_size()
 * - **共享内存**：CreateInit(), ResumeInit()
 * - **链表遍历**：list_begin(), list_end(), list_cbegin(), list_cend()
 * - **LRU控制**：enable_lru(), disable_lru(), is_lru_enabled()
 * - **STL转换**：从std::unordered_multiset和std::multiset构造
 * 
 * **不支持的接口**：
 * - **动态管理**：rehash(), reserve(), max_load_factor()
 * - **哈希策略**：load_factor(), bucket_size()
 * - **自定义分配器**：get_allocator()
 * 
 * @section usage_examples 使用示例
 * 
 * @subsection basic_usage 基础用法（类似std::unordered_multiset）
 * 
 * ```cpp
 * // 定义容量为1000的整数多重集合（带链表）
 * NFShmHashMultiSetWithList<int, 1000> numberMultiSet;
 * numberMultiSet.CreateInit();  // 创建模式初始化
 * 
 * // 插入元素（允许重复）
 * numberMultiSet.insert(42);
 * numberMultiSet.insert(42);  // 重复插入，会成功
 * numberMultiSet.insert(42);
 * numberMultiSet.insert(10);
 * numberMultiSet.insert(10);
 * 
 * std::cout << "MultiSet size: " << numberMultiSet.size() << std::endl;  // 输出：5
 * 
 * // 查看某个元素的数量
 * std::cout << "Count of 42: " << numberMultiSet.count(42) << std::endl;  // 输出：3
 * std::cout << "Count of 10: " << numberMultiSet.count(10) << std::endl;  // 输出：2
 * 
 * // 获取某个元素的所有副本
 * auto range = numberMultiSet.equal_range(42);
 * std::cout << "All instances of 42: ";
 * for (auto it = range.first; it != range.second; ++it) {
 *     std::cout << *it << " ";
 * }
 * std::cout << std::endl;
 * 
 * // 哈希遍历（无序）
 * std::cout << "Hash order traversal:" << std::endl;
 * for (const auto& num : numberMultiSet) {
 *     std::cout << num << " ";
 * }
 * std::cout << std::endl;
 * 
 * // 链表遍历（按访问/插入顺序）
 * std::cout << "List order traversal:" << std::endl;
 * for (auto it = numberMultiSet.list_begin(); it != numberMultiSet.list_end(); ++it) {
 *     std::cout << *it << " ";
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @subsection frequency_counting 频率统计应用
 * 
 * ```cpp
 * NFShmHashMultiSetWithList<std::string, 300> wordFrequencySet;
 * wordFrequencySet.disable_lru();  // 仅跟踪访问顺序，不淘汰
 * 
 * // 模拟文档中的词汇频率统计
 * std::vector<std::string> words = {
 *     "hello", "world", "hello", "c++", "programming",
 *     "world", "hello", "coding", "algorithm", "data",
 *     "structure", "hello", "world", "programming", "c++"
 * };
 * 
 * std::cout << "Processing word frequency..." << std::endl;
 * for (const std::string& word : words) {
 *     wordFrequencySet.insert(word);
 *     std::cout << "Inserted '" << word << "', count: " << wordFrequencySet.count(word) << std::endl;
 * }
 * 
 * std::cout << "Total word entries: " << wordFrequencySet.size() << std::endl;
 * 
 * // 查看每个唯一单词的频率
 * std::map<std::string, int> wordCounts;
 * for (const std::string& word : wordFrequencySet) {
 *     wordCounts[word]++;
 * }
 * 
 * std::cout << "Word frequency statistics:" << std::endl;
 * for (const auto& [word, count] : wordCounts) {
 *     std::cout << "'" << word << "': " << count << " times" << std::endl;
 * }
 * 
 * // 获取特定单词的所有出现位置（按访问顺序）
 * std::string targetWord = "hello";
 * auto range = wordFrequencySet.equal_range(targetWord);
 * std::cout << "All occurrences of '" << targetWord << "':" << std::endl;
 * int occurrence = 1;
 * for (auto it = range.first; it != range.second; ++it, ++occurrence) {
 *     std::cout << "Occurrence " << occurrence << ": " << *it << std::endl;
 * }
 * 
 * // 按访问顺序查看最近处理的单词
 * std::cout << "Recent processing order:" << std::endl;
 * int wordCount = 0;
 * for (auto it = wordFrequencySet.list_begin(); 
 *          it != wordFrequencySet.list_end() && wordCount < 10; 
 *          ++it, ++wordCount) {
 *     std::cout << *it << " ";
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @subsection lru_multiset_cache LRU多重集合缓存
 * 
 * ```cpp
 * // 构建游戏分数记录的LRU缓存
 * NFShmHashMultiSetWithList<int, 150> gameScoresCache;
 * gameScoresCache.CreateInit();
 * gameScoresCache.enable_lru();  // 启用LRU
 * 
 * // 模拟多个玩家的游戏分数
 * std::vector<int> playerScores = {
 *     85, 92, 78, 95, 88, 85, 91, 76, 89, 92,  // 第一轮
 *     82, 94, 85, 87, 90, 78, 93, 86, 88, 95,  // 第二轮
 *     89, 91, 85, 84, 87, 92, 79, 86, 90, 88   // 第三轮
 * };
 * 
 * std::cout << "Recording game scores..." << std::endl;
 * for (int score : playerScores) {
 *     gameScoresCache.insert(score);
 * }
 * 
 * std::cout << "Total score records: " << gameScoresCache.size() << std::endl;
 * 
 * // 继续添加更多分数直到缓存满
 * for (int i = 0; i < 200; ++i) {
 *     int randomScore = 60 + (i % 40);  // 生成60-99的分数
 *     gameScoresCache.insert(randomScore);
 * }
 * 
 * std::cout << "Cache size after filling: " << gameScoresCache.size() << std::endl;
 * std::cout << "Cache is full: " << gameScoresCache.full() << std::endl;
 * 
 * // 查询一些分数的频率（会影响LRU顺序）
 * std::vector<int> queryScores = {85, 90, 95};
 * for (int score : queryScores) {
 *     int count = gameScoresCache.count(score);
 *     std::cout << "Score " << score << " appears " << count << " times" << std::endl;
 * }
 * 
 * // 添加更多分数（会淘汰最久未访问的分数）
 * std::vector<int> newScores = {96, 97, 98, 99, 100};
 * for (int score : newScores) {
 *     for (int i = 0; i < 3; ++i) {  // 每个分数插入3次
 *         gameScoresCache.insert(score);
 *     }
 * }
 * 
 * // 检查查询过的分数是否还存在
 * for (int score : queryScores) {
 *     int count = gameScoresCache.count(score);
 *     std::cout << "After LRU, score " << score << " appears " << count << " times" << std::endl;
 * }
 * 
 * // 查看最近记录的分数（按LRU顺序）
 * std::cout << "Most recent scores (LRU order):" << std::endl;
 * int displayCount = 0;
 * for (auto it = gameScoresCache.list_begin(); 
 *          it != gameScoresCache.list_end() && displayCount < 20; 
 *          ++it, ++displayCount) {
 *     std::cout << *it << " ";
 *     if ((displayCount + 1) % 10 == 0) std::cout << std::endl;
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @section performance_notes 性能说明
 * 
 * - **查找性能**：O(1)平均，O(n)最坏（链表长度）
 * - **插入性能**：O(1)平均，总是成功（除非容量满），包含链表维护开销
 * - **删除性能**：O(1)平均，支持按值删除多个元素，需更新链表结构
 * - **计数操作**：count() O(1)平均 + O(k)统计（k为相同元素数）
 * - **范围查询**：equal_range() O(1)平均 + O(k)遍历（k为相同元素数）
 * - **LRU操作**：O(1)时间复杂度的链表头尾操作
 * - **顺序遍历**：O(n)链表遍历，缓存友好
 * - **内存性能**：零碎片，预分配，额外链表节点开销
 * - **重复元素性能**：相同元素在链表中保持相邻性，遍历效率高
 * - **并发性能**：需要外部同步，但支持多进程读写
 * 
 * @section migration_guide 迁移指南
 * 
 * 从std::unordered_multiset迁移到NFShmHashMultiSetWithList：
 * 
 * 1. **替换类型声明**：
 *    ```cpp
 *    // 原代码
 *    std::unordered_multiset<int> multiset;
 *    
 *    // 新代码（添加容量模板参数）
 *    NFShmHashMultiSetWithList<int, 10000> multiset;
 *    ```
 * 
 * 2. **添加初始化**：
 *    ```cpp
 *    // 添加共享内存初始化
 *    multiset.CreateInit();  // 或 ResumeInit()
 *    ```
 * 
 * 3. **保持多重集合语义**：
 *    ```cpp
 *    // 多重集合的核心操作保持不变
 *    multiset.insert(element);      // 总是成功插入
 *    auto count = multiset.count(element);  // 获取元素数量
 *    auto range = multiset.equal_range(element);  // 获取范围
 *    ```
 * 
 * 4. **利用链表特性**：
 *    ```cpp
 *    // 启用LRU缓存语义
 *    multiset.enable_lru();
 *    
 *    // 使用链表顺序遍历
 *    for (auto it = multiset.list_begin(); it != multiset.list_end(); ++it) {
 *         // 按访问/插入顺序处理元素
 *    }
 *    ```
 * 
 * 5. **处理容量限制**：
 *    ```cpp
 *    // 利用LRU自动淘汰机制
 *    if (multiset.full() && multiset.is_lru_enabled()) {
 *         // 插入会自动淘汰最久未使用的元素
 *         multiset.insert(element);
 *    }
 *    ```
 * 
 * 6. **移除不支持的操作**：
 *    ```cpp
 *    // 移除这些调用（NFShmHashMultiSetWithList不支持）
 *    // multiset.rehash(1000);     // 动态管理不支持
 *    // multiset.reserve(500);     // 动态管理不支持
 *    // multiset.max_load_factor(0.8);  // 负载因子管理不支持
 *    ```
 * 
 * 7. **充分利用新特性**：
 *    ```cpp
 *    // 检查LRU状态
 *    if (multiset.is_lru_enabled()) {
 *         // 基于访问模式的逻辑
 *    }
 *    
 *    // 动态控制LRU行为
 *    multiset.disable_lru();  // 禁用LRU，仅维护访问顺序
 *    multiset.enable_lru();   // 重新启用LRU淘汰
 *    
 *    // 处理重复元素
 *    auto range = multiset.equal_range(element);
 *    for (auto it = range.first; it != range.second; ++it) {
 *         // 处理所有相同的元素
 *    }
 *    ```
 */

/****************************************************************************
 * STL std::unordered_multiset 对比分析（WithList增强版）
 ****************************************************************************
 * 
 * 1. 内存管理策略对比：
 *    - std::unordered_multiset: 动态内存分配，使用allocator管理堆内存
 *    - NFShmHashMultiSetWithList: 固定大小共享内存，预分配所有节点，支持进程间共享
 * 
 * 2. 容量管理对比：
 *    - std::unordered_multiset: 动态扩容，load_factor超过阈值时自动rehash
 *    - NFShmHashMultiSetWithList: 固定容量MAX_SIZE，通过LRU机制处理容量满的情况
 * 
 * 3. 重复元素处理对比：
 *    - 都允许相同元素的多个副本存在
 *    - 都在相同元素之间保持相邻性
 *    - insert操作都总是成功（除非容量不足）
 * 
 * 4. 访问顺序管理对比：
 *    - std::unordered_multiset: 无访问顺序概念，遍历顺序依赖哈希桶
 *    - NFShmHashMultiSetWithList: 内置双向链表维护访问/插入顺序，支持LRU语义
 * 
 * 5. 元素存储对比：
 *    - std::unordered_multiset: 直接存储元素值
 *    - NFShmHashMultiSetWithList: 同样直接存储元素值，使用std::_Identity函数提取键
 * 
 * 6. 缓存语义对比：
 *    - std::unordered_multiset: 无缓存概念，需要手动实现LRU逻辑
 *    - NFShmHashMultiSetWithList: 原生LRU支持，自动淘汰最久未使用元素
 * 
 * 7. 性能特征对比：
 *    - std::unordered_multiset: O(1)平均时间复杂度，但可能触发rehash开销
 *    - NFShmHashMultiSetWithList: O(1)平均时间复杂度，额外O(1)链表维护开销，无rehash开销
 * 
 * 8. API兼容性：
 *    - 兼容接口：insert, find, erase, begin, end, size, empty, count, equal_range等
 *    - 扩展接口：list_begin, list_end, enable_lru, disable_lru, is_lru_enabled等
 *    - 特有接口：full(), left_size(), CreateInit(), ResumeInit()等共享内存特有功能
 *    - 缺少接口：rehash, reserve, bucket, load_factor等动态管理接口
 * 
 * 9. 使用场景对比：
 *    - std::unordered_multiset: 通用多重集合容器，频率统计
 *    - NFShmHashMultiSetWithList: 频率缓存、访问统计、重复数据管理、LRU淘汰策略、共享内存应用
 * 
 * 10. 与NFShmHashSetWithList的区别：
 *     - NFShmHashSetWithList: 保证元素唯一性，使用insert_unique
 *     - NFShmHashMultiSetWithList: 允许重复元素，使用insert_equal
 *****************************************************************************/

// ==================== 前向声明 ====================

template <class Value, int MAX_SIZE,
          class HashFcn = std::hash<Value>,
          class EqualKey = std::equal_to<Value>>
class NFShmHashMultiSetWithList;

template <class Val, int MAX_SIZE, class HashFcn, class EqualKey>
bool operator==(const NFShmHashMultiSetWithList<Val, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashMultiSetWithList<Val, MAX_SIZE, HashFcn, EqualKey>& hs2);

// ==================== 主要容器类 ====================

/**
 * @brief 基于共享内存的无序多重集合容器
 * @tparam Value 元素类型
 * @tparam MAX_SIZE 最大容量（固定）
 * @tparam HashFcn 哈希函数类型，默认std::hash<Value>
 * @tparam EqualKey 元素比较函数类型，默认std::equal_to<Value>
 * 
 * 设计特点：
 * 1. 固定容量，不支持动态扩容（与STL主要区别）
 * 2. 基于共享内存，支持进程间共享
 * 3. API设计尽量兼容STL std::unordered_multiset
 * 4. 允许重复元素，不保证元素唯一性（与NFShmHashSet的主要区别）
 * 
 * 与std::unordered_multiset的主要差异：
 * - 容量限制：固定大小 vs 动态扩容
 * - 内存管理：共享内存 vs 堆内存
 * - 性能特征：无rehash开销 vs 动态性能优化
 * - 进程支持：进程间共享 vs 单进程内使用
 * - 插入语义：总是成功插入 vs 动态管理存储
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
class NFShmHashMultiSetWithList
{
private:
    /// @brief 底层哈希表类型，使用std::_Identity函数提取键，允许重复元素
    typedef NFShmHashTable<Value, Value, MAX_SIZE, HashFcn, std::stl__Identity<Value>, EqualKey> HashTable;
    HashTable m_hashTable; ///< 底层哈希表实例

public:
    // ==================== STL兼容类型定义 ====================

    typedef typename HashTable::key_type key_type; ///< 键类型（与值类型相同）
    typedef typename HashTable::value_type value_type; ///< 值类型（即元素类型）
    typedef typename HashTable::hasher hasher; ///< 哈希函数类型
    typedef typename HashTable::key_equal key_equal; ///< 键相等比较函数类型

    typedef typename HashTable::size_type size_type; ///< 大小类型
    typedef typename HashTable::difference_type difference_type; ///< 差值类型
    typedef typename HashTable::pointer pointer; ///< 指针类型
    typedef typename HashTable::const_pointer const_pointer; ///< 常量指针类型
    typedef typename HashTable::reference reference; ///< 引用类型
    typedef typename HashTable::const_reference const_reference; ///< 常量引用类型

    typedef typename HashTable::iterator iterator; ///< 迭代器类型
    typedef typename HashTable::const_iterator const_iterator; ///< 常量迭代器类型

    typedef typename HashTable::list_iterator list_iterator; ///< 迭代器类型
    typedef typename HashTable::const_list_iterator const_list_iterator; ///< 常量迭代器类型
public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 默认构造函数
     * @note 根据SHM_CREATE_MODE决定创建或恢复模式
     * @note 与std::unordered_multiset()行为类似，但增加共享内存初始化
     */
    NFShmHashMultiSetWithList()
    {
        if (SHM_CREATE_MODE)
        {
            CreateInit();
        }
        else
        {
            ResumeInit();
        }
    }

    /**
     * @brief 范围构造函数
     * @tparam InputIterator 输入迭代器类型
     * @param f 起始迭代器
     * @param l 结束迭代器
     * @note 与std::unordered_multiset(first, last)兼容
     */
    template <class InputIterator>
    NFShmHashMultiSetWithList(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 数组构造函数
     * @param f 起始指针
     * @param l 结束指针
     * @note 与std::unordered_multiset兼容
     */
    NFShmHashMultiSetWithList(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    NFShmHashMultiSetWithList(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 拷贝构造函数
     * @param x 源NFShmHashMultiSetWithList对象
     * @note 与std::unordered_multiset拷贝构造函数兼容
     */
    NFShmHashMultiSetWithList(const NFShmHashMultiSetWithList& x)
    {
        if (this != &x) m_hashTable = x.m_hashTable;
    }

    /**
     * @brief 从std::unordered_set构造
     * @param set 源std::unordered_set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiSetWithList(const std::unordered_set<Value>& set)
    {
        m_hashTable.insert_equal(set.begin(), set.end());
    }

    /**
     * @brief 从std::set构造
     * @param set 源std::set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiSetWithList(const std::set<Value>& set)
    {
        m_hashTable.insert_equal(set.begin(), set.end());
    }

    /**
     * @brief 从std::unordered_multiset构造
     * @param set 源std::unordered_multiset对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiSetWithList(const std::unordered_multiset<Value>& set)
    {
        m_hashTable.insert_equal(set.begin(), set.end());
    }

    /**
     * @brief 从std::multiset构造
     * @param set 源std::multiset对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiSetWithList(const std::multiset<Value>& set)
    {
        m_hashTable.insert_equal(set.begin(), set.end());
    }

    /**
     * @brief 初始化列表构造函数
     * @param list 初始化列表
     * @note 与std::unordered_multiset(std::initializer_list)兼容
     */
    NFShmHashMultiSetWithList(const std::initializer_list<Value>& list)
    {
        insert(list.begin(), list.end());
    }

    // ==================== 共享内存特有接口 ====================

    /**
     * @brief 创建模式初始化
     * @return 0表示成功
     * @note STL容器没有此接口，共享内存特有
     */
    int CreateInit()
    {
        return 0;
    }

    /**
     * @brief 恢复模式初始化
     * @return 0表示成功
     * @note 从共享内存恢复时调用，STL容器没有此接口
     */
    int ResumeInit()
    {
        return 0;
    }

    /**
     * @brief 就地初始化
     * @note 使用placement new重新构造对象
     * @note STL容器没有此接口
     */
    void Init()
    {
        new(this) NFShmHashMultiSetWithList();
    }

    // ==================== 赋值操作符 ====================

    /**
     * @brief 拷贝赋值操作符
     * @param x 源NFShmHashMultiSetWithList对象
     * @return 自身引用
     * @note 与std::unordered_multiset::operator=兼容
     */
    NFShmHashMultiSetWithList<Value, MAX_SIZE>& operator=(const NFShmHashMultiSetWithList<Value, MAX_SIZE>& x);

    /**
     * @brief 从std::unordered_multiset赋值
     * @param x 源std::unordered_multiset对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiSetWithList<Value, MAX_SIZE>& operator=(const std::unordered_multiset<Value>& x);

    /**
     * @brief 从std::multiset赋值
     * @param x 源std::multiset对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiSetWithList<Value, MAX_SIZE>& operator=(const std::multiset<Value>& x);

    /**
     * @brief 从std::unordered_set赋值
     * @param x 源std::unordered_set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiSetWithList<Value, MAX_SIZE>& operator=(const std::unordered_set<Value>& x);

    /**
     * @brief 从std::set赋值
     * @param x 源std::set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiSetWithList<Value, MAX_SIZE>& operator=(const std::set<Value>& x);

    /**
     * @brief 从初始化列表赋值
     * @param x 初始化列表
     * @return 自身引用
     * @note 与std::unordered_multiset::operator=(std::initializer_list)兼容
     */
    NFShmHashMultiSetWithList<Value, MAX_SIZE>& operator=(const std::initializer_list<Value>& x);

public:
    // ==================== 容量相关接口（STL兼容） ====================

    /**
     * @brief 获取当前元素数量
     * @return 元素数量（包括重复元素）
     * @note 与std::unordered_multiset::size()兼容
     */
    size_type size() const { return m_hashTable.size(); }

    /**
     * @brief 获取最大容量
     * @return 最大容量MAX_SIZE
     * @note 与std::unordered_multiset::max_size()不同，返回固定值而非理论最大值
     */
    size_type max_size() const { return m_hashTable.max_size(); }

    /**
     * @brief 判断是否为空
     * @return true表示空
     * @note 与std::unordered_multiset::empty()兼容
     */
    bool empty() const { return m_hashTable.empty(); }

    /**
     * @brief 判断是否已满
     * @return true表示已满
     * @note STL容器没有此接口，固定容量特有
     */
    bool full() const { return m_hashTable.full(); }

    /**
     * @brief 获取剩余容量
     * @return 剩余可用空间
     * @note STL容器没有此接口，固定容量特有
     */
    size_t left_size() const { return m_hashTable.left_size(); }

    /**
     * @brief 交换两个容器的内容
     * @param hs 另一个NFShmHashMultiSetWithList对象
     * @note 与std::unordered_multiset::swap()兼容
     */
    void swap(NFShmHashMultiSetWithList& hs) noexcept { m_hashTable.swap(hs.m_hashTable); }

    // ==================== 友元比较操作符 ====================

    template <class Val, int O_MAX_SIZE, class Hf, class EqK>
    friend bool operator==(const NFShmHashMultiSetWithList<Val, O_MAX_SIZE, Hf, EqK>&,
                           const NFShmHashMultiSetWithList<Val, O_MAX_SIZE, Hf, EqK>&);

    // ==================== STL兼容迭代器接口 ====================

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     * @note 与std::unordered_multiset::begin()兼容
     */
    iterator begin() { return m_hashTable.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     * @note 与std::unordered_multiset::end()兼容
     */
    iterator end() { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_multiset::begin() const兼容
     */
    const_iterator begin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_multiset::end() const兼容
     */
    const_iterator end() const { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_multiset::cbegin()兼容
     */
    const_iterator cbegin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_multiset::cend()兼容
     */
    const_iterator cend() const { return m_hashTable.end(); }
public:
    // ==================== 链表迭代器接口（新增） ====================

    /**
     * @brief 获取链表起始迭代器（按插入顺序）
     * @return 指向链表第一个元素的迭代器
     * @note 按插入顺序遍历，用于实现FIFO或调试
     */
    list_iterator list_begin() { return m_hashTable.list_begin(); }

    /**
     * @brief 获取链表结束迭代器
     * @return 指向链表末尾的迭代器
     */
    list_iterator list_end() { return m_hashTable.list_end(); }

    /**
     * @brief 获取常量链表起始迭代器
     * @return 指向链表第一个元素的常量迭代器
     */
    const_list_iterator list_begin() const { return m_hashTable.list_begin(); }

    /**
     * @brief 获取常量链表结束迭代器
     * @return 指向链表末尾的常量迭代器
     */
    const_list_iterator list_end() const { return m_hashTable.list_end(); }

    /**
     * @brief 获取常量链表起始迭代器（C++11风格）
     * @return 指向链表第一个元素的常量迭代器
     */
    const_list_iterator list_cbegin() const { return m_hashTable.list_cbegin(); }

    /**
     * @brief 获取常量链表结束迭代器（C++11风格）
     * @return 指向链表末尾的常量迭代器
     */
    const_list_iterator list_cend() const { return m_hashTable.list_cend(); }

    // ==================== LRU功能控制接口（新增） ====================

    /**
     * @brief 启用LRU功能
     * @note 启用后，find/count操作会将访问的节点移动到链表尾部
     */
    void enable_lru() { m_hashTable.enable_lru(); }

    /**
     * @brief 禁用LRU功能
     * @note 禁用后，find/count操作不会移动节点位置
     */
    void disable_lru() { m_hashTable.disable_lru(); }

    /**
     * @brief 检查LRU功能是否启用
     * @return true表示启用，false表示禁用
     */
    bool is_lru_enabled() const { return m_hashTable.is_lru_enabled(); }
public:
    // ==================== 插入接口（STL兼容） ====================

    /**
     * @brief 插入元素（多重集合总是成功）
     * @param obj 要插入的元素
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_multiset::insert()兼容，总是成功插入
     */
    iterator insert(const value_type& obj)
    {
        return m_hashTable.insert_equal(obj);
    }

    /**
     * @brief 带提示插入元素
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param obj 要插入的元素
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_multiset::insert(const_iterator, const value_type&)兼容
     */
    iterator insert(const_iterator hint, const value_type& obj)
    {
        (void)hint; // 忽略提示参数
        return m_hashTable.insert_equal(obj);
    }

    /**
     * @brief 就地构造元素
     * @tparam Args 构造参数类型包
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_multiset::emplace()兼容
     */
    template <typename... Args>
    iterator emplace(const Args&... args);

    /**
     * @brief 带提示就地构造元素
     * @tparam Args 构造参数类型包
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_multiset::emplace_hint()兼容
     */
    template <typename... Args>
    iterator emplace_hint(const_iterator hint, const Args&... args);

    /**
     * @brief 范围插入
     * @tparam InputIterator 输入迭代器类型
     * @param f 起始迭代器
     * @param l 结束迭代器
     * @note 与std::unordered_multiset::insert(first, last)兼容
     */
    template <class InputIterator>
    void insert(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 数组范围插入
     * @param f 起始指针
     * @param l 结束指针
     */
    void insert(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 迭代器范围插入
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    void insert(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 初始化列表插入
     * @param list 初始化列表
     * @note 与std::unordered_multiset::insert(std::initializer_list)兼容
     */
    void insert(const std::initializer_list<Value>& list)
    {
        m_hashTable.insert_equal(list.begin(), list.end());
    }

    // ==================== 查找接口（STL兼容） ====================

    /**
     * @brief 查找元素
     * @param key 要查找的键（元素值）
     * @return 指向第一个匹配元素的迭代器，未找到返回end()
     * @note 与std::unordered_multiset::find()兼容
     */
    iterator find(const key_type& key)
    {
        return m_hashTable.find(key);
    }

    const_iterator find(const key_type& key) const
    {
        return m_hashTable.find(key);
    }

    /**
     * @brief 统计指定键的元素数量
     * @param key 要统计的键
     * @return 元素数量（可能大于1，因为是multiset）
     * @note 与std::unordered_multiset::count()兼容
     */
    size_type count(const key_type& key) const
    {
        return m_hashTable.count(key);
    }

    /**
     * @brief 获取指定键的元素范围
     * @param key 要查找的键
     * @return pair<iterator, iterator>表示范围
     * @note 与std::unordered_multiset::equal_range()兼容
     */
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const
    {
        return m_hashTable.equal_range(key);
    }

    std::pair<iterator, iterator> equal_range(const key_type& key)
    {
        return m_hashTable.equal_range(key);
    }

    // ==================== 删除接口（STL兼容） ====================

    /**
     * @brief 根据键删除所有匹配的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     * @note 与std::unordered_multiset::erase()兼容
     */
    size_type erase(const key_type& key)
    {
        return m_hashTable.erase(key);
    }

    /**
     * @brief 根据迭代器删除元素
     * @param it 指向要删除元素的迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_multiset::erase()兼容
     */
    iterator erase(iterator it)
    {
        return m_hashTable.erase(it);
    }

    /**
     * @brief 根据常量迭代器删除元素
     * @param it 指向要删除元素的常量迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_multiset::erase()兼容
     */
    iterator erase(const_iterator it)
    {
        return m_hashTable.erase(it);
    }

    /**
     * @brief 删除指定范围的元素
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_multiset::erase(first, last)兼容
     */
    iterator erase(const_iterator f, const_iterator l)
    {
        return m_hashTable.erase(f, l);
    }

    /**
     * @brief 清空所有元素
     * @note 与std::unordered_multiset::clear()兼容
     */
    void clear()
    {
        m_hashTable.clear();
    }

public:
    // ==================== 桶接口（STL兼容） ====================

    /**
     * @brief 调整大小提示
     * @param hint 元素数量提示
     * @note 与std::unordered_multiset::rehash()类似但实际不执行操作（固定容量）
     */
    void resize(size_type hint)
    {
        m_hashTable.resize(hint);
    }

    /**
     * @brief 获取桶数量
     * @return 桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_multiset::bucket_count()兼容，但返回固定值
     */
    size_type bucket_count() const
    {
        return m_hashTable.bucket_count();
    }

    /**
     * @brief 获取最大桶数量
     * @return 最大桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_multiset::max_bucket_count()兼容
     */
    size_type max_bucket_count() const
    {
        return m_hashTable.max_bucket_count();
    }

    /**
     * @brief 获取指定桶中的元素数量
     * @param n 桶索引
     * @return 该桶中的元素数量
     * @note 与std::unordered_multiset::bucket_size()类似
     */
    size_type elems_in_bucket(size_type n) const
    {
        return m_hashTable.elems_in_bucket(n);
    }
};

// ==================== 实现部分 ====================

/**
 * @brief 从std::set赋值实现
 * @param x 源std::set对象
 * @return 自身引用
 * @note STL容器没有此接口，为方便从有序容器转换而提供
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiSetWithList<Value, MAX_SIZE>& NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::set<Value>& x)
{
    clear();
    m_hashTable.insert_equal(x.begin(), x.end());
    return *this;
}

/**
 * @brief 从std::unordered_set赋值实现
 * @param x 源std::unordered_set对象
 * @return 自身引用
 * @note STL容器没有此接口，为方便从STL无序容器转换而提供
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiSetWithList<Value, MAX_SIZE>& NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::unordered_set<Value>& x)
{
    clear();
    m_hashTable.insert_equal(x.begin(), x.end());
    return *this;
}

/**
 * @brief 从std::multiset赋值实现
 * @param x 源std::multiset对象
 * @return 自身引用
 * @note STL容器没有此接口，为方便从有序多重容器转换而提供
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiSetWithList<Value, MAX_SIZE>& NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::multiset<Value>& x)
{
    clear();
    m_hashTable.insert_equal(x.begin(), x.end());
    return *this;
}

/**
 * @brief 从std::unordered_multiset赋值实现
 * @param x 源std::unordered_multiset对象
 * @return 自身引用
 * @note STL容器没有此接口，为方便从STL无序多重容器转换而提供
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiSetWithList<Value, MAX_SIZE>& NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::unordered_multiset<Value>& x)
{
    clear();
    m_hashTable.insert_equal(x.begin(), x.end());
    return *this;
}

/**
 * @brief 拷贝赋值操作符实现
 * @param x 源NFShmHashMultiSetWithList对象
 * @return 自身引用
 * @note 与std::unordered_multiset::operator=兼容
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiSetWithList<Value, MAX_SIZE>& NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const NFShmHashMultiSetWithList<Value, MAX_SIZE>& x)
{
    if (this != &x)
    {
        clear();
        m_hashTable = x.m_hashTable;
    }
    return *this;
}

/**
 * @brief 从初始化列表赋值实现
 * @param x 初始化列表
 * @return 自身引用
 * @note 与std::unordered_multiset::operator=(std::initializer_list)兼容
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiSetWithList<Value, MAX_SIZE>& NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::initializer_list<Value>& x)
{
    clear();
    m_hashTable.insert_equal(x.begin(), x.end());
    return *this;
}

/**
 * @brief 就地构造元素实现
 * @tparam Args 构造参数类型包
 * @param args 构造参数
 * @return 指向插入元素的迭代器
 * @note 与std::unordered_multiset::emplace()兼容，总是成功插入
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename... Args>
typename NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::iterator
NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::emplace(const Args&... args)
{
    value_type obj(args...);
    return m_hashTable.insert_equal(obj);
}

/**
 * @brief 带提示就地构造元素实现
 * @tparam Args 构造参数类型包
 * @param hint 位置提示迭代器（忽略）
 * @param args 构造参数
 * @return 指向插入元素的迭代器
 * @note 与std::unordered_multiset::emplace_hint()兼容，但忽略提示参数
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename... Args>
typename NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::iterator
NFShmHashMultiSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::emplace_hint(const_iterator hint, const Args&... args)
{
    (void)hint; // 忽略提示参数
    value_type obj(args...);
    return m_hashTable.insert_equal(obj);
}

// ==================== 全局操作符 ====================

/**
 * @brief 相等比较操作符
 * @param hs1 第一个NFShmHashMultiSetWithList对象
 * @param hs2 第二个NFShmHashMultiSetWithList对象
 * @return true表示相等
 * @note 与std::unordered_multiset的operator==兼容
 * @note 比较所有元素是否相等，包括重复元素的数量，顺序无关
 */
template <class Val, int MAX_SIZE, class HashFcn, class EqualKey>
bool operator==(const NFShmHashMultiSetWithList<Val, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashMultiSetWithList<Val, MAX_SIZE, HashFcn, EqualKey>& hs2)
{
    return hs1.m_hashTable == hs2.m_hashTable;
}

template <class Val, int MAX_SIZE, class HashFcn, class EqualKey>
void swap(const NFShmHashMultiSetWithList<Val, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashMultiSetWithList<Val, MAX_SIZE, HashFcn, EqualKey>& hs2)
{
    hs1.m_hashTable.swap(hs2.m_hashTable);
}
