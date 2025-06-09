// -------------------------------------------------------------------------
//    @FileName         :    NFShmHashtable.h
//    @Author           :    gaoyi
//    @Date             :    23-2-6
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmHashtable
//
// -------------------------------------------------------------------------

/**
 * @file NFShmHashTable.h
 * @brief 基于共享内存的哈希表实现，提供类似std::unordered_map/unordered_set的功能
 * 
 * @section overview 概述
 * 
 * NFShmHashTable 是一个专为共享内存环境设计的哈希表容器，为std::unordered_map
 * 和std::unordered_set提供共享内存兼容的替代方案。在API设计上高度兼容STL，但在内存管理、
 * 容量限制、冲突解决等方面针对共享内存环境进行了优化。
 * 
 * @section features 核心特性
 * 
 * 1. **哈希表结构**：
 *    - 链地址法(Separate Chaining)解决冲突
 *    - 固定桶数量，高效哈希分布
 *    - 支持唯一键(类似unordered_map)和重复键(类似unordered_multimap)
 *    - O(1)平均查找、插入、删除性能
 * 
 * 2. **共享内存优化**：
 *    - 固定大小内存布局，避免动态分配
 *    - 基于索引的链表实现，支持进程间共享
 *    - 内存对齐优化，提高访问效率
 *    - 支持CREATE/RESUME模式初始化
 * 
 * 3. **STL高度兼容**：
 *    - 完整的前向迭代器支持
 *    - 标准的容器操作（insert、find、erase、clear等）
 *    - 哈希表特有操作（bucket相关接口）
 *    - 自定义哈希函数和比较函数支持
 * 
 * 4. **扩展功能**：
 *    - 节点池管理和回收
 *    - 容量检查功能（full()、left_size()等）
 *    - 详细的调试和诊断接口
 *    - 完整的哈希表操作集合
 * 
 * @section stl_comparison STL容器对比
 * 
 * | 特性 | STL unordered_map | NFShmHashTable |
 * |------|-------------------|----------------|
 * | **数据结构** | 哈希表(链地址法) | 哈希表(链地址法) |
 * | **容量管理** | 动态扩容，无限制 | 固定容量MAX_SIZE，编译时确定 |
 * | **内存管理** | 堆内存，动态分配 | 共享内存，预分配节点池 |
 * | **桶管理** | 动态调整，自动rehash | 固定桶数量，无rehash |
 * | **插入删除** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 |
 * | **查找访问** | O(1)平均 | O(1)平均 |
 * | **迭代器类型** | 前向迭代器 | 前向迭代器 |
 * | **内存布局** | 分散分配 | 连续节点池 |
 * | **进程共享** | 不支持 | **原生支持** |
 * | **异常安全** | 强异常安全保证 | 无异常，错误码返回 |
 * | **内存碎片** | 可能产生 | **无碎片**（固定节点池） |
 * | **负载因子** | 自动维护 | 固定结构，无动态调整 |
 * 
 * @section api_compatibility API兼容性
 * 
 * **完全兼容的接口**：
 * - size(), empty(), max_size()
 * - begin(), end()
 * - insert(), insert_unique(), insert_equal()
 * - find(), count(), equal_range()
 * - erase(), clear()
 * - bucket_count(), max_bucket_count()
 * - swap()
 * 
 * **扩展的接口（新增）**：
 * - full(), left_size() - 容量检查
 * - CreateInit(), ResumeInit() - 共享内存初始化
 * - GetValidNode(), get_iterator() - 节点访问接口
 * - print_structure(), print_detailed() - 调试接口
 * - insert_unique_noresize(), insert_equal_noresize() - 无扩容插入
 * 
 * **行为差异的接口**：
 * - max_size()：返回MAX_SIZE而非理论最大值
 * - bucket_count()：返回固定值MAX_SIZE
 * - 插入操作：节点池满时失败而非自动扩容
 * - 不支持：rehash(), reserve(), load_factor()等动态调整接口
 * 
 * @section usage_examples 使用示例
 * 
 * @subsection basic_usage 基础用法（类似std::unordered_map）
 * 
 * ```cpp
 * // 定义容量为1000的字符串到整数映射
 * NFShmHashTable<std::pair<std::string, int>, std::string, 1000, 
 *                std::hash<std::string>, select1st<std::pair<std::string, int>>, 
 *                std::equal_to<std::string>> table;
 * table.CreateInit();  // 创建模式初始化
 * 
 * // STL兼容的基础操作
 * table.insert_unique(std::make_pair("apple", 100));
 * table.insert_unique(std::make_pair("banana", 200));
 * table.insert_unique(std::make_pair("orange", 300));
 * 
 * // 查找操作
 * auto it = table.find("apple");
 * if (it != table.end()) {
 *     std::cout << "Found: " << it->first << " = " << it->second << std::endl;
 * }
 * 
 * // 统计和范围查找
 * std::cout << "Count of 'apple': " << table.count("apple") << std::endl;
 * auto range = table.equal_range("banana");
 * 
 * // 迭代器遍历
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     std::cout << it->first << " = " << it->second << std::endl;
 * }
 * 
 * // 范围for循环（C++11）
 * for (const auto& pair : table) {
 *     std::cout << pair.first << " = " << pair.second << std::endl;
 * }
 * ```
 * 
 * @subsection unique_vs_equal 唯一插入 vs 重复插入
 * 
 * ```cpp
 * NFShmHashTable<int, int, 100, std::hash<int>, identity<int>, std::equal_to<int>> table;
 * 
 * // 唯一插入（类似unordered_set）
 * auto result1 = table.insert_unique(42);
 * if (result1.second) {
 *     std::cout << "插入成功" << std::endl;
 * } else {
 *     std::cout << "键已存在" << std::endl;
 * }
 * 
 * auto result2 = table.insert_unique(42);  // 失败，键已存在
 * assert(result2.second == false);
 * 
 * // 重复插入（类似unordered_multiset）
 * auto it1 = table.insert_equal(42);  // 成功，允许重复
 * auto it2 = table.insert_equal(42);  // 也成功
 * 
 * std::cout << "Count of 42: " << table.count(42) << std::endl;  // 输出：3
 * ```
 * 
 * @subsection capacity_management 容量管理
 * 
 * ```cpp
 * NFShmHashTable<int, int, 100, std::hash<int>, identity<int>, std::equal_to<int>> table;
 * 
 * // 容量检查
 * std::cout << "Max size: " << table.max_size() << std::endl;      // 100
 * std::cout << "Current size: " << table.size() << std::endl;      // 0
 * std::cout << "Left size: " << table.left_size() << std::endl;    // 100
 * 
 * // 安全插入模式
 * int value = 0;
 * while (!table.full()) {
 *     table.insert_unique(value++);
 * }
 * 
 * if (table.full()) {
 *     std::cout << "哈希表已满，无法继续添加元素" << std::endl;
 * }
 * 
 * // 桶信息
 * std::cout << "Bucket count: " << table.bucket_count() << std::endl;
 * for (size_t i = 0; i < table.bucket_count(); ++i) {
 *     size_t count = table.elems_in_bucket(i);
 *     if (count > 0) {
 *         std::cout << "Bucket " << i << " has " << count << " elements" << std::endl;
 *     }
 * }
 * ```
 * 
 * @subsection custom_hash_func 自定义哈希函数
 * 
 * ```cpp
 * // 自定义数据结构
 * struct Person {
 *     std::string name;
 *     int age;
 *     
 *     Person(const std::string& n, int a) : name(n), age(a) {}
 * };
 * 
 * // 自定义哈希函数
 * struct PersonHash {
 *     size_t operator()(const Person& p) const {
 *         return std::hash<std::string>()(p.name) ^ (std::hash<int>()(p.age) << 1);
 *     }
 * };
 * 
 * // 自定义比较函数
 * struct PersonEqual {
 *     bool operator()(const Person& lhs, const Person& rhs) const {
 *         return lhs.name == rhs.name && lhs.age == rhs.age;
 *     }
 * };
 * 
 * // 使用自定义哈希表
 * NFShmHashTable<Person, Person, 1000, PersonHash, identity<Person>, PersonEqual> personTable;
 * personTable.CreateInit();
 * 
 * personTable.insert_unique(Person("Alice", 30));
 * personTable.insert_unique(Person("Bob", 25));
 * 
 * auto it = personTable.find(Person("Alice", 30));
 * if (it != personTable.end()) {
 *     std::cout << "Found: " << it->name << ", age " << it->age << std::endl;
 * }
 * ```
 * 
 * @section performance_characteristics 性能特征
 * 
 * | 操作 | STL unordered_map | NFShmHashTable |
 * |------|-------------------|----------------|
 * | **查找 find** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 |
 * | **插入 insert** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 |
 * | **删除 erase** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 |
 * | **遍历 iteration** | O(n) | O(n) |
 * | **rehash** | O(n) | **不支持**（固定结构） |
 * | **内存分配** | 动态分配 | 预分配固定内存 |
 * | **缓存友好性** | 中等（桶分散） | **较好**（连续节点池） |
 * | **内存开销** | 动态 | 固定MAX_SIZE * sizeof(Node) |
 * | **负载因子** | 自动维护 | 需要预估合理MAX_SIZE |
 * 
 * @section memory_layout 内存布局
 * 
 * ```
 * NFShmHashTable 内存结构：
 * ┌─────────────────┐
 * │   管理数据       │ <- 基类信息，元素数量等
 * ├─────────────────┤
 * │   桶索引数组     │ <- m_bucketsFirstIdx[MAX_SIZE]
 * │   [0] -> node_x │    ├─ 桶0的首节点索引
 * │   [1] -> node_y │    ├─ 桶1的首节点索引
 * │   ...           │    ├─ ...
 * │   [MAX-1] -> -1 │    └─ 桶MAX_SIZE-1（空桶）
 * ├─────────────────┤
 * │   节点池         │ <- m_mem[MAX_SIZE] (AlignedStorage)
 * │   [0] 节点0      │    ├─ value + next索引 + valid标志
 * │   [1] 节点1      │    ├─ ...
 * │   ...           │    ├─ ...
 * │   [MAX-1] 节点   │    └─ 最后一个节点
 * └─────────────────┘
 * 
 * 节点结构：
 * ┌─────────────────┐
 * │   m_value       │ <- 存储的键值对或值
 * │   m_next        │ <- 链表中下一个节点索引(-1表示末尾)
 * │   m_valid       │ <- 节点是否有效
 * │   m_self        │ <- 自身索引（调试用）
 * └─────────────────┘
 * 
 * 哈希分布示例：
 * 桶0: node5 -> node12 -> node89 -> -1
 * 桶1: node3 -> -1
 * 桶2: -1 (空桶)
 * 桶3: node7 -> node24 -> -1
 * ...
 * 
 * 索引引用优势：
 * - 进程间地址无关，支持共享内存
 * - 固定大小，内存布局可预测
 * - 避免悬空指针问题
 * - 链表操作基于索引，安全高效
 * ```
 * 
 * @section thread_safety 线程安全
 * 
 * - **非线程安全**：需要外部同步机制
 * - **共享内存兼容**：多进程可安全访问（需进程间锁）
 * - **无内部锁**：避免性能开销，由用户控制并发
 * - **迭代器稳定性**：插入删除可能影响其他迭代器
 * 
 * @section migration_guide 从STL迁移指南
 * 
 * 1. **包含头文件**：
 *    ```cpp
 *    // 替换
 *    #include <unordered_map>
 *    // 为
 *    #include "NFComm/NFShmStl/NFShmHashTable.h"
 *    ```
 * 
 * 2. **类型定义**：
 *    ```cpp
 *    // STL
 *    std::unordered_map<std::string, int> map;
 *    
 *    // NFShmHashTable（需要完整模板参数）
 *    NFShmHashTable<std::pair<std::string, int>, std::string, 1000,
 *                   std::hash<std::string>, 
 *                   select1st<std::pair<std::string, int>>,
 *                   std::equal_to<std::string>> map;
 *    ```
 * 
 * 3. **初始化**：
 *    ```cpp
     *    // 添加初始化调用
     *    map.CreateInit();  // 或 ResumeInit() 用于共享内存恢复
     *    ```
 * 
 * 4. **插入操作**：
 *    ```cpp
     *    // STL方式
     *    map["key"] = value;
     *    map.insert({key, value});
     *    
     *    // NFShmHashTable方式
     *    map.insert_unique(std::make_pair(key, value));
     *    // 或使用find_or_insert
     *    map.find_or_insert(std::make_pair(key, value)) = value;
     *    ```
 * 
 * 5. **容量管理**：
 *    ```cpp
     *    // 添加容量检查
     *    if (map.full()) {
     *        // 处理容量已满的情况
     *    }
     *    
     *    // 移除无限容量假设
     *    while (!map.full()) {
     *        map.insert_unique(getData());  // 安全插入
     *    }
     *    ```
 * 
 * @section best_practices 最佳实践
 * 
 * 1. **容量规划**：根据预期数据量选择合适的MAX_SIZE，考虑负载因子
 * 2. **哈希函数选择**：
 *    - 使用高质量的哈希函数，确保均匀分布
 *    - 避免哈希冲突过多，影响性能
 *    - 对于字符串，考虑使用FNV、MurmurHash等
 * 3. **初始化模式**：
 *    - 新创建时使用CreateInit()
 *    - 恢复现有数据时使用ResumeInit()
 * 4. **迭代器使用**：
 *    - 插入删除可能使迭代器失效，及时更新
 *    - 遍历时避免修改容器结构
 * 5. **性能优化**：
 *    - 合理设置容量，避免过高的负载因子
 *    - 使用insert_*_noresize版本避免不必要的检查
 *    - 批量操作时考虑预留空间
 * 
 * @warning 注意事项
 * - 固定容量限制，超出MAX_SIZE的操作会失败
 * - 不支持动态rehash，需要合理预估容量
 * - 哈希函数质量直接影响性能，避免频繁冲突
 * - 共享内存环境下需要考虑进程崩溃的数据恢复
 * - 非线程安全，多线程访问需要外部同步
 * 
 * @see NFShmVector - 基于共享内存的动态数组实现
 * @see NFShmList - 基于共享内存的双向链表实现
 * @see std::unordered_map - 标准无序关联容器
 * @see std::unordered_set - 标准无序集合容器
 * 
 * @author gaoyi
 * @date 2023-02-06
 */

#pragma once

#include "NFShmStl.h"
#include <iterator>
#include <algorithm>

// ==================== 前向声明 ====================

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
class NFShmHashTable;

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableIterator;

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableConstIterator;

// ==================== 节点定义 ====================

/**
 * @brief Hash表节点结构
 * @tparam Val 存储的值类型
 * 
 * 与STL不同，使用索引而非指针构建链表，适合共享内存环境。
 * STL通常使用指针链表，而这里使用索引链表提高共享内存兼容性。
 */
template <class Val>
struct NFShmHashTableNode
{
    /**
     * @brief 构造函数，根据创建模式选择初始化方式
     * 
     * 与STL节点不同，支持共享内存的创建/恢复模式：
     * - 创建模式：初始化所有成员
     * - 恢复模式：从共享内存恢复，保持现有状态
     */
    NFShmHashTableNode()
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

    /// @brief 创建模式初始化
    int CreateInit()
    {
        m_valid = false;
        m_next = -1;
        m_self = 0;
        return 0;
    }

    /// @brief 恢复模式初始化
    int ResumeInit()
    {
        return 0;
    }

    int m_next; ///< 下一个节点的索引（-1表示链表末尾）
    Val m_value; ///< 存储的值
    bool m_valid; ///< 节点是否有效
    size_t m_self; ///< 节点自身索引（用于调试和验证）
};

// ==================== 迭代器实现 ====================

/**
 * @brief Hash表非常量迭代器
 * @tparam Val 值类型
 * @tparam Key 键类型  
 * @tparam MAX_SIZE 最大容量
 * @tparam HashFcn 哈希函数类型
 * @tparam ExtractKey 键提取函数类型
 * @tparam EqualKey 键比较函数类型
 * 
 * STL兼容性说明：
 * - 实现std::forward_iterator_tag，与STL unordered_map迭代器兼容
 * - 支持operator++, operator*, operator->等标准操作
 * - 与STL不同：基于索引实现，需要验证节点有效性
 */
template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableIterator
{
    // ==================== STL兼容类型定义 ====================
    typedef NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> Hashtable;
    typedef NFShmHashTableIterator iterator;
    typedef NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> const_iterator;
    typedef NFShmHashTableNode<Val> Node;

    /// @brief STL标准迭代器类型定义
    typedef std::forward_iterator_tag iterator_category; ///< 前向迭代器
    typedef Val value_type; ///< 值类型
    typedef ptrdiff_t difference_type; ///< 差值类型
    typedef size_t size_type; ///< 大小类型
    typedef Val& reference; ///< 引用类型
    typedef Val* pointer; ///< 指针类型

    // ==================== 成员变量 ====================
    Node* m_curNode; ///< 当前节点指针
    Hashtable* m_hashTable; ///< 所属哈希表指针
    static value_type m_staticError; ///< 错误时返回的静态对象

    // ==================== 构造函数 ====================

    /**
     * @brief 构造迭代器
     * @param n 节点指针
     * @param tab 哈希表指针
     */
    NFShmHashTableIterator(Node* n, Hashtable* tab)
        : m_curNode(n), m_hashTable(tab)
    {
    }

    /// @brief 默认构造函数，创建无效迭代器
    NFShmHashTableIterator(): m_curNode(nullptr), m_hashTable(nullptr)
    {
    }

    // ==================== STL兼容操作符 ====================

    /**
     * @brief 解引用操作符
     * @return 当前节点的值引用
     * @note 与STL行为一致，提供安全检查
     */
    reference operator*() const
    {
        CHECK_EXPR(m_curNode != nullptr, m_staticError, "Iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, m_staticError, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, m_staticError, "Iterator points to invalid node, TRACE_STACK:%s", TRACE_STACK());
        return m_curNode->m_value;
    }

    /**
     * @brief 成员访问操作符
     * @return 当前节点值的指针
     * @note 与STL行为一致
     */
    pointer operator->() const
    {
        CHECK_EXPR(m_curNode != nullptr, &m_staticError, "Iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, &m_staticError, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, &m_staticError, "Iterator points to invalid node, TRACE_STACK:%s", TRACE_STACK());
        return &(m_curNode->m_value);
    }

    /// @brief 前置递增操作符，移动到下一个有效元素
    iterator& operator++();

    /// @brief 后置递增操作符
    iterator operator++(int);

    /// @brief 相等比较操作符
    bool operator==(const iterator& it) const { return m_curNode == it.m_curNode; }

    /// @brief 不等比较操作符
    bool operator!=(const iterator& it) const { return m_curNode != it.m_curNode; }
};

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
Val NFShmHashTableIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::m_staticError = Val();

/**
 * @brief Hash表常量迭代器
 * @note 与非常量迭代器类似，但提供const访问
 * @note 与STL const_iterator兼容
 */
template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableConstIterator
{
    // ==================== STL兼容类型定义 ====================
    typedef NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> Hashtable;
    typedef NFShmHashTableIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> iterator;
    typedef NFShmHashTableConstIterator const_iterator;
    typedef NFShmHashTableNode<Val> Node;

    typedef std::forward_iterator_tag iterator_category;
    typedef Val value_type;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef const Val& reference; ///< 常量引用
    typedef const Val* pointer; ///< 常量指针

    // ==================== 成员变量 ====================
    const Node* m_curNode; ///< 当前节点常量指针
    const Hashtable* m_hashTable; ///< 所属哈希表常量指针
    static value_type m_staticError;

    // ==================== 构造函数 ====================

    NFShmHashTableConstIterator(const Node* n, const Hashtable* tab)
        : m_curNode(n), m_hashTable(tab)
    {
    }

    NFShmHashTableConstIterator(): m_curNode(nullptr), m_hashTable(nullptr)
    {
    }

    /**
     * @brief 从非常量迭代器构造
     * @param it 非常量迭代器
     * @note STL标准要求支持此转换
     */
    NFShmHashTableConstIterator(const iterator& it)
        : m_curNode(it.m_curNode), m_hashTable(it.m_hashTable)
    {
    }

    // ==================== STL兼容操作符 ====================

    reference operator*() const
    {
        CHECK_EXPR(m_curNode != nullptr, m_staticError, "Const iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, m_staticError, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, m_staticError, "Const iterator points to invalid node, TRACE_STACK:%s", TRACE_STACK());
        return m_curNode->m_value;
    }

    pointer operator->() const
    {
        CHECK_EXPR(m_curNode != nullptr, &m_staticError, "Const iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, &m_staticError, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, &m_staticError, "Const iterator points to invalid node, TRACE_STACK:%s", TRACE_STACK());
        return &(m_curNode->m_value);
    }

    const_iterator& operator++();
    const_iterator operator++(int);

    bool operator==(const const_iterator& it) const { return m_curNode == it.m_curNode; }
    bool operator!=(const const_iterator& it) const { return m_curNode != it.m_curNode; }
};

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
Val NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::m_staticError = Val();

// ==================== 主要Hash表类 ====================

/**
 * @brief 基于共享内存的Hash表实现
 * @tparam Val 值类型
 * @tparam Key 键类型
 * @tparam MAX_SIZE 最大容量（固定）
 * @tparam HashFcn 哈希函数类型
 * @tparam ExtractKey 从值中提取键的函数类型
 * @tparam EqualKey 键比较函数类型
 * 
 * 设计特点：
 * 1. 固定容量，不支持动态扩容（与STL主要区别）
 * 2. 基于共享内存，支持进程间共享
 * 3. 使用索引链表而非指针链表
 * 4. API设计尽量兼容STL unordered_map/unordered_set
 * 
 * 与STL unordered_map的主要差异：
 * - 容量限制：固定大小 vs 动态扩容
 * - 内存管理：共享内存 vs 堆内存
 * - 性能特征：无rehash开销 vs 动态性能优化
 */
template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
class NFShmHashTable
{
public:
    // ==================== STL兼容类型定义 ====================

    typedef Key key_type; ///< 键类型
    typedef Val value_type; ///< 值类型
    typedef HashFcn hasher; ///< 哈希函数类型
    typedef EqualKey key_equal; ///< 键相等比较函数类型

    typedef size_t size_type; ///< 大小类型
    typedef ptrdiff_t difference_type; ///< 差值类型
    typedef value_type* pointer; ///< 指针类型
    typedef const value_type* const_pointer; ///< 常量指针类型
    typedef value_type& reference; ///< 引用类型
    typedef const value_type& const_reference; ///< 常量引用类型

    typedef NFShmHashTableNode<Val> Node; ///< 节点类型

private:
    // ==================== 内存管理 ====================

    /// @brief 对齐存储，确保节点内存布局正确
    typedef typename std::aligned_storage<sizeof(Node), alignof(Node)>::type AlignedStorage;

    AlignedStorage m_buckets[MAX_SIZE]; ///< 节点存储区域
    int m_bucketsFirstIdx[MAX_SIZE]; ///< 每个桶的首节点索引
    int m_firstFreeIdx; ///< 空闲链表头节点索引
    size_type m_size; ///< 当前元素数量
    int8_t m_init; ///< 初始化状态
    hasher m_hash; ///< 哈希函数对象
    key_equal m_equals; ///< 键比较函数对象
    ExtractKey m_getKey; ///< 键提取函数对象
    static value_type m_staticError; ///< 错误返回值

private:
    /// @brief 获取桶数组指针
    Node* GetBuckets() { return reinterpret_cast<Node*>(m_buckets); }
    /// @brief 获取桶数组常量指针
    const Node* GetBuckets() const { return reinterpret_cast<const Node*>(m_buckets); }

private:
    /**
     * @brief 创建新节点
     * @return 新节点指针，失败返回nullptr
     * @note 从空闲链表中获取节点，与STL的allocator分配不同
     */
    Node* CreateNode();

    /**
     * @brief 回收节点到空闲链表
     * @param p 要回收的节点指针
     * @note 不释放内存，而是标记为无效并加入空闲链表
     */
    void RecycleNode(Node* p);

public:
    // ==================== 迭代器类型定义 ====================

    typedef NFShmHashTableIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> iterator;
    typedef NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> const_iterator;

    friend struct NFShmHashTableIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>;
    friend struct NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>;

public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 构造函数
     * @note 根据SHM_CREATE_MODE决定创建或恢复模式
     */
    NFShmHashTable();

    /**
     * @brief 拷贝构造函数
     * @param ht 源哈希表
     */
    NFShmHashTable(const NFShmHashTable& ht);

    /**
     * @brief 赋值操作符
     * @param ht 源哈希表
     * @return 自身引用
     */
    NFShmHashTable& operator=(const NFShmHashTable& ht);

    /**
     * @brief 析构函数
     */
    virtual ~NFShmHashTable();

    // ==================== 共享内存特有接口 ====================

    /**
     * @brief 创建模式初始化（共享内存特有）
     * @return 0表示成功，-1表示失败
     * @note 与STL对比：
     *       - STL: 无对应概念，构造函数即完成初始化
     *       - NFShmHashTable: 支持共享内存CREATE/RESUME两阶段初始化
     *       - 用于共享内存首次创建时的初始化
     *       - 初始化桶数组、空闲链表等数据结构
     *       - 所有节点标记为无效状态，建立空闲链表
     *       - 在SHM_CREATE_MODE下自动调用
     * 
     * 使用示例：
     * ```cpp
     * NFShmHashTable<int, int, 100, std::hash<int>, identity<int>, std::equal_to<int>> table;
     * if (table.CreateInit() == 0) {
     *     // 创建成功，可以开始使用
     *     table.insert_unique(42);
     * }
     * ```
     */
    int CreateInit();

    /**
     * @brief 恢复模式初始化（共享内存特有）
     * @return 0表示成功，-1表示失败
     * @note 与STL对比：
     *       - STL: 无对应概念
     *       - NFShmHashTable: 用于从已存在的共享内存恢复容器状态
     *       - 不清空现有数据，恢复哈希表结构
     *       - 对非平凡构造类型执行placement构造
     *       - 进程重启后恢复共享内存数据使用
     *       - 验证数据完整性和状态一致性
     * 
     * 使用示例：
     * ```cpp
     * // 进程重启后恢复哈希表
     * NFShmHashTable<int, int, 100, std::hash<int>, identity<int>, std::equal_to<int>> table;
     * if (table.ResumeInit() == 0) {
     *     // 恢复成功，数据已可用
     *     std::cout << "Recovered " << table.size() << " elements" << std::endl;
     * }
     * ```
     */
    int ResumeInit();

    /**
     * @brief 就地初始化
     * @note 使用placement new重新构造对象
     */
    void Init();

    // ==================== 容量相关接口（STL兼容） ====================

    /**
     * @brief 获取当前元素数量
     * @return 元素数量
     * @note 与STL unordered_map::size()兼容
     */
    size_type size() const;

    /**
     * @brief 获取最大容量
     * @return 最大容量MAX_SIZE
     * @note 与STL不同，返回固定值而非理论最大值
     */
    size_type max_size() const;

    /**
     * @brief 判断是否为空
     * @return true表示空
     * @note 与STL unordered_map::empty()兼容
     */
    bool empty() const;

    /**
     * @brief 判断是否已满
     * @return true表示已满
     * @note 与STL对比：
     *       - STL: 无对应接口，unordered_map可无限扩容
     *       - NFShmHashTable: 特有功能，体现固定容量限制
     *       - 用于在插入前检查容量，避免操作失败
     *       - 等价于 size() >= max_size()
     *       - 与empty()形成对比，提供完整的容量状态检查
     * 
     * 使用示例：
     * ```cpp
     * NFShmHashTable<int, int, 100, std::hash<int>, identity<int>, std::equal_to<int>> table;
     * 
     * // 安全插入模式
     * int value = 0;
     * while (!table.full() && hasMoreData()) {
     *     table.insert_unique(value++);
     * }
     * 
     * // 容量检查
     * if (table.full()) {
     *     std::cout << "哈希表已满，无法继续添加元素" << std::endl;
     * } else {
     *     std::cout << "还可以添加 " << table.left_size() 
     *               << " 个元素" << std::endl;
     * }
     * ```
     */
    bool full() const;

    /**
     * @brief 获取剩余容量
     * @return 剩余可用空间
     * @note 与STL对比：
     *       - STL: 无对应接口，容量可动态调整
     *       - NFShmHashTable: 特有功能，显示剩余可用节点数
     *       - 等价于 max_size() - size()
     *       - 用于监控内存使用情况和规划插入操作
     *       - 帮助判断是否有足够空间进行批量操作
     * 
     * 使用示例：
     * ```cpp
     * // 批量插入前检查容量
     * std::vector<int> dataToInsert = getDataBatch();
     * if (table.left_size() >= dataToInsert.size()) {
     *     for (const auto& item : dataToInsert) {
     *         table.insert_unique(item);
     *     }
     * } else {
     *     std::cout << "空间不足，需要 " << dataToInsert.size() 
     *               << " 但只剩 " << table.left_size() << std::endl;
     * }
     * ```
     */
    size_t left_size() const;

    // ==================== 节点访问接口 ====================

    /**
     * @brief 根据索引获取有效节点
     * @param idx 节点索引
     * @return 节点指针，无效返回nullptr
     */
    Node* GetValidNode(int idx);

    /**
     * @brief 根据索引获取有效节点（常量版本）
     * @param idx 节点索引
     * @return 常量节点指针，无效返回nullptr
     */
    const Node* GetValidNode(int idx) const;

    /**
     * @brief 根据索引创建迭代器
     * @param idx 节点索引
     * @return 迭代器
     */
    iterator get_iterator(int idx);

    /**
     * @brief 根据索引创建常量迭代器
     * @param idx 节点索引  
     * @return 常量迭代器
     */
    const_iterator get_iterator(int idx) const;

    // ==================== STL兼容交换接口 ====================

    /**
     * @brief 交换两个哈希表的内容
     * @param other 另一个哈希表
     * @note 与STL unordered_map::swap()兼容
     */
    void swap(NFShmHashTable& other) noexcept;

    // ==================== STL兼容迭代器接口 ====================

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     * @note 与STL unordered_map::begin()兼容
     */
    iterator begin();

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     * @note 与STL unordered_map::end()兼容
     */
    iterator end();

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与STL unordered_map::begin() const兼容
     */
    const_iterator begin() const;

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与STL unordered_map::end() const兼容
     */
    const_iterator end() const;

    // ==================== 友元比较操作符 ====================

    template <class Vl, class Ky, int MaxSize, class HF, class Ex, class Eq>
    friend bool operator==(const NFShmHashTable<Vl, Ky, MaxSize, HF, Ex, Eq>&,
                           const NFShmHashTable<Vl, Ky, MaxSize, HF, Ex, Eq>&);

public:
    // ==================== 桶接口（STL兼容） ====================

    /**
     * @brief 获取桶数量
     * @return 桶数量（固定为MAX_SIZE）
     * @note 与STL unordered_map::bucket_count()兼容，但返回固定值
     */
    size_type bucket_count() const;

    /**
     * @brief 获取最大桶数量
     * @return 最大桶数量（固定为MAX_SIZE）
     * @note 与STL unordered_map::max_bucket_count()兼容
     */
    size_type max_bucket_count() const;

    /**
     * @brief 获取指定桶中的元素数量
     * @param bucket 桶索引
     * @return 该桶中的元素数量
     * @note 与STL unordered_map::bucket_size()类似
     */
    size_type elems_in_bucket(size_type bucket) const;

    // ==================== 插入接口（STL兼容） ====================

    /**
     * @brief 插入唯一元素
     * @param obj 要插入的值
     * @return pair<iterator, bool>，迭代器指向元素，bool表示是否插入成功
     * @note 与STL对比：
     *       - STL: std::unordered_map::insert() 行为完全一致
     *       - NFShmHashTable: 相同键的元素不会重复插入
     *       - 如果键已存在，返回指向现有元素的迭代器，bool为false
     *       - 如果键不存在且容量足够，插入新元素，bool为true
     *       - 容量满时插入失败，而STL会自动扩容
     *       - 时间复杂度：O(1)平均，O(n)最坏（链表长度）
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法
     * std::unordered_map<int, std::string> stdMap;
     * auto result1 = stdMap.insert({42, "value"});
     * if (result1.second) {
     *     std::cout << "插入成功" << std::endl;
     * }
     * 
     * // NFShmHashTable用法
     * NFShmHashTable<std::pair<int, std::string>, int, 100, 
     *                std::hash<int>, select1st<std::pair<int, std::string>>, 
     *                std::equal_to<int>> table;
     * auto result2 = table.insert_unique(std::make_pair(42, "value"));
     * if (result2.second) {
     *     std::cout << "插入成功" << std::endl;
     * } else {
     *     std::cout << "键已存在或容量已满" << std::endl;
     * }
     * ```
     */
    std::pair<iterator, bool> insert_unique(const value_type& obj);

    /**
     * @brief 插入允许重复的元素
     * @param obj 要插入的值
     * @return 指向插入元素的迭代器
     * @note 与STL对比：
     *       - STL: std::unordered_multimap::insert() 行为类似
     *       - NFShmHashTable: 即使相同键也允许插入多个元素
     *       - 总是尝试插入，除非容量已满
     *       - 返回指向新插入元素的迭代器，失败时返回end()
     *       - 容量满时插入失败，而STL会自动扩容
     *       - 适用于需要存储多个相同键的场景
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法
     * std::unordered_multimap<int, std::string> stdMultiMap;
     * auto it1 = stdMultiMap.insert({42, "first"});
     * auto it2 = stdMultiMap.insert({42, "second"});  // 成功，允许重复
     * 
     * // NFShmHashTable用法
     * NFShmHashTable<std::pair<int, std::string>, int, 100, 
     *                std::hash<int>, select1st<std::pair<int, std::string>>, 
     *                std::equal_to<int>> table;
     * auto it3 = table.insert_equal(std::make_pair(42, "first"));
     * auto it4 = table.insert_equal(std::make_pair(42, "second"));  // 也成功
     * std::cout << "Count of 42: " << table.count(42) << std::endl;  // 输出：2
     * ```
     */
    iterator insert_equal(const value_type& obj);

    /**
     * @brief 插入唯一元素（无扩容版本）
     * @param obj 要插入的值
     * @return pair<iterator, bool>
     * @note 内部使用，不触发扩容检查
     */
    std::pair<iterator, bool> insert_unique_noresize(const value_type& obj);

    /**
     * @brief 插入允许重复元素（无扩容版本）
     * @param obj 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal_noresize(const value_type& obj);

    // ==================== 范围插入接口 ====================

    template <class InputIterator>
    void insert_unique(InputIterator f, InputIterator l);

    template <class InputIterator>
    void insert_equal(InputIterator f, InputIterator l);

    template <class InputIterator>
    void insert_unique(InputIterator f, InputIterator l, std::input_iterator_tag);

    template <class InputIterator>
    void insert_equal(InputIterator f, InputIterator l, std::input_iterator_tag);

    template <class ForwardIterator>
    void insert_unique(ForwardIterator f, ForwardIterator l, std::forward_iterator_tag);

    template <class ForwardIterator>
    void insert_equal(ForwardIterator f, ForwardIterator l, std::forward_iterator_tag);

    void insert_unique(const value_type* f, const value_type* l);
    void insert_equal(const value_type* f, const value_type* l);
    void insert_unique(const_iterator f, const_iterator l);
    void insert_equal(const_iterator f, const_iterator l);

    // ==================== 查找接口（STL兼容） ====================

    /**
     * @brief 查找或插入元素
     * @param obj 要查找或插入的值
     * @return 元素的引用
     * @note 类似STL unordered_map::operator[]的行为
     */
    reference find_or_insert(const value_type& obj);

    /**
     * @brief 查找元素
     * @param key 要查找的键
     * @return 指向元素的迭代器，未找到返回end()
     * @note 与STL对比：
     *       - STL: std::unordered_map::find() 行为完全一致
     *       - NFShmHashTable: 返回指向第一个匹配元素的迭代器
     *       - 时间复杂度：O(1)平均，O(n)最坏（链表长度）
     *       - 未找到时返回end()，而非抛出异常
     *       - 支持自定义哈希函数和比较函数
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法
     * std::unordered_map<std::string, int> stdMap;
     * stdMap["apple"] = 100;
     * auto it1 = stdMap.find("apple");
     * if (it1 != stdMap.end()) {
     *     std::cout << "Found: " << it1->second << std::endl;
     * }
     * 
     * // NFShmHashTable用法
     * NFShmHashTable<std::pair<std::string, int>, std::string, 100, 
     *                std::hash<std::string>, select1st<std::pair<std::string, int>>, 
     *                std::equal_to<std::string>> table;
     * table.insert_unique(std::make_pair("apple", 100));
     * auto it2 = table.find("apple");
     * if (it2 != table.end()) {
     *     std::cout << "Found: " << it2->second << std::endl;
     * }
     * ```
     */
    iterator find(const key_type& key);

    /**
     * @brief 查找元素（常量版本）
     * @param key 要查找的键
     * @return 指向元素的常量迭代器
     * @note 与STL unordered_map::find() const兼容
     */
    const_iterator find(const key_type& key) const;

    /**
     * @brief 统计指定键的元素数量
     * @param key 要统计的键
     * @return 元素数量
     * @note 与STL对比：
     *       - STL: std::unordered_map::count() 完全兼容
     *       - NFShmHashTable: 返回具有指定键的元素数量
     *       - 对于insert_unique插入的元素，结果为0或1
     *       - 对于insert_equal插入的元素，可能大于1
     *       - 时间复杂度：O(1)平均，O(n)最坏
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法
     * std::unordered_multimap<int, std::string> stdMultiMap;
     * stdMultiMap.insert({42, "first"});
     * stdMultiMap.insert({42, "second"});
     * std::cout << "Count: " << stdMultiMap.count(42) << std::endl;  // 输出：2
     * 
     * // NFShmHashTable用法
     * NFShmHashTable<std::pair<int, std::string>, int, 100, 
     *                std::hash<int>, select1st<std::pair<int, std::string>>, 
     *                std::equal_to<int>> table;
     * table.insert_equal(std::make_pair(42, "first"));
     * table.insert_equal(std::make_pair(42, "second"));
     * std::cout << "Count: " << table.count(42) << std::endl;  // 输出：2
     * ```
     */
    size_type count(const key_type& key) const;

    /**
     * @brief 获取指定键的元素范围
     * @param key 要查找的键
     * @return pair<iterator, iterator>表示范围
     * @note 与STL unordered_map::equal_range()兼容
     */
    std::pair<iterator, iterator> equal_range(const key_type& key);

    /**
     * @brief 获取指定键的元素范围（常量版本）
     * @param key 要查找的键
     * @return pair<const_iterator, const_iterator>表示范围
     */
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const;

    value_type& at(const key_type&  key);
    const value_type& at(const key_type&  key) const;

    // ==================== 删除接口（STL兼容） ====================

    /**
     * @brief 根据键删除元素
     * @param key 要删除的键
     * @return 删除的元素数量
     * @note 与STL对比：
     *       - STL: std::unordered_map::erase() 行为完全一致
     *       - NFShmHashTable: 删除所有具有指定键的元素
     *       - 返回实际删除的元素数量
     *       - 时间复杂度：O(1)平均，O(n)最坏
     *       - 删除后节点回收到空闲链表，可重复使用
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法
     * std::unordered_map<int, std::string> stdMap;
     * stdMap[42] = "value";
     * size_t count1 = stdMap.erase(42);  // 返回1
     * 
     * // NFShmHashTable用法
     * NFShmHashTable<std::pair<int, std::string>, int, 100, 
     *                std::hash<int>, select1st<std::pair<int, std::string>>, 
     *                std::equal_to<int>> table;
     * table.insert_unique(std::make_pair(42, "value"));
     * size_t count2 = table.erase(42);  // 返回1
     * ```
     */
    size_type erase(const key_type& key);

    /**
     * @brief 根据迭代器删除元素
     * @param it 指向要删除元素的迭代器
     * @return 指向下一个元素的迭代器
     * @note 与STL对比：
     *       - STL: std::unordered_map::erase() 行为完全一致
     *       - NFShmHashTable: 删除迭代器指向的单个元素
     *       - 返回指向下一个有效元素的迭代器
     *       - 删除后原迭代器失效，但返回的迭代器有效
     *       - 适用于在遍历过程中删除元素
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法
     * std::unordered_map<int, std::string> stdMap;
     * auto it1 = stdMap.find(42);
     * if (it1 != stdMap.end()) {
     *     auto next = stdMap.erase(it1);  // 返回下一个迭代器
     * }
     * 
     * // NFShmHashTable用法
     * auto it2 = table.find(42);
     * if (it2 != table.end()) {
     *     auto next = table.erase(it2);  // 相同行为
     * }
     * ```
     */
    iterator erase(iterator it);

    /**
     * @brief 根据常量迭代器删除元素
     * @param it 指向要删除元素的常量迭代器
     * @return 指向下一个元素的常量迭代器
     */
    iterator erase(const_iterator it);

    /**
     * @brief 删除指定范围的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @note 与STL unordered_map::erase()兼容
     */
    iterator erase(iterator first, iterator last);

    /**
     * @brief 删除指定范围的元素（常量版本）
     * @param first 起始常量迭代器
     * @param last 结束常量迭代器
     */
    iterator erase(const_iterator first, const_iterator last);

    // ==================== 容量管理接口 ====================

    /**
     * @brief 调整大小提示
     * @param numElementsHint 元素数量提示
     * @note 与STL接口兼容但实际不执行操作（固定容量）
     */
    void resize(size_type numElementsHint);

    /**
     * @brief 清空所有元素
     * @note 与STL对比：
     *       - STL: std::unordered_map::clear() 行为完全一致
     *       - NFShmHashTable: 删除所有元素，重置为空状态
     *       - 所有节点回收到空闲链表，可重复使用
     *       - 桶数组重置为空状态，所有桶指向-1
     *       - 时间复杂度：O(n)，n为当前元素数量
     *       - 操作后size()返回0，full()返回false
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法
     * std::unordered_map<int, std::string> stdMap;
     * stdMap[1] = "one";
     * stdMap[2] = "two";
     * stdMap.clear();  // 清空所有元素
     * assert(stdMap.empty());
     * 
     * // NFShmHashTable用法
     * NFShmHashTable<std::pair<int, std::string>, int, 100, 
     *                std::hash<int>, select1st<std::pair<int, std::string>>, 
     *                std::equal_to<int>> table;
     * table.insert_unique(std::make_pair(1, "one"));
     * table.insert_unique(std::make_pair(2, "two"));
     * table.clear();  // 相同效果
     * assert(table.empty());
     * assert(table.left_size() == table.max_size());
     * ```
     */
    void clear();

    // ==================== 调试和诊断接口 ====================

    /**
     * @brief 打印哈希表结构信息
     * @note 调试用，显示桶链表结构
     */
    void print_structure() const;

    /**
     * @brief 打印详细信息
     * @note 调试用，显示所有节点状态
     */
    void print_detailed() const;

    /**
     * @brief 打印简化信息
     * @note 调试用，仅显示非空桶
     */
    void print_simple() const;

private:
    // ==================== 内部实现函数 ====================

    /**
     * @brief 初始化桶数组
     * @note 设置空闲链表和桶索引数组
     */
    void InitializeBuckets();

    /**
     * @brief 根据键计算桶索引
     * @param key 键值
     * @return 桶索引
     */
    size_type BktNumKey(const key_type& key) const;

    /**
     * @brief 根据值计算桶索引
     * @param obj 值对象
     * @return 桶索引
     */
    size_type BktNum(const value_type& obj) const;

    /**
     * @brief 根据键和桶数量计算桶索引
     * @param key 键值
     * @param n 桶数量
     * @return 桶索引
     */
    size_type BktNumKey(const key_type& key, size_t n) const;

    /**
     * @brief 根据值和桶数量计算桶索引
     * @param obj 值对象
     * @param n 桶数量
     * @return 桶索引
     */
    size_type MBktNum(const value_type& obj, size_t n) const;

    /**
     * @brief 创建新节点并初始化
     * @param obj 要存储的值
     * @return 新节点指针，失败返回nullptr
     */
    Node* NewNode(const value_type& obj);

    /**
     * @brief 删除节点并回收
     * @param pNode 要删除的节点指针
     */
    void DeleteNode(Node* pNode);

    /**
     * @brief 删除桶中指定范围的节点
     * @param n 桶索引
     * @param first 起始节点
     * @param last 结束节点
     */
    void EraseBucket(size_type n, Node* first, Node* last);

    /**
     * @brief 删除桶中从头到指定节点的所有节点
     * @param n 桶索引
     * @param last 结束节点
     */
    void EraseBucket(size_type n, Node* last);

    /**
     * @brief 从另一个哈希表复制内容
     * @param ht 源哈希表
     */
    void CopyFrom(const NFShmHashTable& ht);
};

// ==================== 静态成员定义 ====================

template <class Val, class Key, int MAX_SIZE, class Hf, class Ex, class Eq>
Val NFShmHashTable<Val, Key, MAX_SIZE, Hf, Ex, Eq>::m_staticError = Val();

// ==================== 迭代器实现 ====================

/**
 * @brief 迭代器前置递增操作符实现
 * @note 移动到下一个有效元素，跨桶遍历
 */
template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
NFShmHashTableIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&
NFShmHashTableIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>::operator++()
{
    const Node* old = m_curNode;
    if (old)
    {
        m_curNode = m_hashTable->GetValidNode(m_curNode->m_next);
        if (!m_curNode)
        {
            size_type bucket = m_hashTable->BktNum(old->m_value);
            while (!m_curNode && ++bucket < MAX_SIZE)
                m_curNode = m_hashTable->GetValidNode(m_hashTable->m_bucketsFirstIdx[bucket]);
        }
    }
    return *this;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
NFShmHashTableIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>
NFShmHashTableIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>::operator++(int)
{
    iterator tmp = *this;
    ++*this;
    return tmp;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&
NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>::operator++()
{
    const Node* old = m_curNode;
    if (old)
    {
        m_curNode = m_hashTable->GetValidNode(m_curNode->m_next);
        if (!m_curNode)
        {
            size_type bucket = m_hashTable->BktNum(old->m_value);
            while (!m_curNode && ++bucket < MAX_SIZE)
                m_curNode = m_hashTable->GetValidNode(m_hashTable->m_bucketsFirstIdx[bucket]);
        }
    }
    return *this;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>
NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>::operator++(int)
{
    const_iterator tmp = *this;
    ++*this;
    return tmp;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Node* NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::CreateNode()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(m_firstFreeIdx >= 0 && m_firstFreeIdx < MAX_SIZE, nullptr, "Invalid free index %d, valid range [0, %d), TRACE_STACK:%s", m_firstFreeIdx, MAX_SIZE, TRACE_STACK());
    auto pBuckets = GetBuckets();

    int iNowAssignIdx = m_firstFreeIdx;
    m_firstFreeIdx = pBuckets[m_firstFreeIdx].m_next;
    ++m_size;
    return &pBuckets[iNowAssignIdx];
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::RecycleNode(Node* p)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(p != nullptr, "Node pointer is null, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(p->m_valid, "Node is already invalid, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(p->m_self >= 0 && p->m_self < MAX_SIZE, "Node self index out of range: %zu, TRACE_STACK:%s", p->m_self, TRACE_STACK());
    CHECK_EXPR_RE_VOID(m_size > 0, "Size is already 0, cannot recycle node, TRACE_STACK:%s", TRACE_STACK());

    p->m_valid = false;
    p->m_next = m_firstFreeIdx;
    m_firstFreeIdx = p->m_self;
    --m_size;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::NFShmHashTable()
{
    m_hash = HashFcn();
    m_equals = EqualKey();
    m_getKey = ExtractKey();

    if (SHM_CREATE_MODE)
    {
        CreateInit();
    }
    else
    {
        ResumeInit();
    }
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::NFShmHashTable(const NFShmHashTable& ht)
{
    m_hash = HashFcn();
    m_equals = EqualKey();
    m_getKey = ExtractKey();
    CreateInit();
    CopyFrom(ht);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>& NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::operator=(const NFShmHashTable& ht)
{
    if (&ht != this)
    {
        CopyFrom(ht);
    }
    return *this;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::~NFShmHashTable()
{
    clear();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
int NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::CreateInit()
{
    InitializeBuckets();
    m_init = EN_NF_SHM_STL_INIT_OK;
    return 0;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
int NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::ResumeInit()
{
    if (m_init == EN_NF_SHM_STL_INIT_OK)
    {
        auto pNode = GetBuckets();
        // 对于非平凡构造的类型，恢复时需要调用构造函数
        if (!std::stl_is_trivially_default_constructible<Val>::value)
        {
            for (size_t i = 0; i < MAX_SIZE; i++)
            {
                if (pNode[i].m_valid)
                {
                    std::_Construct(&pNode[i].m_value);
                }
            }
        }
    }
    return 0;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Init()
{
    new(this) NFShmHashTable();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::max_size() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, MAX_SIZE, "not init, TRACE_STACK:%s", TRACE_STACK());
    return MAX_SIZE;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
bool NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::empty() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, true, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size == 0;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
bool NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::full() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size == MAX_SIZE;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
size_t NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::left_size() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size >= MAX_SIZE ? 0 : MAX_SIZE - m_size;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Node* NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::GetValidNode(int idx)
{
    if (idx >= 0 && idx < MAX_SIZE)
    {
        auto pBuckets = GetBuckets();
        auto pNode = &pBuckets[idx];
        CHECK_EXPR(pNode->m_self == idx, nullptr, "Node self index mismatch: expected %d, got %zu, TRACE_STACK:%s", idx, pNode->m_self, TRACE_STACK());
        if (pNode->m_valid)
        {
            return pNode;
        }
    }
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
const typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Node* NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::GetValidNode(int idx) const
{
    if (idx >= 0 && idx < MAX_SIZE)
    {
        auto pBuckets = GetBuckets();
        auto pNode = &pBuckets[idx];
        CHECK_EXPR(pNode->m_self == idx, nullptr, "Node self index mismatch: expected %d, got %zu, TRACE_STACK:%s", idx, pNode->m_self, TRACE_STACK());
        if (pNode->m_valid)
        {
            return pNode;
        }
    }
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::get_iterator(int idx)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(idx >= 0 && idx < MAX_SIZE, iterator(nullptr, this), "Index out of range: %d, TRACE_STACK:%s", idx, TRACE_STACK());
    return iterator(GetValidNode(idx), this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_iterator NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::get_iterator(int idx) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(idx >= 0 && idx < MAX_SIZE, const_iterator(nullptr, this), "Index out of range: %d, TRACE_STACK:%s", idx, TRACE_STACK());
    return const_iterator(GetValidNode(idx), this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::swap(NFShmHashTable& other) noexcept
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "this not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(other.m_init == EN_NF_SHM_STL_INIT_OK, , "other not init, TRACE_STACK:%s", TRACE_STACK());

    if (this != &other)
    {
        NFShmHashTable temp(*this);
        CopyFrom(other);
        other.CopyFrom(temp);
    }
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::begin()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    for (size_type n = 0; n < MAX_SIZE; ++n)
        if (m_bucketsFirstIdx[n] != -1)
            return iterator(GetValidNode(m_bucketsFirstIdx[n]), this);
    return end();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::end()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return iterator(nullptr, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_iterator NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::begin() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    for (size_type n = 0; n < MAX_SIZE; ++n)
        if (m_bucketsFirstIdx[n] != -1)
            return const_iterator(GetValidNode(m_bucketsFirstIdx[n]), this);
    return end();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_iterator NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::end() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return const_iterator(nullptr, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::bucket_count() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return MAX_SIZE;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::max_bucket_count() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return MAX_SIZE;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::elems_in_bucket(size_type bucket) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(bucket < MAX_SIZE, 0, "bucket index out of range: %lu >= %d, TRACE_STACK:%s", bucket, MAX_SIZE, TRACE_STACK());

    size_type result = 0;
    int curIndex = (int)bucket;
    int firstIdx = m_bucketsFirstIdx[curIndex];

    if (firstIdx >= 0 && firstIdx < MAX_SIZE)
    {
        auto pNode = GetValidNode(firstIdx);
        size_type maxIterations = m_size + 1; // 防止无限循环
        size_type iterations = 0;

        while (pNode && iterations < maxIterations)
        {
            result++;
            iterations++;
            pNode = GetValidNode(pNode->m_next);
        }

        CHECK_EXPR(iterations < maxIterations, result, "Possible infinite loop detected in bucket %lu, TRACE_STACK:%s", bucket, TRACE_STACK());
    }
    return result;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
std::pair<typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator, bool> NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(const value_type& obj)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, (std::pair<iterator, bool>(iterator(nullptr, this), false)), "not init, TRACE_STACK:%s", TRACE_STACK());
    return insert_unique_noresize(obj);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(const value_type& obj)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return insert_equal_noresize(obj);
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
std::forward_iterator_tag iterator_category(const NFShmHashTableIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return std::forward_iterator_tag();
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
Val* value_type(const NFShmHashTableIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
typename NFShmHashTable<Val, Key, MAX_SIZE, HF, ExK, Eqk>::difference_type* distance_type(const NFShmHashTableIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
std::forward_iterator_tag iterator_category(const NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return std::forward_iterator_tag();
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
Val* value_type(const NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
typename NFShmHashTable<Val, Key, MAX_SIZE, HF, ExK, Eqk>::difference_type* distance_type(const NFShmHashTableConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
bool operator==(const NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht1,
                const NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht2)
{
    if (ht1.size() != ht2.size()) return false;

    typedef typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::Node Node;
    for (int n = 0; n < MAX_SIZE; ++n)
    {
        const Node* cur1 = ht1.GetValidNode(ht1.m_bucketsFirstIdx[n]);
        const Node* cur2 = ht2.GetValidNode(ht2.m_bucketsFirstIdx[n]);

        while (cur1 && cur2)
        {
            if (!(cur1->m_value == cur2->m_value))
                return false;
            cur1 = ht1.GetValidNode(cur1->m_next);
            cur2 = ht2.GetValidNode(cur2->m_next);
        }

        if (cur1 || cur2) // 链表长度不同
            return false;
    }
    return true;
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
bool operator!=(const NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht1,
                const NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht2)
{
    return !(ht1 == ht2);
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void swap(NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht1, NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht2) noexcept
{
    ht1.swap(ht2);
}


template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
std::pair<typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator, bool> NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::insert_unique_noresize(const value_type& obj)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, (std::pair<iterator, bool>(iterator(nullptr, this), false)), "not init, TRACE_STACK:%s", TRACE_STACK());
    const size_type n = BktNum(obj);
    CHECK_EXPR(n < MAX_SIZE, (std::pair<iterator, bool>(end(), false)), "bucket index n:%lu >= MAX_SIZE:%lu, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());

    int iFirstIndex = m_bucketsFirstIdx[n];
    for (Node* cur = GetValidNode(iFirstIndex); cur; cur = GetValidNode(cur->m_next))
    {
        if (m_equals(m_getKey(cur->m_value), m_getKey(obj)))
        {
            return std::pair<iterator, bool>(iterator(cur, this), false);
        }
    }

    Node* tmp = NewNode(obj);
    if (tmp == nullptr)
    {
        return std::pair<iterator, bool>(end(), false);
    }

    tmp->m_next = iFirstIndex;
    m_bucketsFirstIdx[n] = tmp->m_self;
    return std::pair<iterator, bool>(iterator(tmp, this), true);
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::insert_equal_noresize(const value_type& obj)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());

    //已经没有可用的节点了
    const size_type n = BktNum(obj);
    CHECK_EXPR(n < MAX_SIZE, end(), "bucket index n:%lu >= MAX_SIZE:%lu, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());

    int iFirstIndex = m_bucketsFirstIdx[n];

    for (Node* cur = GetValidNode(iFirstIndex); cur; cur = GetValidNode(cur->m_next))
    {
        if (m_equals(m_getKey(cur->m_value), m_getKey(obj)))
        {
            Node* tmp = NewNode(obj);
            if (tmp == nullptr)
            {
                return end();
            }
            tmp->m_next = cur->m_next;
            cur->m_next = tmp->m_self;
            return iterator(tmp, this);
        }
    }

    Node* tmp = NewNode(obj);
    if (tmp == nullptr)
    {
        return end();
    }

    tmp->m_next = iFirstIndex;
    m_bucketsFirstIdx[n] = tmp->m_self;
    return iterator(tmp, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class InputIterator>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(InputIterator f, InputIterator l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    insert_unique(f, l, typename std::iterator_traits<InputIterator>::iterator_category());
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class InputIterator>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(InputIterator f, InputIterator l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    insert_equal(f, l, typename std::iterator_traits<InputIterator>::iterator_category());
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class InputIterator>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(InputIterator f, InputIterator l, std::input_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (; f != l; ++f)
        insert_unique(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class InputIterator>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(InputIterator f, InputIterator l, std::input_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (; f != l; ++f)
        insert_equal(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class ForwardIterator>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(ForwardIterator f, ForwardIterator l, std::forward_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = std::distance(f, l);
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTable does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_unique_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class ForwardIterator>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(ForwardIterator f, ForwardIterator l, std::forward_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = std::distance(f, l);
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTable does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_equal_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(const value_type* f, const value_type* l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = l - f;
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTable does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_unique_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(const value_type* f, const value_type* l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = l - f;
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTable does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_equal_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(const_iterator f, const_iterator l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = std::distance(f, l);
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTable does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_unique_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(const_iterator f, const_iterator l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = std::distance(f, l);
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTable does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_equal_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::reference NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::find_or_insert(const value_type& obj)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
    const size_type n = BktNum(obj);
    CHECK_EXPR(n < MAX_SIZE, m_staticError, "bucket index n:%lu >= MAX_SIZE:%lu, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());

    int iFirstIndex = m_bucketsFirstIdx[n];
    for (Node* cur = GetValidNode(iFirstIndex); cur; cur = GetValidNode(cur->m_next))
    {
        if (m_equals(m_getKey(cur->m_value), m_getKey(obj)))
        {
            return cur->m_value;
        }
    }

    Node* tmp = NewNode(obj);
    CHECK_EXPR(tmp != nullptr, m_staticError, "Failed to create new node, TRACE_STACK:%s", TRACE_STACK());

    tmp->m_next = m_bucketsFirstIdx[n];
    m_bucketsFirstIdx[n] = tmp->m_self;
    return tmp->m_value;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::find(const key_type& key)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = BktNumKey(key);
    CHECK_EXPR(n < MAX_SIZE, end(), "n:%lu >= MAX_SIZE:%lu TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
    int iFirstIndex = m_bucketsFirstIdx[n];

    Node* first;
    for (first = GetValidNode(iFirstIndex);
         first && !m_equals(m_getKey(first->m_value), key);
         first = GetValidNode(first->m_next))
    {
    }

    return iterator(first, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_iterator NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::find(const key_type& key) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = BktNumKey(key);
    CHECK_EXPR(n < MAX_SIZE, end(), "bucket index n:%lu >= MAX_SIZE:%lu, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
    int iFirstIndex = m_bucketsFirstIdx[n];

    const Node* first;
    for (first = GetValidNode(iFirstIndex);
         first && !m_equals(m_getKey(first->m_value), key);
         first = GetValidNode(first->m_next))
    {
    }

    return const_iterator(first, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::count(const key_type& key) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    const size_type n = BktNumKey(key);
    size_type result = 0;
    CHECK_EXPR(n < MAX_SIZE, 0, "n:%lu >= MAX_SIZE:%lu TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
    int iFirstIndex = m_bucketsFirstIdx[n];

    for (const Node* cur = GetValidNode(iFirstIndex); cur; cur = GetValidNode(cur->m_next))
    {
        if (m_equals(m_getKey(cur->m_value), key))
        {
            ++result;
        }
    }

    return result;
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
std::pair<typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator, typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator> NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::equal_range(const key_type& key)
{
    typedef std::pair<iterator, iterator> pii;
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, pii(end(), end()), "not init, TRACE_STACK:%s", TRACE_STACK());
    const size_type n = BktNumKey(key);
    CHECK_EXPR(n < MAX_SIZE, (pii(end(), end())), "bucket index n:%lu >= MAX_SIZE:%lu, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());

    int iFirstIndex = m_bucketsFirstIdx[n];
    for (Node* first = GetValidNode(iFirstIndex); first; first = GetValidNode(first->m_next))
    {
        if (m_equals(m_getKey(first->m_value), key))
        {
            for (Node* cur = GetValidNode(first->m_next); cur; cur = GetValidNode(cur->m_next))
            {
                if (!m_equals(m_getKey(cur->m_value), key))
                {
                    return pii(iterator(first, this), iterator(cur, this));
                }
            }
            for (size_type m = n + 1; m < MAX_SIZE; ++m)
            {
                if (m_bucketsFirstIdx[m] != -1)
                {
                    return pii(iterator(first, this), iterator(GetValidNode(m_bucketsFirstIdx[m]), this));
                }
            }
            return pii(iterator(first, this), end());
        }
    }
    return pii(end(), end());
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
std::pair<typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::const_iterator, typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::const_iterator>
NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::equal_range(const key_type& key) const
{
    typedef std::pair<const_iterator, const_iterator> pii;
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, (pii(end(), end())), "not init, TRACE_STACK:%s", TRACE_STACK());
    const size_type n = BktNumKey(key);
    CHECK_EXPR(n < MAX_SIZE, (pii(end(), end())), "bucket index n:%lu >= MAX_SIZE:%lu, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());

    int iFirstIndex = m_bucketsFirstIdx[n];
    for (const Node* first = GetValidNode(iFirstIndex); first; first = GetValidNode(first->m_next))
    {
        if (m_equals(m_getKey(first->m_value), key))
        {
            for (const Node* cur = GetValidNode(first->m_next); cur; cur = GetValidNode(cur->m_next))
            {
                if (!m_equals(m_getKey(cur->m_value), key))
                {
                    return pii(const_iterator(first, this), const_iterator(cur, this));
                }
            }
            for (size_type m = n + 1; m < MAX_SIZE; ++m)
            {
                if (m_bucketsFirstIdx[m] != -1)
                {
                    return pii(const_iterator(first, this), const_iterator(GetValidNode(m_bucketsFirstIdx[m]), this));
                }
            }
            return pii(const_iterator(first, this), end());
        }
    }
    return pii(end(), end());
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(const key_type& key)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    const size_type n = BktNumKey(key);
    CHECK_EXPR(n < MAX_SIZE, 0, "bucket index n:%lu >= MAX_SIZE:%lu, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
    int iFirstIndex = m_bucketsFirstIdx[n];
    size_type erased = 0;

    Node* first = GetValidNode(iFirstIndex);
    if (first)
    {
        Node* cur = first;
        Node* next = GetValidNode(cur->m_next);
        while (next)
        {
            if (m_equals(m_getKey(next->m_value), key))
            {
                cur->m_next = next->m_next;
                DeleteNode(next);
                next = GetValidNode(cur->m_next);
                ++erased;
            }
            else
            {
                cur = next;
                next = GetValidNode(cur->m_next);
            }
        }
        if (m_equals(m_getKey(first->m_value), key))
        {
            m_bucketsFirstIdx[n] = first->m_next;
            DeleteNode(first);
            ++erased;
        }
    }
    return erased;
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(iterator it)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(it.m_curNode != nullptr, end(), "Iterator is null, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(it.m_curNode->m_valid, end(), "Iterator points to invalid node, TRACE_STACK:%s", TRACE_STACK());

    Node* nodeToDelete = it.m_curNode;

    // 先获取下一个迭代器，再删除当前节点
    iterator nextIter = it;
    ++nextIter; // 利用iterator++操作获取下一个有效迭代器

    // 现在安全地删除节点
    const size_type bucketIndex = BktNum(nodeToDelete->m_value);
    CHECK_EXPR(bucketIndex < MAX_SIZE, end(), "Bucket index out of range: %zu, TRACE_STACK:%s", bucketIndex, TRACE_STACK());

    int& bucketHead = m_bucketsFirstIdx[bucketIndex];
    Node* currentNode = GetValidNode(bucketHead);

    if (currentNode == nodeToDelete)
    {
        // 删除链表头节点
        bucketHead = nodeToDelete->m_next;
        DeleteNode(nodeToDelete);
        return nextIter;
    }
    else
    {
        // 删除链表中间或尾部节点
        while (currentNode && currentNode->m_next != -1)
        {
            Node* nextInChain = GetValidNode(currentNode->m_next);
            if (nextInChain == nodeToDelete)
            {
                currentNode->m_next = nodeToDelete->m_next;
                DeleteNode(nodeToDelete);
                return nextIter;
            }
            currentNode = nextInChain;
        }

        // 如果没有找到要删除的节点，这是一个错误情况
        CHECK_EXPR(false, end(), "Node to delete not found in bucket chain, TRACE_STACK:%s", TRACE_STACK());
        return end();
    }
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(iterator first, iterator last)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type fBucket = first.m_curNode ? BktNum(first.m_curNode->m_value) : MAX_SIZE;
    size_type lBucket = last.m_curNode ? BktNum(last.m_curNode->m_value) : MAX_SIZE;

    if (first.m_curNode == last.m_curNode)
    {
        return last;
    }
    else if (fBucket == lBucket)
    {
        EraseBucket(fBucket, first.m_curNode, last.m_curNode);
        return last;
    }
    else
    {
        EraseBucket(fBucket, first.m_curNode, nullptr);
        for (size_type n = fBucket + 1; n < lBucket; ++n)
            EraseBucket(n, nullptr);
        if (lBucket != MAX_SIZE)
            EraseBucket(lBucket, last.m_curNode);
    }
    return last;
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(const_iterator first, const_iterator last)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
    return erase(iterator(const_cast<Node*>(first.m_curNode),
                   const_cast<NFShmHashTable*>(first.m_hashTable)),
          iterator(const_cast<Node*>(last.m_curNode),
                   const_cast<NFShmHashTable*>(last.m_hashTable)));
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
typename NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(const_iterator it)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
    return erase(iterator(const_cast<Node*>(it.m_curNode),
                          const_cast<NFShmHashTable*>(it.m_hashTable)));
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>
::resize(size_type)
{
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::EraseBucket(const size_type n, Node* first, Node* last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(n < MAX_SIZE, "n:%zu >= MAX_SIZE:%d, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
    Node* cur = GetValidNode(m_bucketsFirstIdx[n]);
    CHECK_EXPR_RE_VOID(cur, "TRACE_STACK:%s", TRACE_STACK());
    if (cur == first)
        EraseBucket(n, last);
    else
    {
        Node* next;
        for (next = GetValidNode(cur->m_next);
             next != first;
             cur = next, next = GetValidNode(cur->m_next)) {}
        while (next != last)
        {
            cur->m_next = next->m_next;
            DeleteNode(next);
            next = GetValidNode(cur->m_next);
        }
    }
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::EraseBucket(const size_type n, Node* last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(n < MAX_SIZE, "n:%zu >= MAX_SIZE:%d, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
    Node* cur = GetValidNode(m_bucketsFirstIdx[n]);
    if (!cur) return;

    while (cur != last)
    {
        if (cur)
        {
            Node* next = GetValidNode(cur->m_next);
            DeleteNode(cur);
            cur = next;
        }
        else
        {
            LOG_ERR(0, -1, "error, TRACE_STACK:%s", TRACE_STACK());
            break;
        }

        m_bucketsFirstIdx[n] = cur ? cur->m_self : -1;
    }
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::clear()
{
    for (int i = 0; i < MAX_SIZE; i++)
    {
        auto pNode = GetValidNode(m_bucketsFirstIdx[i]);
        while (pNode)
        {
            int next = pNode->m_next;
            DeleteNode(pNode);
            pNode = GetValidNode(next);
        }
        m_bucketsFirstIdx[i] = INVALID_ID;
    }
    InitializeBuckets();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::print_structure() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== NFShmHashTable Structure ===\n");
    printf("Size: %zu, Max Size: %d, First Free Index: %d\n", m_size, MAX_SIZE, m_firstFreeIdx);
    printf("=====================================\n");

    size_type totalNodes = 0;
    size_type emptyBuckets = 0;

    for (size_type i = 0; i < MAX_SIZE; ++i)
    {
        int firstIdx = m_bucketsFirstIdx[i];
        if (firstIdx == -1)
        {
            emptyBuckets++;
            continue;
        }

        printf("Bucket[%3zu]: ", i);

        // Print linked list
        const Node* cur = GetValidNode(firstIdx);
        size_type chainLength = 0;
        size_type maxChainLength = m_size + 1; // Prevent infinite loop

        while (cur && chainLength < maxChainLength)
        {
            printf("[%zu", cur->m_self);

            // Print key information
            try
            {
                auto key = m_getKey(cur->m_value);
                printf("(k:");
                if (std::is_arithmetic<decltype(key)>::value)
                {
                    if (std::is_integral<decltype(key)>::value)
                    {
                        printf("%lld", (long long)key);
                    }
                    else
                    {
                        printf("%.2f", (double)key);
                    }
                }
                else
                {
                    printf("?");
                }
                printf(")");
            }
            catch (...)
            {
                printf("(k:?)");
            }

            printf("]");
            totalNodes++;
            chainLength++;

            if (cur->m_next != -1)
            {
                printf(" -> ");
                cur = GetValidNode(cur->m_next);
            }
            else
            {
                break;
            }
        }

        if (chainLength >= maxChainLength)
        {
            printf(" ... (Loop detected!)");
        }

        printf(" (Length: %zu)\n", chainLength);
    }

    printf("=====================================\n");
    printf("Statistics:\n");
    printf("  Non-empty buckets: %zu\n", MAX_SIZE - emptyBuckets);
    printf("  Empty buckets: %zu\n", emptyBuckets);
    printf("  Total traversed nodes: %zu\n", totalNodes);
    printf("  Recorded size: %zu\n", m_size);

    if (totalNodes != m_size)
    {
        printf("  Warning: Traversed nodes count doesn't match recorded size!\n");
    }

    // Print free list
    printf("\nFree list: ");
    if (m_firstFreeIdx == -1)
    {
        printf("Empty\n");
    }
    else
    {
        auto pBuckets = GetBuckets();
        int freeIdx = m_firstFreeIdx;
        size_type freeCount = 0;
        size_type maxFreeCount = MAX_SIZE; // Prevent infinite loop

        while (freeIdx != -1 && freeCount < maxFreeCount)
        {
            printf("[%d]", freeIdx);
            freeCount++;

            if (freeIdx >= 0 && freeIdx < MAX_SIZE)
            {
                freeIdx = pBuckets[freeIdx].m_next;
                if (freeIdx != -1)
                {
                    printf(" -> ");
                }
            }
            else
            {
                printf(" (Invalid index!)");
                break;
            }
        }

        if (freeCount >= maxFreeCount)
        {
            printf(" ... (Loop detected!)");
        }

        printf(" (Free nodes: %zu)\n", freeCount);

        size_type expectedFreeCount = MAX_SIZE - m_size;
        if (freeCount != expectedFreeCount)
        {
            printf("  Warning: Free nodes count %zu doesn't match expected %zu!\n", freeCount, expectedFreeCount);
        }
    }

    printf("=====================================\n\n");
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::print_detailed() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== NFShmHashTable Detailed View ===\n");
    printf("Size: %zu, Max Size: %d, First Free Index: %d\n", m_size, MAX_SIZE, m_firstFreeIdx);
    printf("=====================================\n");

    auto pBuckets = GetBuckets();

    // Print all node states
    printf("Node Status Table:\n");
    printf("Index Valid SelfRef  Next    Value/Key Info\n");
    printf("----- ----- -------  ----    --------------\n");

    for (int i = 0; i < MAX_SIZE; ++i)
    {
        printf("%5d %5s %7zu %6d  ",
               i,
               pBuckets[i].m_valid ? "Yes" : "No",
               pBuckets[i].m_self,
               pBuckets[i].m_next);

        if (pBuckets[i].m_valid)
        {
            try
            {
                // Try to get key-value info (assuming key can be converted to string or number)
                auto key = m_getKey(pBuckets[i].m_value);
                printf("Key: ");

                // Print based on key type
                if (std::is_arithmetic<decltype(key)>::value)
                {
                    if (std::is_integral<decltype(key)>::value)
                    {
                        printf("%lld", (long long)key);
                    }
                    else
                    {
                        printf("%.2f", (double)key);
                    }
                }
                else
                {
                    printf("(Complex type)");
                }

                // Calculate and show expected bucket
                size_type expectedBucket = BktNumKey(key);
                printf(", Expected bucket: %zu", expectedBucket);
            }
            catch (...)
            {
                printf("(Cannot get key info)");
            }
        }
        else
        {
            printf("(Free node)");
        }

        printf("\n");
    }

    printf("\nBucket Chain Details:\n");
    printf("=====================================\n");

    for (size_type i = 0; i < MAX_SIZE; ++i)
    {
        int firstIdx = m_bucketsFirstIdx[i];
        if (firstIdx == -1) continue;

        printf("Bucket[%3zu] -> ", i);

        const Node* cur = GetValidNode(firstIdx);
        size_type chainLength = 0;
        size_type maxChainLength = m_size + 1;

        while (cur && chainLength < maxChainLength)
        {
            printf("Node%zu", cur->m_self);

            if (cur->m_valid)
            {
                try
                {
                    auto key = m_getKey(cur->m_value);
                    printf("(");

                    if (std::is_arithmetic<decltype(key)>::value)
                    {
                        if (std::is_integral<decltype(key)>::value)
                        {
                            printf("k:%lld", (long long)key);
                        }
                        else
                        {
                            printf("k:%.2f", (double)key);
                        }
                    }
                    else
                    {
                        printf("k:?");
                    }
                    printf(")");
                }
                catch (...)
                {
                    printf("(k:?)");
                }
            }
            else
            {
                printf("(Invalid!)");
            }

            chainLength++;

            if (cur->m_next != -1)
            {
                printf(" -> ");
                cur = GetValidNode(cur->m_next);
            }
            else
            {
                break;
            }
        }

        if (chainLength >= maxChainLength)
        {
            printf(" ... (Loop!)");
        }

        printf("\n");
    }

    printf("=====================================\n\n");
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::print_simple() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== Hash Table Simple View ===\n");
    printf("Size: %zu/%d, Free head: %d\n", m_size, MAX_SIZE, m_firstFreeIdx);

    // Only show non-empty buckets
    size_type nonEmptyBuckets = 0;
    for (size_type i = 0; i < MAX_SIZE; ++i)
    {
        if (m_bucketsFirstIdx[i] != -1)
        {
            nonEmptyBuckets++;
            printf("%zu: ", i);

            const Node* cur = GetValidNode(m_bucketsFirstIdx[i]);
            size_type count = 0;
            while (cur && count < 10) // Show at most 10 nodes
            {
                printf("%zu", cur->m_self);

                // Print key information
                try
                {
                    auto key = m_getKey(cur->m_value);
                    printf("(");
                    if (std::is_arithmetic<decltype(key)>::value)
                    {
                        if (std::is_integral<decltype(key)>::value)
                        {
                            printf("%lld", (long long)key);
                        }
                        else
                        {
                            printf("%.2f", (double)key);
                        }
                    }
                    else
                    {
                        printf("?");
                    }
                    printf(")");
                }
                catch (...)
                {
                    printf("(?)");
                }

                count++;

                if (cur->m_next != -1)
                {
                    printf("->");
                    cur = GetValidNode(cur->m_next);
                }
                else
                {
                    break;
                }
            }

            if (count >= 10)
            {
                printf("...");
            }

            printf(" (%zu items)\n", count);
        }
    }

    if (nonEmptyBuckets == 0)
    {
        printf("(All buckets are empty)\n");
    }

    printf("Non-empty buckets: %zu, Load factor: %.2f\n",
           nonEmptyBuckets,
           (double)m_size / MAX_SIZE);
    printf("==============================\n\n");
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::InitializeBuckets()
{
    m_size = 0;
    m_firstFreeIdx = 0;
    auto pBuckets = GetBuckets();
    for (int i = 0; i < MAX_SIZE; ++i)
    {
        pBuckets[i].m_next = i + 1;
        pBuckets[i].m_valid = false;
        pBuckets[i].m_self = i;
    }

    pBuckets[MAX_SIZE - 1].m_next = -1;

    for (int i = 0; i < MAX_SIZE; ++i)
    {
        m_bucketsFirstIdx[i] = -1;
    }
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::BktNumKey(const key_type& key) const
{
    return BktNumKey(key, MAX_SIZE);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::BktNum(const value_type& obj) const
{
    return BktNumKey(m_getKey(obj));
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::BktNumKey(const key_type& key, size_t n) const
{
    CHECK_EXPR(n > 0, 0, "Bucket count cannot be zero, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(n <= MAX_SIZE, 0, "Bucket count %zu exceeds MAX_SIZE %d, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());

    size_t hashValue = m_hash(key);
    return hashValue % n;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::MBktNum(const value_type& obj, size_t n) const
{
    return BktNumKey(m_getKey(obj), n);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Node* NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::NewNode(const value_type& obj)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), nullptr, "HashTable is full, cannot create new node, TRACE_STACK:%s", TRACE_STACK());

    Node* pNode = CreateNode();
    if (pNode)
    {
        CHECK_EXPR(pNode->m_valid == false, nullptr, "Node should be invalid before initialization, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(pNode->m_self >= 0 && pNode->m_self < MAX_SIZE, nullptr, "Node self index out of range: %zu, TRACE_STACK:%s", pNode->m_self, TRACE_STACK());

        pNode->m_valid = true;
        pNode->m_next = -1;

        try
        {
            std::_Construct(&pNode->m_value, obj);
        }
        catch (...)
        {
            // 如果构造失败，回滚状态
            RecycleNode(pNode);
            return nullptr;
        }
    }

    return pNode;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::DeleteNode(Node* pNode)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode != nullptr, "Node pointer is null, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_valid, "Node is already invalid, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_self >= 0 && pNode->m_self < MAX_SIZE, "Node self index out of range: %zu, TRACE_STACK:%s", pNode->m_self, TRACE_STACK());

    try
    {
        std::_Destroy(&pNode->m_value);
    }
    catch (...)
    {
        // 即使析构失败，也要回收节点
    }
    RecycleNode(pNode);
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void NFShmHashTable<Val, Key, MAX_SIZE, HF, Ex, Eq>::CopyFrom(const NFShmHashTable& ht)
{
    if (this == &ht) return;

    clear();
    for (int i = 0; i < MAX_SIZE; i++)
    {
        auto pCur = ht.GetValidNode(ht.m_bucketsFirstIdx[i]);
        if (pCur)
        {
            // Create first node in the chain
            auto pCopy = NewNode(pCur->m_value);
            if (!pCopy)
            {
                // If allocation fails, clear what we've copied so far and return
                clear();
                return;
            }
            m_bucketsFirstIdx[i] = pCopy->m_self;

            // Copy the rest of the chain, maintaining order
            for (auto pNext = ht.GetValidNode(pCur->m_next); pNext; pCur = pNext, pNext = ht.GetValidNode(pNext->m_next))
            {
                auto pNewCopy = NewNode(pNext->m_value);
                if (!pNewCopy)
                {
                    // If allocation fails, clear what we've copied so far and return
                    clear();
                    return;
                }
                pCopy->m_next = pNewCopy->m_self;
                pCopy = pNewCopy;
            }
        }
    }
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::value_type& NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::at(const key_type& key)
{
    iterator it = find(key);
    if (it == end())
    {
        LOG_ERR(0, -1, "NFShmHashTable::at: key not found, TRACE_STACK:%s", TRACE_STACK());
        return m_staticError;
    }
    return *it;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
const typename NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::value_type& NFShmHashTable<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::at(const key_type& key) const
{
    const_iterator it = find(key);
    if (it == end())
    {
        LOG_ERR(0, -1, "NFShmHashTable::at: key not found, TRACE_STACK:%s", TRACE_STACK());
        return m_staticError;
    }
    return *it;
}
