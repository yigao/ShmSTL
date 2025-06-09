// -------------------------------------------------------------------------
//    @FileName         :    NFShmHashMapWithList.h
//    @Author           :    gaoyi
//    @Date             :    23-2-11
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmHashMapWithList
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmStl.h"
#include "NFShmHashTableWithList.h"
#include <map>
#include <unordered_map>
#include "NFShmPair.h"

/**
 * @file NFShmHashMapWithList.h
 * @brief 基于共享内存的无序映射容器实现（链表增强版），与STL std::unordered_map高度兼容
 * 
 * @section overview 概述
 * 
 * NFShmHashMapWithList 是一个专为共享内存环境设计的键值映射容器，在NFShmHashMap基础上
 * 增加了双向链表支持，提供LRU（最近最少使用）缓存语义和顺序遍历功能。它在API设计上与STL
 * std::unordered_map高度兼容，但在内存管理、容量控制和访问模式方面针对共享内存场景进行
 * 了深度优化。
 * 
 * @section features 核心特性
 * 
 * 1. **STL高度兼容**：
 *    - 完整的std::unordered_map API支持
 *    - 标准的迭代器接口和类型定义
 *    - operator[]、at()、find()等常用操作
 *    - 支持范围for循环和STL算法
 * 
 * 2. **双向链表增强**：
 *    - 内建双向链表，维护元素访问/插入顺序
 *    - list_begin()、list_end()提供顺序遍历
 *    - LRU缓存语义，自动淘汰最久未使用元素
 *    - enable_lru()/disable_lru()动态控制LRU行为
 * 
 * 3. **共享内存优化**：
 *    - 固定大小内存布局，避免动态分配
 *    - 基于NFShmPair的键值对存储
 *    - 支持CREATE/RESUME两阶段初始化
 *    - 无内存碎片，高效的内存使用
 * 
 * 4. **缓存友好设计**：
 *    - O(1)平均时间复杂度的哈希操作
 *    - 链表维护访问顺序，适合LRU淘汰策略
 *    - 预分配节点池，快速内存分配
 *    - 固定桶数量，无rehash开销
 * 
 * @section stl_comparison 与STL std::unordered_map对比
 * 
 * | 特性 | std::unordered_map | NFShmHashMapWithList |
 * |------|-------------------|---------------------|
 * | **内存管理** | 动态堆内存分配 | 固定共享内存预分配 |
 * | **容量限制** | 无限制，动态扩容 | 固定容量MAX_SIZE |
 * | **扩容机制** | 自动rehash扩容 | **不支持扩容** |
 * | **键值对类型** | std::pair<const Key, T> | NFShmPair<Key, T> |
 * | **访问顺序** | 无保证，依赖哈希顺序 | **双向链表维护顺序** |
 * | **LRU支持** | 不支持 | **原生LRU缓存语义** |
 * | **顺序遍历** | 仅哈希桶顺序 | **链表顺序遍历** |
 * | **缓存淘汰** | 不支持 | **自动LRU淘汰机制** |
 * | **进程共享** | 不支持 | **原生支持** |
 * | **初始化方式** | 构造函数 | CreateInit/ResumeInit |
 * | **负载因子** | 动态调整，max_load_factor | 固定结构，无动态调整 |
 * | **异常安全** | 强异常安全保证 | 无异常，错误码返回 |
 * | **内存碎片** | 可能产生 | **无碎片** |
 * | **性能稳定性** | rehash时性能抖动 | **性能稳定** |
 * | **迭代器稳定性** | rehash时失效 | **始终稳定** |
 * | **operator[]** | 完全支持 | 完全支持 |
 * | **at()访问** | 异常安全访问 | 边界检查访问 |
 * 
 * @section api_compatibility API兼容性
 * 
 * **完全兼容的接口**：
 * - **容器属性**：size(), empty(), max_size()
 * - **迭代器**：begin(), end(), cbegin(), cend()
 * - **查找访问**：find(), count(), at(), operator[]
 * - **修改操作**：insert(), emplace(), erase(), clear()
 * - **容器操作**：swap()
 * - **桶接口**：bucket_count(), max_bucket_count(), elems_in_bucket()
 * 
 * **扩展的接口（新增）**：
 * - **容量检查**：full(), left_size()
 * - **共享内存**：CreateInit(), ResumeInit()
 * - **链表遍历**：list_begin(), list_end(), list_cbegin(), list_cend()
 * - **LRU控制**：enable_lru(), disable_lru(), is_lru_enabled()
 * - **STL转换**：从std::unordered_map和std::map构造
 * 
 * **不支持的接口**：
 * - **动态管理**：rehash(), reserve(), max_load_factor()
 * - **哈希策略**：load_factor(), bucket_size()
 * - **自定义分配器**：get_allocator()
 * 
 * @section usage_examples 使用示例
 * 
 * @subsection basic_usage 基础用法（类似std::unordered_map）
 * 
 * ```cpp
 * // 定义容量为1000的字符串到整数映射（带链表）
 * NFShmHashMapWithList<std::string, int, 1000> userCacheMap;
 * userCacheMap.CreateInit();  // 创建模式初始化
 * 
 * // STL兼容的基础操作
 * userCacheMap["alice"] = 100;
 * userCacheMap["bob"] = 200;
 * userCacheMap["charlie"] = 300;
 * 
 * // 查找操作（与STL完全一致）
 * auto it = userCacheMap.find("alice");
 * if (it != userCacheMap.end()) {
 *     std::cout << "Found: " << it->first << " = " << it->second << std::endl;
 * }
 * 
 * // 安全访问（边界检查）
 * try {
 *     int score = userCacheMap.at("david");  // 不存在则返回错误
 * } catch (...) {
 *     std::cout << "Key not found" << std::endl;
 * }
 * 
 * // 统计和删除
 * std::cout << "Count: " << userCacheMap.count("bob") << std::endl;  // 输出：1
 * userCacheMap.erase("bob");
 * 
 * // 哈希遍历（无序）
 * std::cout << "Hash order traversal:" << std::endl;
 * for (const auto& pair : userCacheMap) {
 *     std::cout << pair.first << ": " << pair.second << std::endl;
 * }
 * 
 * // 链表遍历（按访问/插入顺序）
 * std::cout << "List order traversal:" << std::endl;
 * for (auto it = userCacheMap.list_begin(); it != userCacheMap.list_end(); ++it) {
 *     std::cout << it->first << ": " << it->second << std::endl;
 * }
 * ```
 * 
 * @subsection lru_cache_usage LRU缓存使用
 * 
 * ```cpp
 * NFShmHashMapWithList<int, std::string, 100> lruCache;
 * lruCache.CreateInit();
 * 
 * // 启用LRU缓存语义
 * lruCache.enable_lru();
 * 
 * // 批量插入数据
 * for (int i = 1; i <= 100; ++i) {
 *     lruCache[i] = "data_" + std::to_string(i);
 * }
 * 
 * std::cout << "Cache is full: " << lruCache.full() << std::endl;  // true
 * 
 * // 访问一些元素（会更新它们在LRU链表中的位置）
 * auto val1 = lruCache[1];   // 将键1移到LRU链表头部
 * auto val10 = lruCache[10]; // 将键10移到LRU链表头部
 * auto val50 = lruCache[50]; // 将键50移到LRU链表头部
 * 
 * // 插入新元素（会自动淘汰最久未使用的元素）
 * lruCache[101] = "new_data_101";  // 自动删除LRU链表尾部的元素
 * lruCache[102] = "new_data_102";  // 继续淘汰
 * 
 * std::cout << "Cache size after LRU: " << lruCache.size() << std::endl;  // 仍然100
 * 
 * // 检查最近访问的元素仍然存在
 * std::cout << "Key 1 exists: " << (lruCache.count(1) > 0) << std::endl;   // true
 * std::cout << "Key 10 exists: " << (lruCache.count(10) > 0) << std::endl; // true
 * std::cout << "Key 50 exists: " << (lruCache.count(50) > 0) << std::endl; // true
 * 
 * // 最先插入且未被访问的元素可能已被淘汰
 * std::cout << "Key 2 exists: " << (lruCache.count(2) > 0) << std::endl;   // 可能为false
 * 
 * // 按LRU顺序遍历（最近使用的在前）
 * std::cout << "LRU order (most recent first):" << std::endl;
 * int count = 0;
 * for (auto it = lruCache.list_begin(); it != lruCache.list_end() && count < 5; ++it, ++count) {
 *     std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
 * }
 * ```
 * 
 * @subsection access_pattern_tracking 访问模式跟踪
 * 
 * ```cpp
 * NFShmHashMapWithList<std::string, int, 500> accessTracker;
 * 
 * // 禁用LRU，仅跟踪访问顺序
 * accessTracker.disable_lru();
 * 
 * // 模拟用户访问模式
 * accessTracker["page_home"] = 1;
 * accessTracker["page_login"] = 1;
 * accessTracker["page_profile"] = 1;
 * 
 * // 模拟用户访问（会影响链表顺序）
 * accessTracker["page_home"]++;      // 访问首页
 * accessTracker["page_profile"]++;   // 访问个人资料
 * accessTracker["page_home"]++;      // 再次访问首页
 * 
 * // 添加新页面访问
 * accessTracker["page_settings"] = 1;
 * accessTracker["page_help"] = 1;
 * 
 * // 按访问顺序查看页面（最后访问的在链表头部）
 * std::cout << "Page access order (most recent first):" << std::endl;
 * for (auto it = accessTracker.list_begin(); it != accessTracker.list_end(); ++it) {
 *     std::cout << "Page: " << it->first << ", Visits: " << it->second << std::endl;
 * }
 * 
 * // 统计访问模式
 * std::cout << "Total pages: " << accessTracker.size() << std::endl;
 * 
 * // 查找热门页面
 * auto max_visits = std::max_element(accessTracker.begin(), accessTracker.end(),
 *     [](const auto& a, const auto& b) { return a.second < b.second; });
 * 
 * if (max_visits != accessTracker.end()) {
 *     std::cout << "Most visited page: " << max_visits->first 
 *               << " (" << max_visits->second << " visits)" << std::endl;
 * }
 * ```
 * 
 * @subsection capacity_management 容量管理
 * 
 * ```cpp
 * NFShmHashMapWithList<int, std::string, 100> limitedMap;
 * 
 * // 容量检查（STL没有的功能）
 * std::cout << "Max size: " << limitedMap.max_size() << std::endl;      // 100
 * std::cout << "Current size: " << limitedMap.size() << std::endl;       // 0
 * std::cout << "Is full: " << limitedMap.full() << std::endl;           // false
 * std::cout << "Left space: " << limitedMap.left_size() << std::endl;   // 100
 * 
 * // 启用LRU以处理容量满的情况
 * limitedMap.enable_lru();
 * 
 * // 批量插入超过容量的数据
 * for (int i = 0; i < 150; ++i) {
 *     limitedMap[i] = "value" + std::to_string(i);
 *     
 *     if (i % 20 == 0) {  // 每20个打印一次状态
 *         std::cout << "Inserted " << (i + 1) << " items, "
 *                   << "size: " << limitedMap.size() 
 *                   << ", full: " << limitedMap.full() << std::endl;
 *     }
 * }
 * 
 * // 检查最终状态
 * std::cout << "Final size: " << limitedMap.size() << std::endl;         // 100
 * std::cout << "Is full: " << limitedMap.full() << std::endl;           // true
 * std::cout << "Left space: " << limitedMap.left_size() << std::endl;   // 0
 * 
 * // 验证LRU行为：最后插入的150个元素中的后100个应该存在
 * int exists_count = 0;
 * for (int i = 50; i < 150; ++i) {  // 检查后100个
 *     if (limitedMap.count(i) > 0) {
 *         exists_count++;
 *     }
 * }
 * std::cout << "Elements [50-149] still exist: " << exists_count << "/100" << std::endl;
 * ```
 * 
 * @subsection shared_memory_usage 共享内存使用
 * 
 * ```cpp
 * // 进程A：创建共享内存映射
 * NFShmHashMapWithList<int, std::string, 1000> sharedMap;
 * if (sharedMap.CreateInit() == 0) {  // 创建成功
 *     sharedMap.enable_lru();  // 启用LRU
 *     sharedMap[1] = "Process A data";
 *     sharedMap[2] = "Shared data";
 *     sharedMap[3] = "Cache entry";
 *     std::cout << "Created shared map with " << sharedMap.size() << " items" << std::endl;
 * }
 * 
 * // 进程B：恢复已存在的共享内存映射
 * NFShmHashMapWithList<int, std::string, 1000> restoredMap;
 * if (restoredMap.ResumeInit() == 0) {  // 恢复成功
 *     std::cout << "Restored map with " << restoredMap.size() << " items" << std::endl;
 *     std::cout << "LRU enabled: " << restoredMap.is_lru_enabled() << std::endl;
 *     
 *     // 访问进程A创建的数据（影响LRU顺序）
 *     auto it = restoredMap.find(1);
 *     if (it != restoredMap.end()) {
 *         std::cout << "Found data from Process A: " << it->second << std::endl;
 *     }
 *     
 *     // 添加进程B的数据
 *     restoredMap[4] = "Process B data";
 *     
 *     // 查看LRU顺序
 *     std::cout << "Current LRU order:" << std::endl;
 *     for (auto it = restoredMap.list_begin(); it != restoredMap.list_end(); ++it) {
 *         std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
 *     }
 * }
 * ```
 * 
 * @subsection stl_interop STL容器互操作
 * 
 * ```cpp
 * // 从STL容器构造
 * std::unordered_map<std::string, int> stdMap = {
 *     {"apple", 100}, {"banana", 200}, {"orange", 300}
 * };
 * 
 * NFShmHashMapWithList<std::string, int, 1000> shmMap(stdMap);  // 从STL构造
 * shmMap.enable_lru();
 * 
 * // 转换回STL容器（保持链表顺序）
 * std::vector<std::pair<std::string, int>> orderedData;
 * for (auto it = shmMap.list_begin(); it != shmMap.list_end(); ++it) {
 *     orderedData.emplace_back(it->first, it->second);
 * }
 * 
 * // 使用STL算法
 * auto count = std::count_if(shmMap.begin(), shmMap.end(),
 *     [](const auto& pair) { return pair.second > 150; });
 * std::cout << "Values > 150: " << count << std::endl;
 * 
 * // 基于访问频率的操作
 * shmMap["apple"];   // 访问apple，移到LRU头部
 * shmMap["banana"];  // 访问banana，移到LRU头部
 * 
 * // 获取最近访问的前N个元素
 * std::vector<std::string> recentItems;
 * int n = 2;
 * auto it = shmMap.list_begin();
 * for (int i = 0; i < n && it != shmMap.list_end(); ++i, ++it) {
 *     recentItems.push_back(it->first);
 * }
 * 
 * std::cout << "Most recent " << n << " items: ";
 * for (const auto& item : recentItems) {
 *     std::cout << item << " ";
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @section performance_notes 性能说明
 * 
 * - **查找性能**：O(1)平均，O(n)最坏（链表长度）
 * - **插入性能**：O(1)平均，包含链表维护开销
 * - **删除性能**：O(1)平均，需更新链表结构
 * - **LRU操作**：O(1)时间复杂度的链表头尾操作
 * - **顺序遍历**：O(n)链表遍历，缓存友好
 * - **内存性能**：零碎片，预分配，额外链表节点开销
 * - **并发性能**：需要外部同步，但支持多进程读写
 * 
 * @section migration_guide 迁移指南
 * 
 * 从std::unordered_map迁移到NFShmHashMapWithList：
 * 
 * 1. **替换类型声明**：
 *    ```cpp
 *    // 原代码
 *    std::unordered_map<int, std::string> map;
 *    
 *    // 新代码（添加容量模板参数）
 *    NFShmHashMapWithList<int, std::string, 10000> map;
 *    ```
 * 
 * 2. **添加初始化**：
 *    ```cpp
 *    // 添加共享内存初始化
 *    map.CreateInit();  // 或 ResumeInit()
 *    ```
 * 
 * 3. **利用链表特性**：
 *    ```cpp
 *    // 启用LRU缓存语义
 *    map.enable_lru();
 *    
 *    // 使用链表顺序遍历
 *    for (auto it = map.list_begin(); it != map.list_end(); ++it) {
 *         // 按访问顺序处理元素
 *    }
 *    ```
 * 
 * 4. **处理容量限制**：
 *    ```cpp
 *    // 利用LRU自动淘汰机制
 *    if (map.full() && map.is_lru_enabled()) {
 *         // 插入会自动淘汰最久未使用的元素
 *         map[new_key] = new_value;
 *    }
 *    ```
 * 
 * 5. **移除动态管理**：
 *    ```cpp
 *    // 移除这些调用（NFShmHashMapWithList不支持）
 *    // map.rehash(1000);
 *    // map.reserve(500);
 *    // map.max_load_factor(0.8);
 *    ```
 * 
 * 6. **充分利用新特性**：
 *    ```cpp
 *    // 检查LRU状态
 *    if (map.is_lru_enabled()) {
 *         // 基于访问模式的逻辑
 *    }
 *    
 *    // 动态控制LRU行为
 *    map.disable_lru();  // 禁用LRU，仅维护访问顺序
 *    map.enable_lru();   // 重新启用LRU淘汰
 *    ```
 */

/****************************************************************************
 * STL std::unordered_map 对比分析（WithList增强版）
 ****************************************************************************
 * 
 * 1. 内存管理策略对比：
 *    - std::unordered_map: 动态内存分配，使用allocator管理堆内存
 *    - NFShmHashMapWithList: 固定大小共享内存，预分配所有节点，支持进程间共享
 * 
 * 2. 容量管理对比：
 *    - std::unordered_map: 动态扩容，load_factor超过阈值时自动rehash
 *    - NFShmHashMapWithList: 固定容量MAX_SIZE，通过LRU机制处理容量满的情况
 * 
 * 3. 访问顺序管理对比：
 *    - std::unordered_map: 无访问顺序概念，遍历顺序依赖哈希桶
 *    - NFShmHashMapWithList: 内置双向链表维护访问/插入顺序，支持LRU语义
 * 
 * 4. 键值对存储对比：
 *    - std::unordered_map: std::pair<const Key, T>
 *    - NFShmHashMapWithList: NFShmPair<Key, T>（适配共享内存环境）
 * 
 * 5. 缓存语义对比：
 *    - std::unordered_map: 无缓存概念，需要手动实现LRU逻辑
 *    - NFShmHashMapWithList: 原生LRU支持，自动淘汰最久未使用元素
 * 
 * 6. 性能特征对比：
 *    - std::unordered_map: O(1)平均时间复杂度，但可能触发rehash开销
 *    - NFShmHashMapWithList: O(1)平均时间复杂度，额外O(1)链表维护开销，无rehash开销
 * 
 * 7. API兼容性：
 *    - 兼容接口：insert, find, erase, operator[], at, begin, end, size, empty等
 *    - 扩展接口：list_begin, list_end, enable_lru, disable_lru, is_lru_enabled等
 *    - 特有接口：full(), left_size(), CreateInit(), ResumeInit()等共享内存特有功能
 *    - 缺少接口：rehash, reserve, bucket, load_factor等动态管理接口
 * 
 * 8. 使用场景对比：
 *    - std::unordered_map: 通用键值映射，注重查找性能
 *    - NFShmHashMapWithList: 缓存系统、访问模式跟踪、LRU淘汰策略、共享内存应用
 *****************************************************************************/

// ==================== 前向声明 ====================

template <class Key, class Tp, int MAX_SIZE,
          class HashFcn = std::hash<Key>,
          class EqualKey = std::equal_to<Key>>
class NFShmHashMapWithList;

template <class Key, class Tp, int MAX_SIZE, class HashFn, class EqKey>
bool operator==(const NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFn, EqKey>&,
                const NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFn, EqKey>&);

// ==================== 主要容器类 ====================

/**
 * @brief 基于共享内存的无序映射容器
 * @tparam Key 键类型
 * @tparam Tp 值类型（映射的目标类型）
 * @tparam MAX_SIZE 最大容量（固定）
 * @tparam HashFcn 哈希函数类型，默认std::hash<Key>
 * @tparam EqualKey 键比较函数类型，默认std::equal_to<Key>
 *
 * 设计特点：
 * 1. 固定容量，不支持动态扩容（与STL主要区别）
 * 2. 基于共享内存，支持进程间共享
 * 3. API设计尽量兼容STL std::unordered_map
 * 4. 使用NFShmPair代替std::pair适配共享内存环境
 *
 * 与std::unordered_map的主要差异：
 * - 容量限制：固定大小 vs 动态扩容
 * - 内存管理：共享内存 vs 堆内存
 * - 性能特征：无rehash开销 vs 动态性能优化
 * - 进程支持：进程间共享 vs 单进程内使用
 */
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
class NFShmHashMapWithList
{
private:
    /// @brief 底层哈希表类型，使用NFShmPair存储键值对
    typedef NFShmHashTableWithList<NFShmPair<Key, Tp>, Key, MAX_SIZE, HashFcn, std::_Select1st<NFShmPair<Key, Tp>>, EqualKey> HashTable;
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

    typedef typename HashTable::list_iterator list_iterator; ///< 迭代器类型
    typedef typename HashTable::const_list_iterator const_list_iterator; ///< 常量迭代器类型

public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 默认构造函数
     * @note 根据SHM_CREATE_MODE决定创建或恢复模式
     * @note 与std::unordered_map()行为类似，但增加共享内存初始化
     */
    NFShmHashMapWithList()
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
     * @note 与std::unordered_map(first, last)兼容
     */
    template <class InputIterator>
    NFShmHashMapWithList(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 数组构造函数
     * @param f 起始指针
     * @param l 结束指针
     * @note 与std::unordered_map兼容
     */
    NFShmHashMapWithList(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    NFShmHashMapWithList(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始迭代器
     * @param l 结束迭代器
     */
    NFShmHashMapWithList(iterator f, iterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 拷贝构造函数
     * @param x 源NFShmHashMapWithList对象
     * @note 与std::unordered_map拷贝构造函数兼容
     */
    NFShmHashMapWithList(const NFShmHashMapWithList& x)
    {
        if (this != &x) m_hashTable = x.m_hashTable;
    }

    /**
     * @brief 从std::unordered_map构造
     * @param map 源std::unordered_map对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMapWithList(const std::unordered_map<Key, Tp>& map)
    {
        m_hashTable.insert_unique(map.begin(), map.end());
    }

    /**
     * @brief 从std::map构造
     * @param map 源std::map对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMapWithList(const std::map<Key, Tp>& map)
    {
        m_hashTable.insert_unique(map.begin(), map.end());
    }

    /**
     * @brief 初始化列表构造函数
     * @param list 初始化列表
     * @note 与std::unordered_map(std::initializer_list)兼容
     */
    NFShmHashMapWithList(const std::initializer_list<value_type>& list)
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
        new(this) NFShmHashMapWithList();
    }

    // ==================== 赋值操作符 ====================

    /**
     * @brief 拷贝赋值操作符
     * @param x 源NFShmHashMapWithList对象
     * @return 自身引用
     * @note 与std::unordered_map::operator=兼容
     */
    NFShmHashMapWithList<Key, Tp, MAX_SIZE>& operator=(const NFShmHashMapWithList<Key, Tp, MAX_SIZE>& x);

    /**
     * @brief 从std::unordered_map赋值
     * @param x 源std::unordered_map对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMapWithList<Key, Tp, MAX_SIZE>& operator=(const std::unordered_map<Key, Tp>& x);

    /**
     * @brief 从std::map赋值
     * @param x 源std::map对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMapWithList<Key, Tp, MAX_SIZE>& operator=(const std::map<Key, Tp>& x);

    /**
     * @brief 从初始化列表赋值
     * @param x 初始化列表
     * @return 自身引用
     * @note 与std::unordered_map::operator=(std::initializer_list)兼容
     */
    NFShmHashMapWithList<Key, Tp, MAX_SIZE>& operator=(const std::initializer_list<value_type>& x);

public:
    // ==================== 容量相关接口（STL兼容） ====================

    /**
     * @brief 获取当前元素数量
     * @return 元素数量
     * @note 与std::unordered_map::size()兼容
     */
    size_type size() const { return m_hashTable.size(); }

    /**
     * @brief 获取最大容量
     * @return 最大容量MAX_SIZE
     * @note 与std::unordered_map::max_size()不同，返回固定值而非理论最大值
     */
    size_type max_size() const { return m_hashTable.max_size(); }

    /**
     * @brief 判断是否为空
     * @return true表示空
     * @note 与std::unordered_map::empty()兼容
     */
    bool empty() const { return m_hashTable.empty(); }

    /**
     * @brief 交换两个容器的内容
     * @param hs 另一个NFShmHashMapWithList对象
     * @note 与std::unordered_map::swap()兼容
     */
    void swap(NFShmHashMapWithList& hs) noexcept { m_hashTable.swap(hs.m_hashTable); }

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
    friend bool operator==(const NFShmHashMapWithList<K1, T1, O_MAX_SIZE, Hf, EqK>&,
                           const NFShmHashMapWithList<K1, T1, O_MAX_SIZE, Hf, EqK>&);

    // ==================== STL兼容迭代器接口 ====================

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     * @note 与std::unordered_map::begin()兼容
     */
    iterator begin() { return m_hashTable.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     * @note 与std::unordered_map::end()兼容
     */
    iterator end() { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_map::begin() const兼容
     */
    const_iterator begin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_map::end() const兼容
     */
    const_iterator end() const { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_map::cbegin()兼容
     */
    const_iterator cbegin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_map::cend()兼容
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
     * @brief 插入键值对
     * @param obj 要插入的键值对
     * @return pair<iterator, bool>，迭代器指向元素，bool表示是否插入成功
     * @note 与std::unordered_map::insert()兼容
     */
    std::pair<iterator, bool> insert(const value_type& obj)
    {
        return m_hashTable.insert_unique(obj);
    }

    /**
     * @brief 带提示插入键值对
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param obj 要插入的键值对
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_map::insert(const_iterator, const value_type&)兼容
     */
    iterator insert(const_iterator hint, const value_type& obj)
    {
        (void)hint; // 忽略提示参数
        return m_hashTable.insert_unique(obj).first;
    }

    /**
     * @brief 就地构造元素
     * @tparam Args 构造参数类型包
     * @param args 构造参数
     * @return pair<iterator, bool>
     * @note 与std::unordered_map::emplace()兼容
     */
    template <typename... Args>
    std::pair<iterator, bool> emplace(const Args&... args);

    /**
     * @brief 带提示就地构造元素
     * @tparam Args 构造参数类型包
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_map::emplace_hint()兼容
     */
    template <typename... Args>
    iterator emplace_hint(const_iterator hint, const Args&... args);

    /**
     * @brief 范围插入
     * @tparam InputIterator 输入迭代器类型
     * @param f 起始迭代器
     * @param l 结束迭代器
     * @note 与std::unordered_map::insert(first, last)兼容
     */
    template <class InputIterator>
    void insert(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 数组范围插入
     * @param f 起始指针
     * @param l 结束指针
     */
    void insert(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 迭代器范围插入
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    void insert(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 初始化列表插入
     * @param x 初始化列表
     * @note 与std::unordered_map::insert(std::initializer_list)兼容
     */
    void insert(const std::initializer_list<value_type>& x)
    {
        m_hashTable.insert_unique(x.begin(), x.end());
    }

    /**
     * @brief 无扩容插入（内部接口）
     * @param obj 要插入的键值对
     * @return pair<iterator, bool>
     * @note STL容器没有此接口，用于性能优化
     */
    std::pair<iterator, bool> insert_noresize(const value_type& obj)
    {
        return m_hashTable.insert_unique_noresize(obj);
    }

    // ==================== 查找接口（STL兼容） ====================

    /**
     * @brief 查找元素
     * @param key 要查找的键
     * @return 指向元素的迭代器，未找到返回end()
     * @note 与std::unordered_map::find()兼容
     */
    iterator find(const key_type& key)
    {
        return m_hashTable.find(key);
    }

    /**
     * @brief 查找元素（常量版本）
     * @param key 要查找的键
     * @return 指向元素的常量迭代器
     * @note 与std::unordered_map::find() const兼容
     */
    const_iterator find(const key_type& key) const
    {
        return m_hashTable.find(key);
    }

    /**
     * @brief 下标访问操作符
     * @param key 键
     * @return 对应值的引用，不存在则插入默认值
     * @note 与std::unordered_map::operator[]兼容
     */
    Tp& operator[](const key_type& key)
    {
        return m_hashTable.find_or_insert(value_type(key, Tp())).second;
    }

    /**
     * @brief 安全访问元素
     * @param key 键
     * @return 对应值的引用
     * @throw 如果键不存在会记录错误日志并返回静态错误对象
     * @note 与std::unordered_map::at()类似，但异常处理不同
     */
    Tp& at(const key_type& key) { return m_hashTable.at(key).second; }

    /**
     * @brief 安全访问元素（常量版本）
     * @param key 键
     * @return 对应值的常量引用
     * @note 与std::unordered_map::at() const类似
     */
    const Tp& at(const key_type& key) const { return m_hashTable.at(key).second; }

    /**
     * @brief 统计指定键的元素数量
     * @param key 要统计的键
     * @return 元素数量（0或1，因为是unique容器）
     * @note 与std::unordered_map::count()兼容
     */
    size_type count(const key_type& key) const
    {
        return m_hashTable.count(key);
    }

    /**
     * @brief 获取指定键的元素范围
     * @param key 要查找的键
     * @return pair<iterator, iterator>表示范围
     * @note 与std::unordered_map::equal_range()兼容
     */
    std::pair<iterator, iterator> equal_range(const key_type& key)
    {
        return m_hashTable.equal_range(key);
    }

    /**
     * @brief 获取指定键的元素范围（常量版本）
     * @param key 要查找的键
     * @return pair<const_iterator, const_iterator>表示范围
     * @note 与std::unordered_map::equal_range() const兼容
     */
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const
    {
        return m_hashTable.equal_range(key);
    }

    // ==================== 删除接口（STL兼容） ====================

    /**
     * @brief 根据键删除元素
     * @param key 要删除的键
     * @return 删除的元素数量
     * @note 与std::unordered_map::erase()兼容
     */
    size_type erase(const key_type& key)
    {
        return m_hashTable.erase(key);
    }

    /**
     * @brief 根据迭代器删除元素
     * @param it 指向要删除元素的迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_map::erase()兼容
     */
    iterator erase(iterator it)
    {
        return erase(const_iterator(it));
    }

    /**
     * @brief 根据常量迭代器删除元素
     * @param it 指向要删除元素的常量迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_map::erase()兼容
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
     * @note 与std::unordered_map::erase(first, last)兼容
     */
    iterator erase(const_iterator f, const_iterator l)
    {
        return m_hashTable.erase(f, l);
    }

    /**
     * @brief 清空所有元素
     * @note 与std::unordered_map::clear()兼容
     */
    void clear()
    {
        m_hashTable.clear();
    }

    // ==================== 桶接口（STL兼容） ====================

    /**
     * @brief 调整大小提示
     * @param hint 元素数量提示
     * @note 与std::unordered_map::rehash()类似但实际不执行操作（固定容量）
     */
    void resize(size_type hint)
    {
        m_hashTable.resize(hint);
    }

    /**
     * @brief 获取桶数量
     * @return 桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_map::bucket_count()兼容，但返回固定值
     */
    size_type bucket_count() const
    {
        return m_hashTable.bucket_count();
    }

    /**
     * @brief 获取最大桶数量
     * @return 最大桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_map::max_bucket_count()兼容
     */
    size_type max_bucket_count() const
    {
        return m_hashTable.max_bucket_count();
    }

    /**
     * @brief 获取指定桶中的元素数量
     * @param n 桶索引
     * @return 该桶中的元素数量
     * @note 与std::unordered_map::bucket_size()类似
     */
    size_type elems_in_bucket(size_type n) const
    {
        return m_hashTable.elems_in_bucket(n);
    }
};

// ==================== 实现部分 ====================

/**
 * @brief 从std::map赋值实现
 * @param x 源std::map对象
 * @return 自身引用
 * @note STL容器没有此接口，为方便从有序容器转换而提供
 */
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::map<Key, Tp>& x)
{
    clear();
    m_hashTable.insert_unique(x.begin(), x.end());
    return *this;
}

/**
 * @brief 从std::unordered_map赋值实现
 * @param x 源std::unordered_map对象
 * @return 自身引用
 * @note STL容器没有此接口，为方便从STL无序容器转换而提供
 */
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::unordered_map<Key, Tp>& x)
{
    clear();
    m_hashTable.insert_unique(x.begin(), x.end());
    return *this;
}

/**
 * @brief 从初始化列表赋值实现
 * @param x 初始化列表
 * @return 自身引用
 * @note 与std::unordered_map::operator=(std::initializer_list)兼容
 */
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::initializer_list<value_type>& x)
{
    clear();
    m_hashTable.insert_unique(x.begin(), x.end());
    return *this;
}

/**
 * @brief 就地构造元素实现
 * @tparam Args 构造参数类型包
 * @param args 构造参数
 * @return pair<iterator, bool>，迭代器指向元素，bool表示是否插入成功
 * @note 与std::unordered_map::emplace()兼容
 */
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename... Args>
std::pair<typename NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::iterator, bool>
NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::emplace(const Args&... args)
{
    value_type obj(args...);
    return m_hashTable.insert_unique(obj);
}

/**
 * @brief 带提示就地构造元素实现
 * @tparam Args 构造参数类型包
 * @param hint 位置提示迭代器（忽略）
 * @param args 构造参数
 * @return 指向插入元素的迭代器
 * @note 与std::unordered_map::emplace_hint()兼容，但忽略提示参数
 */
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename... Args>
typename NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::iterator
NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::emplace_hint(const_iterator hint, const Args&... args)
{
    (void)hint; // 忽略提示参数
    value_type obj(args...);
    return m_hashTable.insert_unique(obj).first;
}

/**
 * @brief 拷贝赋值操作符实现
 * @param x 源NFShmHashMapWithList对象
 * @return 自身引用
 * @note 与std::unordered_map::operator=兼容
 */
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const NFShmHashMapWithList<Key, Tp, MAX_SIZE>& x)
{
    if (this != &x)
    {
        clear();
        m_hashTable = x.m_hashTable;
    }
    return *this;
}

// ==================== 全局操作符 ====================

/**
 * @brief 相等比较操作符
 * @param hm1 第一个NFShmHashMapWithList对象
 * @param hm2 第二个NFShmHashMapWithList对象
 * @return true表示相等
 * @note 与std::unordered_map的operator==兼容
 * @note 比较所有键值对是否相等，顺序无关
 */
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqlKey>
bool operator==(const NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqlKey>& hm1,
                const NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqlKey>& hm2)
{
    return hm1.m_hashTable == hm2.m_hashTable;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqlKey>
void swap(const NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqlKey>& hm1,
          const NFShmHashMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqlKey>& hm2)
{
    hm1.m_hashTable.swap(hm2.m_hashTable);
}
