// -------------------------------------------------------------------------
//    @FileName         :    NFShmHashMultiSet.h
//    @Author           :    gaoyi
//    @Date             :    23-2-11
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmHashMultiSet
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmHashTable.h"
#include <set>
#include <unordered_set>

/**
 * @file NFShmHashMultiSet.h
 * @brief 基于共享内存的无序多重集合容器实现，与STL std::unordered_multiset高度兼容
 * 
 * @section overview 概述
 * 
 * NFShmHashMultiSet 是一个专为共享内存环境设计的无序多重集合容器，允许存储重复元素。
 * 在API设计上与STL std::unordered_multiset高度兼容，但在内存管理、容量控制和性能特征方面
 * 针对共享内存场景进行了深度优化。它提供O(1)平均时间复杂度的操作，同时支持进程间数据共享。
 * 
 * @section features 核心特性
 * 
 * 1. **多重集合语义**：
 *    - 允许存储相同元素的多个副本
 *    - 相同元素在迭代时保持相邻性
 *    - insert操作总是成功（除非容量满）
 *    - count()返回指定元素的副本数量
 * 
 * 2. **STL高度兼容**：
 *    - 完整的std::unordered_multiset API支持
 *    - 标准的迭代器接口和类型定义
 *    - find()、count()、equal_range()等常用操作
 *    - 支持范围for循环和STL算法
 * 
 * 3. **共享内存优化**：
 *    - 固定大小内存布局，避免动态分配
 *    - 元素直接存储，无额外包装
 *    - 支持CREATE/RESUME两阶段初始化
 *    - 无内存碎片，高效的内存使用
 * 
 * 4. **性能特征**：
 *    - O(1)平均时间复杂度的哈希操作
 *    - 无rehash开销，稳定的性能表现
 *    - 链地址法解决冲突，固定桶数量
 *    - 预分配节点池，快速内存分配
 * 
 * @section stl_comparison 与STL std::unordered_multiset对比
 * 
 * | 特性 | std::unordered_multiset | NFShmHashMultiSet |
 * |------|------------------------|-------------------|
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
 * // 定义容量为1000的整数多重集合
 * NFShmHashMultiSet<int, 1000> numberMultiSet;
 * numberMultiSet.CreateInit();  // 创建模式初始化
 * 
 * // 插入元素（允许重复）
 * numberMultiSet.insert(42);
 * numberMultiSet.insert(42);  // 重复插入，会成功
 * numberMultiSet.insert(42);  // 再次插入，也会成功
 * 
 * std::cout << "Count of 42: " << numberMultiSet.count(42) << std::endl;  // 输出：3
 * 
 * // 批量插入（包含重复元素）
 * std::vector<int> numbers = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
 * numberMultiSet.insert(numbers.begin(), numbers.end());
 * 
 * std::cout << "Total size: " << numberMultiSet.size() << std::endl;  // 输出：13 (3+10)
 * 
 * // 统计各个元素的数量
 * for (int i = 1; i <= 4; ++i) {
 *     std::cout << "Count of " << i << ": " << numberMultiSet.count(i) << std::endl;
 * }
 * // 输出：
 * // Count of 1: 1
 * // Count of 2: 2  
 * // Count of 3: 3
 * // Count of 4: 4
 * 
 * // 查找操作（找到第一个匹配的元素）
 * auto it = numberMultiSet.find(3);
 * if (it != numberMultiSet.end()) {
 *     std::cout << "Found element: " << *it << std::endl;
 * }
 * 
 * // 获取某个元素的所有副本
 * auto range = numberMultiSet.equal_range(42);
 * std::cout << "All copies of 42: ";
 * for (auto it = range.first; it != range.second; ++it) {
 *     std::cout << *it << " ";
 * }
 * std::cout << std::endl;  // 输出：42 42 42
 * ```
 * 
 * @subsection frequency_counting 频率统计应用
 * 
 * ```cpp
 * NFShmHashMultiSet<std::string, 1000> wordFrequency;
 * 
 * // 模拟单词频率统计
 * std::vector<std::string> text = {
 *     "hello", "world", "hello", "cpp", "world", "hello",
 *     "programming", "cpp", "hello", "world"
 * };
 * 
 * // 插入所有单词（重复的也会插入）
 * for (const auto& word : text) {
 *     wordFrequency.insert(word);
 * }
 * 
 * // 统计每个单词的频率
 * std::set<std::string> uniqueWords(text.begin(), text.end());  // 获取唯一单词
 * 
 * std::cout << "Word frequencies:" << std::endl;
 * for (const auto& word : uniqueWords) {
 *     size_t count = wordFrequency.count(word);
 *     std::cout << word << ": " << count << std::endl;
 * }
 * // 输出：
 * // cpp: 2
 * // hello: 4
 * // programming: 1
 * // world: 3
 * 
 * // 找到出现次数最多的单词
 * std::string mostFrequent;
 * size_t maxCount = 0;
 * for (const auto& word : uniqueWords) {
 *     size_t count = wordFrequency.count(word);
 *     if (count > maxCount) {
 *         maxCount = count;
 *         mostFrequent = word;
 *     }
 * }
 * std::cout << "Most frequent word: " << mostFrequent 
 *           << " (appears " << maxCount << " times)" << std::endl;
 * ```
 * 
 * @subsection batch_operations 批量操作
 * 
 * ```cpp
 * NFShmHashMultiSet<int, 500> scoreMultiSet;
 * 
 * // 模拟游戏分数记录（允许相同分数）
 * std::vector<int> gameScores = {
 *     100, 85, 90, 100, 95, 85, 100, 88, 92, 85,
 *     100, 90, 87, 95, 100, 85, 90, 100, 95, 85
 * };
 * 
 * scoreMultiSet.insert(gameScores.begin(), gameScores.end());
 * 
 * std::cout << "Total games played: " << scoreMultiSet.size() << std::endl;
 * 
 * // 分析分数分布
 * std::set<int> uniqueScores(gameScores.begin(), gameScores.end());
 * std::cout << "Score distribution:" << std::endl;
 * for (const auto& score : uniqueScores) {
 *     size_t count = scoreMultiSet.count(score);
 *     std::cout << "Score " << score << ": " << count << " times" << std::endl;
 * }
 * 
 * // 删除特定分数的所有记录
 * size_t removed = scoreMultiSet.erase(85);
 * std::cout << "Removed " << removed << " scores of 85" << std::endl;
 * 
 * // 删除低于90分的所有记录
 * for (auto it = scoreMultiSet.begin(); it != scoreMultiSet.end();) {
 *     if (*it < 90) {
 *         it = scoreMultiSet.erase(it);
 *     } else {
 *         ++it;
 *     }
 * }
 * 
 * std::cout << "After removing scores < 90: " << scoreMultiSet.size() << " scores left" << std::endl;
 * ```
 * 
 * @subsection capacity_management 容量管理
 * 
 * ```cpp
 * NFShmHashMultiSet<int, 100> limitedMultiSet;
 * 
 * // 容量检查（STL没有的功能）
 * std::cout << "Max size: " << limitedMultiSet.max_size() << std::endl;      // 100
 * std::cout << "Current size: " << limitedMultiSet.size() << std::endl;       // 0
 * std::cout << "Is full: " << limitedMultiSet.full() << std::endl;           // false
 * std::cout << "Left space: " << limitedMultiSet.left_size() << std::endl;   // 100
 * 
 * // 批量插入相同元素直到容量满
 * int value = 42;
 * while (!limitedMultiSet.full()) {
 *     auto it = limitedMultiSet.insert(value);
 *     if (it == limitedMultiSet.end()) {  // 插入失败
 *         break;
 *     }
 * }
 * 
 * std::cout << "Inserted " << limitedMultiSet.count(42) 
 *           << " copies of " << value << std::endl;  // 输出：100
 * 
 * // 检查最终状态
 * std::cout << "Final size: " << limitedMultiSet.size() << std::endl;         // 100
 * std::cout << "Is full: " << limitedMultiSet.full() << std::endl;           // true
 * std::cout << "Left space: " << limitedMultiSet.left_size() << std::endl;   // 0
 * 
 * // 验证所有元素都是相同的
 * std::cout << "All elements are " << value << ": " << std::boolalpha 
 *           << (limitedMultiSet.count(value) == limitedMultiSet.size()) << std::endl;  // true
 * ```
 * 
 * @subsection shared_memory_usage 共享内存使用
 * 
 * ```cpp
 * // 进程A：创建共享的多重集合
 * NFShmHashMultiSet<int, 1000> sharedMultiSet;
 * if (sharedMultiSet.CreateInit() == 0) {  // 创建成功
 *     // 添加一些重复数据
 *     std::vector<int> data = {1, 1, 2, 2, 2, 3, 3, 3, 3};
 *     sharedMultiSet.insert(data.begin(), data.end());
 *     std::cout << "Created shared multiset with " << sharedMultiSet.size() << " elements" << std::endl;
 * }
 * 
 * // 进程B：恢复已存在的共享内存多重集合
 * NFShmHashMultiSet<int, 1000> restoredMultiSet;
 * if (restoredMultiSet.ResumeInit() == 0) {  // 恢复成功
 *     std::cout << "Restored multiset with " << restoredMultiSet.size() << " elements" << std::endl;
 *     
 *     // 检查进程A创建的数据
 *     for (int i = 1; i <= 3; ++i) {
 *         auto count = restoredMultiSet.count(i);
 *         std::cout << "Element " << i << " appears " << count << " times from Process A" << std::endl;
 *     }
 *     
 *     // 添加进程B的数据
 *     restoredMultiSet.insert(1);  // 增加一个1
 *     restoredMultiSet.insert(4);  // 添加新元素4
 *     restoredMultiSet.insert(4);  // 再添加一个4
 *     
 *     std::cout << "After Process B additions:" << std::endl;
 *     std::cout << "Element 1 now appears " << restoredMultiSet.count(1) << " times" << std::endl;
 *     std::cout << "Element 4 now appears " << restoredMultiSet.count(4) << " times" << std::endl;
 * }
 * ```
 * 
 * @subsection stl_interop STL容器互操作
 * 
 * ```cpp
 * // 从STL容器构造
 * std::unordered_multiset<std::string> stdMultiSet = {
 *     "apple", "banana", "apple", "cherry", "banana", "apple"
 * };
 * 
 * NFShmHashMultiSet<std::string, 1000> shmMultiSet(stdMultiSet);  // 从STL构造
 * std::cout << "Converted from STL: " << shmMultiSet.size() << " elements" << std::endl;
 * 
 * // 转换回STL容器
 * std::unordered_multiset<std::string> convertedSet;
 * for (const auto& element : shmMultiSet) {
 *     convertedSet.insert(element);
 * }
 * 
 * // 使用STL算法统计频率
 * std::map<std::string, size_t> frequency;
 * for (const auto& element : shmMultiSet) {
 *     frequency[element]++;
 * }
 * 
 * std::cout << "Element frequencies:" << std::endl;
 * for (const auto& pair : frequency) {
 *     std::cout << pair.first << ": " << pair.second << std::endl;
 * }
 * 
 * // 使用STL算法查找
 * auto count = std::count(shmMultiSet.begin(), shmMultiSet.end(), "apple");
 * std::cout << "STL count of 'apple': " << count << std::endl;
 * 
 * // 使用容器自己的count方法（更高效）
 * auto nativeCount = shmMultiSet.count("apple");
 * std::cout << "Native count of 'apple': " << nativeCount << std::endl;
 * ```
 * 
 * @section performance_notes 性能说明
 * 
 * - **查找性能**：O(1)平均，O(n)最坏（链表长度）
 * - **插入性能**：O(1)平均，总是成功（除非容量满）
 * - **删除性能**：O(1)平均，支持按值删除所有副本
 * - **范围查询**：equal_range() O(1)平均 + O(k)遍历（k为相同元素数）
 * - **计数性能**：count() O(1)平均，直接统计相同元素数量
 * - **内存性能**：零碎片，预分配，缓存友好
 * - **并发性能**：需要外部同步，但支持多进程读写
 * 
 * @section migration_guide 迁移指南
 * 
 * 从std::unordered_multiset迁移到NFShmHashMultiSet：
 * 
 * 1. **替换类型声明**：
 *    ```cpp
 *    // 原代码
 *    std::unordered_multiset<int> multiset;
 *    
 *    // 新代码（添加容量模板参数）
 *    NFShmHashMultiSet<int, 10000> multiset;
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
 *    multiset.insert(element);  // 总是成功（除非容量满）
 *    
 *    auto count = multiset.count(element);  // 获取元素数量
 *    auto range = multiset.equal_range(element);  // 获取范围
 *    ```
 * 
 * 4. **处理容量限制**：
 *    ```cpp
 *    // 检查容量状态
 *    if (!multiset.full()) {
 *        multiset.insert(element);
 *    } else {
 *        // 处理容量满的情况
 *    }
 *    ```
 * 
 * 5. **移除不支持的操作**：
 *    ```cpp
 *    // 移除这些调用（NFShmHashMultiSet不支持）
 *    // multiset.rehash(1000);     // 动态管理不支持
 *    // multiset.reserve(500);     // 动态管理不支持
 *    // multiset.max_load_factor(0.8);  // 负载因子管理不支持
 *    ```
 */

/****************************************************************************
 * STL std::unordered_multiset 对比分析
 ****************************************************************************
 * 
 * 1. 内存管理策略对比：
 *    - std::unordered_multiset: 动态内存分配，使用allocator管理堆内存
 *    - NFShmHashMultiSet: 固定大小共享内存，预分配所有节点，支持进程间共享
 * 
 * 2. 容量管理对比：
 *    - std::unordered_multiset: 动态扩容，load_factor超过阈值时自动rehash
 *    - NFShmHashMultiSet: 固定容量MAX_SIZE，不支持动态扩容
 * 
 * 3. 重复元素处理对比：
 *    - 都允许相同元素的多个副本存在
 *    - 都在相同元素之间保持相邻性
 *    - insert操作都总是成功（除非容量不足）
 * 
 * 4. 元素存储对比：
 *    - std::unordered_multiset: 直接存储元素值
 *    - NFShmHashMultiSet: 同样直接存储元素值，使用std::_Identity函数提取键
 * 
 * 5. 性能特征对比：
 *    - std::unordered_multiset: O(1)平均时间复杂度，但可能触发rehash开销
 *    - NFShmHashMultiSet: O(1)平均时间复杂度，无rehash开销，但可能因固定容量产生更多冲突
 * 
 * 6. API兼容性：
 *    - 兼容接口：insert, find, erase, begin, end, size, empty, count, equal_range等
 *    - 特有接口：full(), left_size(), CreateInit(), ResumeInit()等共享内存特有功能
 *    - 缺少接口：rehash, reserve, bucket, load_factor等动态管理接口
 * 
 * 7. 构造和初始化对比：
 *    - std::unordered_multiset: 标准构造函数，支持多种初始化方式
 *    - NFShmHashMultiSet: 支持从STL容器构造，增加共享内存特有的CreateInit/ResumeInit
 * 
 * 8. 与NFShmHashSet的区别：
 *    - NFShmHashSet: 保证元素唯一性，使用insert_unique
 *    - NFShmHashMultiSet: 允许重复元素，使用insert_equal
 *****************************************************************************/

// ==================== 前向声明 ====================

template <class Value, int MAX_SIZE,
          class HashFcn = std::hash<Value>,
          class EqualKey = std::equal_to<Value>>
class NFShmHashMultiSet;

template <class Val, int MAX_SIZE, class HashFcn, class EqualKey>
bool operator==(const NFShmHashMultiSet<Val, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashMultiSet<Val, MAX_SIZE, HashFcn, EqualKey>& hs2);

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
class NFShmHashMultiSet
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

public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 默认构造函数
     * @note 根据SHM_CREATE_MODE决定创建或恢复模式
     * @note 与std::unordered_multiset()行为类似，但增加共享内存初始化
     */
    NFShmHashMultiSet()
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
    NFShmHashMultiSet(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 数组构造函数
     * @param f 起始指针
     * @param l 结束指针
     * @note 与std::unordered_multiset兼容
     */
    NFShmHashMultiSet(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    NFShmHashMultiSet(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 拷贝构造函数
     * @param x 源NFShmHashMultiSet对象
     * @note 与std::unordered_multiset拷贝构造函数兼容
     */
    NFShmHashMultiSet(const NFShmHashMultiSet& x)
    {
        if (this != &x) m_hashTable = x.m_hashTable;
    }

    /**
     * @brief 从std::unordered_set构造
     * @param set 源std::unordered_set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiSet(const std::unordered_set<Value>& set)
    {
        m_hashTable.insert_equal(set.begin(), set.end());
    }

    /**
     * @brief 从std::set构造
     * @param set 源std::set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiSet(const std::set<Value>& set)
    {
        m_hashTable.insert_equal(set.begin(), set.end());
    }

    /**
     * @brief 从std::unordered_multiset构造
     * @param set 源std::unordered_multiset对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiSet(const std::unordered_multiset<Value>& set)
    {
        m_hashTable.insert_equal(set.begin(), set.end());
    }

    /**
     * @brief 从std::multiset构造
     * @param set 源std::multiset对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiSet(const std::multiset<Value>& set)
    {
        m_hashTable.insert_equal(set.begin(), set.end());
    }

    /**
     * @brief 初始化列表构造函数
     * @param list 初始化列表
     * @note 与std::unordered_multiset(std::initializer_list)兼容
     */
    NFShmHashMultiSet(const std::initializer_list<Value>& list)
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
        new(this) NFShmHashMultiSet();
    }

    // ==================== 赋值操作符 ====================

    /**
     * @brief 拷贝赋值操作符
     * @param x 源NFShmHashMultiSet对象
     * @return 自身引用
     * @note 与std::unordered_multiset::operator=兼容
     */
    NFShmHashMultiSet<Value, MAX_SIZE>& operator=(const NFShmHashMultiSet<Value, MAX_SIZE>& x);

    /**
     * @brief 从std::unordered_multiset赋值
     * @param x 源std::unordered_multiset对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiSet<Value, MAX_SIZE>& operator=(const std::unordered_multiset<Value>& x);

    /**
     * @brief 从std::multiset赋值
     * @param x 源std::multiset对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiSet<Value, MAX_SIZE>& operator=(const std::multiset<Value>& x);

    /**
     * @brief 从std::unordered_set赋值
     * @param x 源std::unordered_set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiSet<Value, MAX_SIZE>& operator=(const std::unordered_set<Value>& x);

    /**
     * @brief 从std::set赋值
     * @param x 源std::set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiSet<Value, MAX_SIZE>& operator=(const std::set<Value>& x);

    /**
     * @brief 从初始化列表赋值
     * @param x 初始化列表
     * @return 自身引用
     * @note 与std::unordered_multiset::operator=(std::initializer_list)兼容
     */
    NFShmHashMultiSet<Value, MAX_SIZE>& operator=(const std::initializer_list<Value>& x);

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
     * @param hs 另一个NFShmHashMultiSet对象
     * @note 与std::unordered_multiset::swap()兼容
     */
    void swap(NFShmHashMultiSet& hs) noexcept { m_hashTable.swap(hs.m_hashTable); }

    // ==================== 友元比较操作符 ====================

    template <class Val, int O_MAX_SIZE, class Hf, class EqK>
    friend bool operator==(const NFShmHashMultiSet<Val, O_MAX_SIZE, Hf, EqK>&,
                           const NFShmHashMultiSet<Val, O_MAX_SIZE, Hf, EqK>&);

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
NFShmHashMultiSet<Value, MAX_SIZE>& NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::set<Value>& x)
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
NFShmHashMultiSet<Value, MAX_SIZE>& NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::unordered_set<Value>& x)
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
NFShmHashMultiSet<Value, MAX_SIZE>& NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::multiset<Value>& x)
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
NFShmHashMultiSet<Value, MAX_SIZE>& NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::unordered_multiset<Value>& x)
{
    clear();
    m_hashTable.insert_equal(x.begin(), x.end());
    return *this;
}

/**
 * @brief 拷贝赋值操作符实现
 * @param x 源NFShmHashMultiSet对象
 * @return 自身引用
 * @note 与std::unordered_multiset::operator=兼容
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiSet<Value, MAX_SIZE>& NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const NFShmHashMultiSet<Value, MAX_SIZE>& x)
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
NFShmHashMultiSet<Value, MAX_SIZE>& NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::initializer_list<Value>& x)
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
typename NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::iterator
NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::emplace(const Args&... args)
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
typename NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::iterator
NFShmHashMultiSet<Value, MAX_SIZE, HashFcn, EqualKey>::emplace_hint(const_iterator hint, const Args&... args)
{
    (void)hint; // 忽略提示参数
    value_type obj(args...);
    return m_hashTable.insert_equal(obj);
}

// ==================== 全局操作符 ====================

/**
 * @brief 相等比较操作符
 * @param hs1 第一个NFShmHashMultiSet对象
 * @param hs2 第二个NFShmHashMultiSet对象
 * @return true表示相等
 * @note 与std::unordered_multiset的operator==兼容
 * @note 比较所有元素是否相等，包括重复元素的数量，顺序无关
 */
template <class Val, int MAX_SIZE, class HashFcn, class EqualKey>
bool operator==(const NFShmHashMultiSet<Val, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashMultiSet<Val, MAX_SIZE, HashFcn, EqualKey>& hs2)
{
    return hs1.m_hashTable == hs2.m_hashTable;
}

template <class Val, int MAX_SIZE, class HashFcn, class EqualKey>
void swap(const NFShmHashMultiSet<Val, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashMultiSet<Val, MAX_SIZE, HashFcn, EqualKey>& hs2)
{
    hs1.m_hashTable.swap(hs2.m_hashTable);
}
