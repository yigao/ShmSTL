// -------------------------------------------------------------------------
//    @FileName         :    NFShmHashMultiMap.h
//    @Author           :    gaoyi
//    @Date             :    23-2-11
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmHashMultiMap
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmHashTable.h"
#include <map>
#include <unordered_map>
#include "NFShmPair.h"

/**
 * @file NFShmHashMultiMap.h
 * @brief 基于共享内存的无序多重映射容器实现，与STL std::unordered_multimap高度兼容
 * 
 * @section overview 概述
 * 
 * NFShmHashMultiMap 是一个专为共享内存环境设计的多重键值映射容器，允许相同键对应多个值。
 * 在API设计上与STL std::unordered_multimap高度兼容，但在内存管理、容量控制和性能特征方面
 * 针对共享内存场景进行了深度优化。它提供O(1)平均时间复杂度的操作，同时支持进程间数据共享。
 * 
 * @section features 核心特性
 * 
 * 1. **多重映射支持**：
 *    - 允许相同键存储多个不同值
 *    - 相同键的元素在迭代时保持相邻性
 *    - equal_range()提供相同键的元素范围
 *    - count()返回指定键的元素数量
 * 
 * 2. **STL高度兼容**：
 *    - 完整的std::unordered_multimap API支持
 *    - 标准的迭代器接口和类型定义
 *    - find()、count()、equal_range()等常用操作
 *    - 支持范围for循环和STL算法
 * 
 * 3. **共享内存优化**：
 *    - 固定大小内存布局，避免动态分配
 *    - 基于NFShmPair的键值对存储
 *    - 支持CREATE/RESUME两阶段初始化
 *    - 无内存碎片，高效的内存使用
 * 
 * 4. **性能特征**：
 *    - O(1)平均时间复杂度的哈希操作
 *    - 无rehash开销，稳定的性能表现
 *    - 链地址法解决冲突，固定桶数量
 *    - 预分配节点池，快速内存分配
 * 
 * @section stl_comparison 与STL std::unordered_multimap对比
 * 
 * | 特性 | std::unordered_multimap | NFShmHashMultiMap |
 * |------|------------------------|-------------------|
 * | **内存管理** | 动态堆内存分配 | 固定共享内存预分配 |
 * | **容量限制** | 无限制，动态扩容 | 固定容量MAX_SIZE |
 * | **扩容机制** | 自动rehash扩容 | **不支持扩容** |
 * | **键值对类型** | std::pair<const Key, T> | NFShmPair<Key, T> |
 * | **多重映射** | 完全支持 | 完全支持 |
 * | **相同键相邻性** | 保证 | 保证 |
 * | **插入语义** | insert总是成功 | insert总是成功（除非满） |
 * | **operator[]** | **不支持**（语义不明确） | **不支持** |
 * | **equal_range** | 完全支持 | 完全支持 |
 * | **count操作** | 返回元素数量 | 返回元素数量 |
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
 * - **STL转换**：从std::unordered_multimap和std::multimap构造
 * 
 * **不支持的接口**：
 * - **访问操作**：operator[]、at()（多重映射语义不明确）
 * - **动态管理**：rehash(), reserve(), max_load_factor()
 * - **哈希策略**：load_factor(), bucket_size()
 * - **自定义分配器**：get_allocator()
 * 
 * @section usage_examples 使用示例
 * 
 * @subsection basic_usage 基础用法（类似std::unordered_multimap）
 * 
 * ```cpp
 * // 定义容量为1000的字符串到整数多重映射
 * NFShmHashMultiMap<std::string, int, 1000> scoreMap;
 * scoreMap.CreateInit();  // 创建模式初始化
 * 
 * // 多重映射：同一个学生可以有多个成绩
 * scoreMap.insert({"alice", 90});
 * scoreMap.insert({"alice", 85});
 * scoreMap.insert({"alice", 95});
 * scoreMap.insert({"bob", 80});
 * scoreMap.insert({"bob", 88});
 * 
 * // 查找操作
 * auto it = scoreMap.find("alice");
 * if (it != scoreMap.end()) {
 *     std::cout << "Found alice's first score: " << it->second << std::endl;
 * }
 * 
 * // 统计某个键的元素数量
 * std::cout << "Alice has " << scoreMap.count("alice") << " scores" << std::endl;  // 输出：3
 * 
 * // 获取某个键的所有值
 * auto range = scoreMap.equal_range("alice");
 * std::cout << "Alice's all scores: ";
 * for (auto it = range.first; it != range.second; ++it) {
 *     std::cout << it->second << " ";
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @subsection multi_value_operations 多值操作
 * 
 * ```cpp
 * NFShmHashMultiMap<std::string, std::string, 500> categoryMap;
 * 
 * // 构建类别到项目的多重映射
 * categoryMap.insert({"fruit", "apple"});
 * categoryMap.insert({"fruit", "banana"});
 * categoryMap.insert({"fruit", "orange"});
 * categoryMap.insert({"vegetable", "carrot"});
 * categoryMap.insert({"vegetable", "lettuce"});
 * 
 * // 函数：打印某个类别的所有项目
 * auto printCategory = [&](const std::string& category) {
 *     std::cout << category << ": ";
 *     auto count = categoryMap.count(category);
 *     if (count > 0) {
 *         auto range = categoryMap.equal_range(category);
 *         for (auto it = range.first; it != range.second; ++it) {
 *             std::cout << it->second << " ";
 *         }
 *     }
 *     std::cout << "(" << count << " items)" << std::endl;
 * };
 * 
 * printCategory("fruit");      // 输出：fruit: apple banana orange (3 items)
 * printCategory("vegetable");  // 输出：vegetable: carrot lettuce (2 items)
 * printCategory("meat");       // 输出：meat: (0 items)
 * 
 * // 删除某个类别的所有项目
 * size_t removed = categoryMap.erase("fruit");
 * std::cout << "Removed " << removed << " fruit items" << std::endl;  // 输出：3
 * ```
 * 
 * @subsection capacity_management 容量管理
 * 
 * ```cpp
 * NFShmHashMultiMap<int, std::string, 100> limitedMap;
 * 
 * // 容量检查（STL没有的功能）
 * std::cout << "Max size: " << limitedMap.max_size() << std::endl;      // 100
 * std::cout << "Current size: " << limitedMap.size() << std::endl;       // 0
 * std::cout << "Is full: " << limitedMap.full() << std::endl;           // false
 * std::cout << "Left space: " << limitedMap.left_size() << std::endl;   // 100
 * 
 * // 批量插入相同键的多个值
 * for (int i = 0; i < 50; ++i) {
 *     limitedMap.insert({1, "value" + std::to_string(i)});  // 所有值都用键1
 * }
 * 
 * // 检查状态
 * std::cout << "Size after insert: " << limitedMap.size() << std::endl;        // 50
 * std::cout << "Count for key 1: " << limitedMap.count(1) << std::endl;        // 50
 * std::cout << "Left space: " << limitedMap.left_size() << std::endl;          // 50
 * 
 * // 继续插入直到容量满
 * int key = 2;
 * while (!limitedMap.full()) {
 *     auto it = limitedMap.insert({key, "data"});
 *     if (it == limitedMap.end()) {  // 插入失败
 *         break;
 *     }
 *     ++key;
 * }
 * 
 * std::cout << "Final state - Size: " << limitedMap.size() 
 *           << ", Full: " << limitedMap.full() << std::endl;
 * ```
 * 
 * @subsection shared_memory_usage 共享内存使用
 * 
 * ```cpp
 * // 进程A：创建共享的多重映射
 * NFShmHashMultiMap<int, std::string, 1000> sharedMultiMap;
 * if (sharedMultiMap.CreateInit() == 0) {  // 创建成功
 *     // 添加一些多重映射数据
 *     sharedMultiMap.insert({1, "Process A - Item 1"});
 *     sharedMultiMap.insert({1, "Process A - Item 2"});
 *     sharedMultiMap.insert({2, "Process A - Data"});
 *     std::cout << "Created shared multimap with " << sharedMultiMap.size() << " items" << std::endl;
 * }
 * 
 * // 进程B：恢复已存在的共享内存多重映射
 * NFShmHashMultiMap<int, std::string, 1000> restoredMultiMap;
 * if (restoredMultiMap.ResumeInit() == 0) {  // 恢复成功
 *     std::cout << "Restored multimap with " << restoredMultiMap.size() << " items" << std::endl;
 *     
 *     // 访问进程A创建的数据
 *     auto count1 = restoredMultiMap.count(1);
 *     std::cout << "Key 1 has " << count1 << " values from Process A" << std::endl;
 *     
 *     // 添加进程B的数据
 *     restoredMultiMap.insert({1, "Process B - Additional"});
 *     restoredMultiMap.insert({3, "Process B - New Key"});
 *     
 *     // 现在键1有3个值（2个来自进程A，1个来自进程B）
 *     std::cout << "Key 1 now has " << restoredMultiMap.count(1) << " values total" << std::endl;
 * }
 * ```
 * 
 * @subsection stl_interop STL容器互操作
 * 
 * ```cpp
 * // 从STL容器构造
 * std::unordered_multimap<std::string, int> stdMultiMap = {
 *     {"math", 90}, {"math", 85}, {"english", 88}, {"english", 92}
 * };
 * 
 * NFShmHashMultiMap<std::string, int, 1000> shmMultiMap(stdMultiMap);  // 从STL构造
 * 
 * // 转换回STL容器
 * std::unordered_multimap<std::string, int> convertedMap;
 * for (const auto& pair : shmMultiMap) {
 *     convertedMap.insert({pair.first, pair.second});
 * }
 * 
 * // 使用STL算法
 * auto mathCount = std::count_if(shmMultiMap.begin(), shmMultiMap.end(),
 *     [](const auto& pair) { return pair.first == "math"; });
 * std::cout << "Math entries: " << mathCount << std::endl;
 * 
 * // 计算某个学科的平均分
 * std::string subject = "math";
 * auto range = shmMultiMap.equal_range(subject);
 * if (range.first != range.second) {
 *     int sum = 0, count = 0;
 *     for (auto it = range.first; it != range.second; ++it) {
 *         sum += it->second;
 *         ++count;
 *     }
 *     std::cout << subject << " average: " << (double)sum / count << std::endl;
 * }
 * ```
 * 
 * @section performance_notes 性能说明
 * 
 * - **查找性能**：O(1)平均，O(n)最坏（链表长度）
 * - **插入性能**：O(1)平均，总是成功（除非容量满）
 * - **删除性能**：O(1)平均，支持按键删除多个元素
 * - **范围查询**：equal_range() O(1)平均 + O(k)遍历（k为相同键元素数）
 * - **内存性能**：零碎片，预分配，缓存友好
 * - **并发性能**：需要外部同步，但支持多进程读写
 * 
 * @section migration_guide 迁移指南
 * 
 * 从std::unordered_multimap迁移到NFShmHashMultiMap：
 * 
 * 1. **替换类型声明**：
 *    ```cpp
 *    // 原代码
 *    std::unordered_multimap<int, std::string> multimap;
 *    
 *    // 新代码（添加容量模板参数）
 *    NFShmHashMultiMap<int, std::string, 10000> multimap;
 *    ```
 * 
 * 2. **添加初始化**：
 *    ```cpp
 *    // 添加共享内存初始化
 *    multimap.CreateInit();  // 或 ResumeInit()
 *    ```
 * 
 * 3. **保持多重映射语义**：
 *    ```cpp
 *    // 多重映射的核心操作保持不变
 *    multimap.insert({key, value1});
 *    multimap.insert({key, value2});  // 相同键，不同值
 *    
 *    auto count = multimap.count(key);  // 获取元素数量
 *    auto range = multimap.equal_range(key);  // 获取范围
 *    ```
 * 
 * 4. **处理容量限制**：
 *    ```cpp
 *    // 检查容量状态
 *    if (!multimap.full()) {
 *        multimap.insert({key, value});
 *    } else {
 *        // 处理容量满的情况
 *    }
 *    ```
 * 
 * 5. **移除不支持的操作**：
 *    ```cpp
 *    // 移除这些调用（NFShmHashMultiMap不支持）
 *    // multimap[key] = value;  // operator[] 不支持
 *    // auto val = multimap.at(key);  // at() 不支持
 *    // multimap.rehash(1000);  // 动态管理不支持
 *    ```
 */

/****************************************************************************
 * STL std::unordered_multimap 对比分析
 ****************************************************************************
 * 
 * 1. 内存管理策略对比：
 *    - std::unordered_multimap: 动态内存分配，使用allocator管理堆内存
 *    - NFShmHashMultiMap: 固定大小共享内存，预分配所有节点，支持进程间共享
 * 
 * 2. 容量管理对比：
 *    - std::unordered_multimap: 动态扩容，load_factor超过阈值时自动rehash
 *    - NFShmHashMultiMap: 固定容量MAX_SIZE，不支持动态扩容
 * 
 * 3. 重复元素处理对比：
 *    - 都允许相同键的多个元素存在
 *    - 都在相同键的元素之间保持相邻性
 *    - insert操作都总是成功（除非容量不足）
 * 
 * 4. 键值对存储对比：
 *    - std::unordered_multimap: std::pair<const Key, T>
 *    - NFShmHashMultiMap: NFShmPair<Key, T>（适配共享内存环境）
 * 
 * 5. 性能特征对比：
 *    - std::unordered_multimap: O(1)平均时间复杂度，但可能触发rehash开销
 *    - NFShmHashMultiMap: O(1)平均时间复杂度，无rehash开销，但可能因固定容量产生更多冲突
 * 
 * 6. API兼容性：
 *    - 兼容接口：insert, find, erase, begin, end, size, empty, count, equal_range等
 *    - 不同点：没有operator[]（多重映射特性决定）
 *    - 特有接口：full(), left_size(), CreateInit(), ResumeInit()等共享内存特有功能
 *    - 缺少接口：rehash, reserve, bucket, load_factor等动态管理接口
 * 
 * 7. 构造和初始化对比：
 *    - std::unordered_multimap: 标准构造函数，支持多种初始化方式
 *    - NFShmHashMultiMap: 支持从STL容器构造，增加共享内存特有的CreateInit/ResumeInit
 *****************************************************************************/

// ==================== 前向声明 ====================

template <class Key, class Tp, int MAX_SIZE, class HashFcn = std::hash<Key>, class EqualKey = std::equal_to<Key>>
class NFShmHashMultiMap;

template <class Key, class Tp, int MAX_SIZE, class Hf, class EqKey>
bool operator==(const NFShmHashMultiMap<Key, Tp, MAX_SIZE, Hf, EqKey>& hm1,
                const NFShmHashMultiMap<Key, Tp, MAX_SIZE, Hf, EqKey>& hm2);

// ==================== 主要容器类 ====================

/**
 * @brief 基于共享内存的无序多重映射容器
 * @tparam Key 键类型
 * @tparam Tp 值类型（映射的目标类型）
 * @tparam MAX_SIZE 最大容量（固定）
 * @tparam HashFcn 哈希函数类型，默认std::hash<Key>
 * @tparam EqualKey 键比较函数类型，默认std::equal_to<Key>
 * 
 * 设计特点：
 * 1. 固定容量，不支持动态扩容（与STL主要区别）
 * 2. 基于共享内存，支持进程间共享
 * 3. API设计尽量兼容STL std::unordered_multimap
 * 4. 允许重复键，支持一对多映射关系
 * 5. 使用NFShmPair代替std::pair适配共享内存环境
 * 
 * 与std::unordered_multimap的主要差异：
 * - 容量限制：固定大小 vs 动态扩容
 * - 内存管理：共享内存 vs 堆内存
 * - 性能特征：无rehash开销 vs 动态性能优化
 * - 进程支持：进程间共享 vs 单进程内使用
 * - 访问方式：没有operator[]（多重映射语义不明确）
 */
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
class NFShmHashMultiMap
{
private:
    /// @brief 底层哈希表类型，使用NFShmPair存储键值对，允许重复键
    typedef NFShmHashTable<NFShmPair<Key, Tp>, Key, MAX_SIZE, HashFcn, std::_Select1st<NFShmPair<Key, Tp>>, EqualKey> HashTable;
    HashTable m_hashTable; ///< 底层哈希表实例

public:
    // ==================== STL兼容类型定义 ====================

    typedef typename HashTable::key_type key_type; ///< 键类型
    typedef Tp data_type; ///< 数据类型（STL兼容）
    typedef Tp mapped_type; ///< 映射类型（STL标准）
    typedef typename HashTable::value_type value_type; ///< 值类型（NFShmPair<Key, Tp>）
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
     * @note 与std::unordered_multimap()行为类似，但增加共享内存初始化
     */
    NFShmHashMultiMap()
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
     * @note 与std::unordered_multimap(first, last)兼容
     */
    template <class InputIterator>
    NFShmHashMultiMap(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 数组构造函数
     * @param f 起始指针
     * @param l 结束指针
     * @note 与std::unordered_multimap兼容
     */
    NFShmHashMultiMap(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    NFShmHashMultiMap(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始迭代器
     * @param l 结束迭代器
     */
    NFShmHashMultiMap(iterator f, iterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 从std::unordered_map构造
     * @param map 源std::unordered_map对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiMap(const std::unordered_map<Key, Tp>& map)
    {
        m_hashTable.insert_equal(map.begin(), map.end());
    }

    /**
     * @brief 从std::unordered_multimap构造
     * @param map 源std::unordered_multimap对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiMap(const std::unordered_multimap<Key, Tp>& map)
    {
        m_hashTable.insert_equal(map.begin(), map.end());
    }

    /**
     * @brief 从std::map构造
     * @param map 源std::map对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiMap(const std::map<Key, Tp>& map)
    {
        m_hashTable.insert_equal(map.begin(), map.end());
    }

    /**
     * @brief 从std::multimap构造
     * @param map 源std::multimap对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiMap(const std::multimap<Key, Tp>& map)
    {
        m_hashTable.insert_equal(map.begin(), map.end());
    }

    /**
     * @brief 拷贝构造函数
     * @param x 源NFShmHashMultiMap对象
     * @note 与std::unordered_multimap拷贝构造函数兼容
     */
    NFShmHashMultiMap(const NFShmHashMultiMap& x)
    {
        if (this != &x) m_hashTable = x.m_hashTable;
    }

    /**
     * @brief 初始化列表构造函数
     * @param list 初始化列表
     * @note 与std::unordered_multimap(std::initializer_list)兼容
     */
    NFShmHashMultiMap(const std::initializer_list<value_type>& list)
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
        new(this) NFShmHashMultiMap();
    }

    // ==================== 赋值操作符 ====================

    /**
     * @brief 拷贝赋值操作符
     * @param x 源NFShmHashMultiMap对象
     * @return 自身引用
     * @note 与std::unordered_multimap::operator=兼容
     */
    NFShmHashMultiMap<Key, Tp, MAX_SIZE>& operator=(const NFShmHashMultiMap& x);

    /**
     * @brief 从std::unordered_map赋值
     * @param x 源std::unordered_map对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiMap<Key, Tp, MAX_SIZE>& operator=(const std::unordered_map<Key, Tp>& x);

    /**
     * @brief 从std::map赋值
     * @param x 源std::map对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiMap<Key, Tp, MAX_SIZE>& operator=(const std::map<Key, Tp>& x);

    /**
     * @brief 从std::unordered_multimap赋值
     * @param x 源std::unordered_multimap对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiMap<Key, Tp, MAX_SIZE>& operator=(const std::unordered_multimap<Key, Tp>& x);

    /**
     * @brief 从std::multimap赋值
     * @param x 源std::multimap对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiMap<Key, Tp, MAX_SIZE>& operator=(const std::multimap<Key, Tp>& x);

    /**
     * @brief 从初始化列表赋值
     * @param x 初始化列表
     * @return 自身引用
     * @note 与std::unordered_multimap::operator=(std::initializer_list)兼容
     */
    NFShmHashMultiMap<Key, Tp, MAX_SIZE>& operator=(const std::initializer_list<value_type>& x);

public:
    // ==================== 容量相关接口（STL兼容） ====================

    /**
     * @brief 获取当前元素数量
     * @return 元素数量
     * @note 与std::unordered_multimap::size()兼容
     */
    size_type size() const { return m_hashTable.size(); }

    /**
     * @brief 获取最大容量
     * @return 最大容量MAX_SIZE
     * @note 与std::unordered_multimap::max_size()不同，返回固定值而非理论最大值
     */
    size_type max_size() const { return m_hashTable.max_size(); }

    /**
     * @brief 判断是否为空
     * @return true表示空
     * @note 与std::unordered_multimap::empty()兼容
     */
    bool empty() const { return m_hashTable.empty(); }

    /**
     * @brief 交换两个容器的内容
     * @param hs 另一个NFShmHashMultiMap对象
     * @note 与std::unordered_multimap::swap()兼容
     */
    void swap(NFShmHashMultiMap& hs) noexcept { m_hashTable.swap(hs.m_hashTable); }

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

    // ==================== 友元比较操作符 ====================

    template <class K1, class T1, int O_MAX_SIZE, class Hf, class EqK>
    friend bool operator==(const NFShmHashMultiMap<K1, T1, O_MAX_SIZE, Hf, EqK>&,
                           const NFShmHashMultiMap<K1, T1, O_MAX_SIZE, Hf, EqK>&);

    // ==================== STL兼容迭代器接口 ====================

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     * @note 与std::unordered_multimap::begin()兼容
     */
    iterator begin() { return m_hashTable.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     * @note 与std::unordered_multimap::end()兼容
     */
    iterator end() { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_multimap::begin() const兼容
     */
    const_iterator begin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_multimap::end() const兼容
     */
    const_iterator end() const { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_multimap::cbegin()兼容
     */
    const_iterator cbegin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_multimap::cend()兼容
     */
    const_iterator cend() const { return m_hashTable.end(); }

public:
    // ==================== 插入接口（STL兼容） ====================

    /**
     * @brief 插入键值对（多重映射总是成功）
     * @param obj 要插入的键值对
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_multimap::insert()兼容，总是成功插入
     */
    iterator insert(const value_type& obj)
    {
        return m_hashTable.insert_equal(obj);
    }

    /**
     * @brief 带提示插入键值对
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param obj 要插入的键值对
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_multimap::insert(const_iterator, const value_type&)兼容
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
     * @note 与std::unordered_multimap::emplace()兼容
     */
    template <typename... Args>
    iterator emplace(const Args&... args);

    /**
     * @brief 带提示就地构造元素
     * @tparam Args 构造参数类型包
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_multimap::emplace_hint()兼容
     */
    template <typename... Args>
    iterator emplace_hint(const_iterator hint, const Args&... args);

    /**
     * @brief 范围插入
     * @tparam InputIterator 输入迭代器类型
     * @param f 起始迭代器
     * @param l 结束迭代器
     * @note 与std::unordered_multimap::insert(first, last)兼容
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
     * @note 与std::unordered_multimap::insert(std::initializer_list)兼容
     */
    void insert(const std::initializer_list<value_type>& list)
    {
        m_hashTable.insert_equal(list.begin(), list.end());
    }

    // ==================== 查找接口（STL兼容） ====================

    /**
     * @brief 查找元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的迭代器，未找到返回end()
     * @note 与std::unordered_multimap::find()兼容
     */
    iterator find(const key_type& key)
    {
        return m_hashTable.find(key);
    }

    /**
     * @brief 查找元素（常量版本）
     * @param key 要查找的键
     * @return 指向第一个匹配元素的常量迭代器
     * @note 与std::unordered_multimap::find() const兼容
     */
    const_iterator find(const key_type& key) const
    {
        return m_hashTable.find(key);
    }

    /**
     * @brief 统计指定键的元素数量
     * @param key 要统计的键
     * @return 元素数量（可能大于1）
     * @note 与std::unordered_multimap::count()兼容
     */
    size_type count(const key_type& key) const
    {
        return m_hashTable.count(key);
    }

    /**
     * @brief 获取指定键的元素范围
     * @param key 要查找的键
     * @return pair<iterator, iterator>表示范围
     * @note 与std::unordered_multimap::equal_range()兼容
     */
    std::pair<iterator, iterator> equal_range(const key_type& key)
    {
        return m_hashTable.equal_range(key);
    }

    /**
     * @brief 获取指定键的元素范围（常量版本）
     * @param key 要查找的键
     * @return pair<const_iterator, const_iterator>表示范围
     * @note 与std::unordered_multimap::equal_range() const兼容
     */
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const
    {
        return m_hashTable.equal_range(key);
    }

    // ==================== 删除接口（STL兼容） ====================

    /**
     * @brief 根据键删除所有匹配的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     * @note 与std::unordered_multimap::erase()兼容
     */
    size_type erase(const key_type& key)
    {
        return m_hashTable.erase(key);
    }

    /**
     * @brief 根据迭代器删除元素
     * @param it 指向要删除元素的迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_multimap::erase()兼容
     */
    iterator erase(iterator it)
    {
        return m_hashTable.erase(it);
    }

    /**
     * @brief 删除指定范围的元素
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_multimap::erase(first, last)兼容
     */
    iterator erase(const_iterator f, const_iterator l)
    {
        return m_hashTable.erase(f, l);
    }

    /**
     * @brief 清空所有元素
     * @note 与std::unordered_multimap::clear()兼容
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
     * @note 与std::unordered_multimap::rehash()类似但实际不执行操作（固定容量）
     */
    void resize(size_type hint)
    {
        m_hashTable.resize(hint);
    }

    /**
     * @brief 获取桶数量
     * @return 桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_multimap::bucket_count()兼容，但返回固定值
     */
    size_type bucket_count() const
    {
        return m_hashTable.bucket_count();
    }

    /**
     * @brief 获取最大桶数量
     * @return 最大桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_multimap::max_bucket_count()兼容
     */
    size_type max_bucket_count() const
    {
        return m_hashTable.max_bucket_count();
    }

    /**
     * @brief 获取指定桶中的元素数量
     * @param n 桶索引
     * @return 该桶中的元素数量
     * @note 与std::unordered_multimap::bucket_size()类似
     */
    size_type elems_in_bucket(size_type n) const
    {
        return m_hashTable.elems_in_bucket(n);
    }
};

// ==================== 全局操作符 ====================

/**
 * @brief 相等比较操作符
 * @param hm1 第一个NFShmHashMultiMap对象
 * @param hm2 第二个NFShmHashMultiMap对象
 * @return true表示相等
 * @note 与std::unordered_multimap的operator==兼容
 * @note 比较所有键值对是否相等，顺序无关
 */
template <class Key, class Tp, int MAX_SIZE, class Hf, class EqKey>
bool operator==(const NFShmHashMultiMap<Key, Tp, MAX_SIZE, Hf, EqKey>& hm1,
                const NFShmHashMultiMap<Key, Tp, MAX_SIZE, Hf, EqKey>& hm2)
{
    return hm1.m_hashTable == hm2.m_hashTable;
}

template <class Key, class Tp, int MAX_SIZE, class Hf, class EqKey>
void swap(const NFShmHashMultiMap<Key, Tp, MAX_SIZE, Hf, EqKey>& hm1,
                const NFShmHashMultiMap<Key, Tp, MAX_SIZE, Hf, EqKey>& hm2)
{
    hm1.m_hashTable.swap(hm2.m_hashTable);
}

// ==================== 实现部分 ====================
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMap<Key, Tp, MAX_SIZE>& NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const NFShmHashMultiMap& x)
{
    if (this != &x)
    {
        clear();
        insert(x.begin(), x.end());
    }
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMap<Key, Tp, MAX_SIZE>& NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const unordered_map<Key, Tp>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMap<Key, Tp, MAX_SIZE>& NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const map<Key, Tp>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMap<Key, Tp, MAX_SIZE>& NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const unordered_multimap<Key, Tp>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMap<Key, Tp, MAX_SIZE>& NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const multimap<Key, Tp>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMap<Key, Tp, MAX_SIZE>& NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::initializer_list<value_type>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename ... Args>
typename NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::iterator NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::emplace(const Args&... args)
{
    value_type obj(args...);
    return m_hashTable.insert_equal(obj);
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename ... Args>
typename NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::iterator NFShmHashMultiMap<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::emplace_hint(const_iterator hint, const Args&... args)
{
    value_type obj(args...);
    return m_hashTable.insert_equal(obj);
}
