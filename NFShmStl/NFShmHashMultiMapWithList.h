// -------------------------------------------------------------------------
//    @FileName         :    NFShmHashMultiMapWithList.h
//    @Author           :    gaoyi
//    @Date             :    23-2-11
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmHashMultiMapWithList
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmHashTableWithList.h"
#include <map>
#include <unordered_map>
#include "NFShmPair.h"

/**
 * @file NFShmHashMultiMapWithList.h
 * @brief 基于共享内存的无序多重映射容器实现（链表增强版），与STL std::unordered_multimap高度兼容
 * 
 * @section overview 概述
 * 
 * NFShmHashMultiMapWithList 是一个专为共享内存环境设计的多重键值映射容器，在NFShmHashMultiMap
 * 基础上增加了双向链表支持，提供LRU（最近最少使用）缓存语义和顺序遍历功能。它允许相同键对应
 * 多个值，同时支持基于访问模式的自动淘汰机制。在API设计上与STL std::unordered_multimap高度兼容，
 * 但在内存管理、容量控制和访问模式方面针对共享内存场景进行了深度优化。
 * 
 * @section features 核心特性
 * 
 * 1. **多重映射支持**：
 *    - 允许相同键存储多个不同值
 *    - 相同键的元素在迭代时保持相邻性
 *    - equal_range()提供相同键的元素范围
 *    - count()返回指定键的元素数量
 * 
 * 2. **双向链表增强**：
 *    - 内建双向链表，维护元素访问/插入顺序
 *    - list_begin()、list_end()提供顺序遍历
 *    - LRU缓存语义，自动淘汰最久未使用元素
 *    - enable_lru()/disable_lru()动态控制LRU行为
 * 
 * 3. **STL高度兼容**：
 *    - 完整的std::unordered_multimap API支持
 *    - 标准的迭代器接口和类型定义
 *    - find()、count()、equal_range()等常用操作
 *    - 支持范围for循环和STL算法
 * 
 * 4. **共享内存优化**：
 *    - 固定大小内存布局，避免动态分配
 *    - 基于NFShmPair的键值对存储
 *    - 支持CREATE/RESUME两阶段初始化
 *    - 无内存碎片，高效的内存使用
 * 
 * 5. **缓存友好设计**：
 *    - O(1)平均时间复杂度的哈希操作
 *    - 链表维护访问顺序，适合LRU淘汰策略
 *    - 预分配节点池，快速内存分配
 *    - 固定桶数量，无rehash开销
 * 
 * @section stl_comparison 与STL std::unordered_multimap对比
 * 
 * | 特性 | std::unordered_multimap | NFShmHashMultiMapWithList |
 * |------|------------------------|--------------------------|
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
 * // 定义容量为1000的字符串到整数多重映射（带链表）
 * NFShmHashMultiMapWithList<std::string, int, 1000> scoreCacheMap;
 * scoreCacheMap.CreateInit();  // 创建模式初始化
 * 
 * // 多重映射：同一个学生可以有多个成绩
 * scoreCacheMap.insert({"alice", 90});
 * scoreCacheMap.insert({"alice", 85});
 * scoreCacheMap.insert({"alice", 95});
 * scoreCacheMap.insert({"bob", 80});
 * scoreCacheMap.insert({"bob", 88});
 * 
 * // 查找操作
 * auto it = scoreCacheMap.find("alice");
 * if (it != scoreCacheMap.end()) {
 *     std::cout << "Found alice's first score: " << it->second << std::endl;
 * }
 * 
 * // 统计某个键的元素数量
 * std::cout << "Alice has " << scoreCacheMap.count("alice") << " scores" << std::endl;  // 输出：3
 * 
 * // 获取某个键的所有值
 * auto range = scoreCacheMap.equal_range("alice");
 * std::cout << "Alice's all scores: ";
 * for (auto it = range.first; it != range.second; ++it) {
 *     std::cout << it->second << " ";
 * }
 * std::cout << std::endl;
 * 
 * // 哈希遍历（无序）
 * std::cout << "Hash order traversal:" << std::endl;
 * for (const auto& pair : scoreCacheMap) {
 *     std::cout << pair.first << ": " << pair.second << std::endl;
 * }
 * 
 * // 链表遍历（按访问/插入顺序）
 * std::cout << "List order traversal:" << std::endl;
 * for (auto it = scoreCacheMap.list_begin(); it != scoreCacheMap.list_end(); ++it) {
 *     std::cout << it->first << ": " << it->second << std::endl;
 * }
 * ```
 * 
 * @subsection lru_multimap_cache LRU多重映射缓存
 * 
 * ```cpp
 * // 构建用户标签系统的LRU缓存
 * NFShmHashMultiMapWithList<int, std::string, 200> userTagsCache;
 * userTagsCache.CreateInit();
 * userTagsCache.enable_lru();  // 启用LRU
 * 
 * // 为用户添加多个标签
 * userTagsCache.insert({1001, "VIP"});
 * userTagsCache.insert({1001, "Gaming"});
 * userTagsCache.insert({1001, "Premium"});
 * 
 * userTagsCache.insert({1002, "Student"});
 * userTagsCache.insert({1002, "Music"});
 * 
 * userTagsCache.insert({1003, "Business"});
 * userTagsCache.insert({1003, "Travel"});
 * userTagsCache.insert({1003, "VIP"});
 * 
 * // 批量添加更多用户标签直到缓存满
 * for (int userId = 1004; userId <= 1100; ++userId) {
 *     userTagsCache.insert({userId, "Basic"});
 *     userTagsCache.insert({userId, "Standard"});
 * }
 * 
 * std::cout << "Cache size: " << userTagsCache.size() << std::endl;
 * std::cout << "Cache full: " << userTagsCache.full() << std::endl;
 * 
 * // 访问早期用户（会影响LRU顺序）
 * auto user1001_tags = userTagsCache.equal_range(1001);
 * std::cout << "User 1001 tags: ";
 * for (auto it = user1001_tags.first; it != user1001_tags.second; ++it) {
 *     std::cout << it->second << " ";
 * }
 * std::cout << std::endl;
 * 
 * // 继续添加新用户（会淘汰最久未访问的用户标签）
 * for (int userId = 1101; userId <= 1110; ++userId) {
 *     userTagsCache.insert({userId, "New"});
 *     userTagsCache.insert({userId, "Active"});
 * }
 * 
 * // 检查早期用户是否还存在
 * std::cout << "User 1001 tag count: " << userTagsCache.count(1001) << std::endl;  // 应该还存在
 * std::cout << "User 1004 tag count: " << userTagsCache.count(1004) << std::endl;  // 可能被淘汰
 * 
 * // 查看LRU顺序的前几个用户
 * std::cout << "Most recent users:" << std::endl;
 * int count = 0;
 * int lastUserId = -1;
 * for (auto it = userTagsCache.list_begin(); it != userTagsCache.list_end() && count < 10; ++it) {
 *     if (it->first != lastUserId) {
 *         std::cout << "User " << it->first << std::endl;
 *         lastUserId = it->first;
 *         count++;
 *     }
 * }
 * ```
 * 
 * @subsection multi_value_operations 多值操作和频率统计
 * 
 * ```cpp
 * NFShmHashMultiMapWithList<std::string, std::string, 500> categoryItemsMap;
 * categoryItemsMap.disable_lru();  // 仅跟踪访问顺序
 * 
 * // 构建类别到项目的多重映射
 * categoryItemsMap.insert({"fruit", "apple"});
 * categoryItemsMap.insert({"fruit", "banana"});
 * categoryItemsMap.insert({"fruit", "orange"});
 * categoryItemsMap.insert({"vegetable", "carrot"});
 * categoryItemsMap.insert({"vegetable", "lettuce"});
 * categoryItemsMap.insert({"dairy", "milk"});
 * categoryItemsMap.insert({"dairy", "cheese"});
 * 
 * // 模拟用户访问某些类别（影响链表顺序）
 * auto fruitRange = categoryItemsMap.equal_range("fruit");
 * std::cout << "Accessing fruit category:" << std::endl;
 * for (auto it = fruitRange.first; it != fruitRange.second; ++it) {
 *     std::cout << "  " << it->second << std::endl;
 * }
 * 
 * auto dairyRange = categoryItemsMap.equal_range("dairy");
 * std::cout << "Accessing dairy category:" << std::endl;
 * for (auto it = dairyRange.first; it != dairyRange.second; ++it) {
 *     std::cout << "  " << it->second << std::endl;
 * }
 * 
 * // 添加新类别
 * categoryItemsMap.insert({"meat", "beef"});
 * categoryItemsMap.insert({"meat", "chicken"});
 * 
 * // 按访问顺序查看最近访问的项目
 * std::cout << "Recent access order:" << std::endl;
 * int itemCount = 0;
 * for (auto it = categoryItemsMap.list_begin(); 
 *          it != categoryItemsMap.list_end() && itemCount < 8; 
 *          ++it, ++itemCount) {
 *     std::cout << "Category: " << it->first << ", Item: " << it->second << std::endl;
 * }
 * 
 * // 统计每个类别的项目数量
 * std::map<std::string, int> categoryCount;
 * for (const auto& pair : categoryItemsMap) {
 *     categoryCount[pair.first]++;
 * }
 * 
 * std::cout << "Category statistics:" << std::endl;
 * for (const auto& [category, count] : categoryCount) {
 *     std::cout << category << ": " << count << " items" << std::endl;
 * }
 * ```
 * 
 * @subsection capacity_management 容量管理和LRU策略
 * 
 * ```cpp
 * NFShmHashMultiMapWithList<int, std::string, 100> limitedMultiMap;
 * 
 * // 容量检查（STL没有的功能）
 * std::cout << "Max size: " << limitedMultiMap.max_size() << std::endl;      // 100
 * std::cout << "Current size: " << limitedMultiMap.size() << std::endl;       // 0
 * std::cout << "Is full: " << limitedMultiMap.full() << std::endl;           // false
 * std::cout << "Left space: " << limitedMultiMap.left_size() << std::endl;   // 100
 * 
 * // 启用LRU以处理容量满的情况
 * limitedMultiMap.enable_lru();
 * 
 * // 为每个键插入多个值
 * for (int key = 1; key <= 25; ++key) {
 *     for (int val = 1; val <= 4; ++val) {  // 每个键4个值
 *         limitedMultiMap.insert({key, "value_" + std::to_string(key) + "_" + std::to_string(val)});
 *     }
 * }
 * 
 * std::cout << "After initial insert - Size: " << limitedMultiMap.size() << std::endl;  // 100
 * std::cout << "Map is full: " << limitedMultiMap.full() << std::endl;  // true
 * 
 * // 访问一些早期的键（更新它们的LRU位置）
 * for (int key = 1; key <= 5; ++key) {
 *     auto range = limitedMultiMap.equal_range(key);
 *     std::cout << "Key " << key << " has " 
 *               << std::distance(range.first, range.second) << " values" << std::endl;
 * }
 * 
 * // 插入新的键值对（会触发LRU淘汰）
 * for (int key = 26; key <= 30; ++key) {
 *     for (int val = 1; val <= 4; ++val) {
 *         limitedMultiMap.insert({key, "new_value_" + std::to_string(key) + "_" + std::to_string(val)});
 *     }
 * }
 * 
 * std::cout << "After LRU eviction - Size: " << limitedMultiMap.size() << std::endl;  // 仍然100
 * 
 * // 检查哪些键被保留了
 * std::cout << "Keys remaining after LRU:" << std::endl;
 * std::set<int> remainingKeys;
 * for (const auto& pair : limitedMultiMap) {
 *     remainingKeys.insert(pair.first);
 * }
 * 
 * for (int key : remainingKeys) {
 *     std::cout << "Key " << key << " (count: " << limitedMultiMap.count(key) << ")" << std::endl;
 * }
 * ```
 * 
 * @subsection shared_memory_usage 共享内存使用
 * 
 * ```cpp
 * // 进程A：创建共享的多重映射
 * NFShmHashMultiMapWithList<int, std::string, 1000> sharedMultiMap;
 * if (sharedMultiMap.CreateInit() == 0) {  // 创建成功
 *     sharedMultiMap.enable_lru();  // 启用LRU
 *     
 *     // 添加一些多重映射数据
 *     sharedMultiMap.insert({1, "Process A - Item 1"});
 *     sharedMultiMap.insert({1, "Process A - Item 2"});
 *     sharedMultiMap.insert({2, "Process A - Data"});
 *     sharedMultiMap.insert({3, "Process A - Cache"});
 *     
 *     std::cout << "Created shared multimap with " << sharedMultiMap.size() << " items" << std::endl;
 * }
 * 
 * // 进程B：恢复已存在的共享内存多重映射
 * NFShmHashMultiMapWithList<int, std::string, 1000> restoredMultiMap;
 * if (restoredMultiMap.ResumeInit() == 0) {  // 恢复成功
 *     std::cout << "Restored multimap with " << restoredMultiMap.size() << " items" << std::endl;
 *     std::cout << "LRU enabled: " << restoredMultiMap.is_lru_enabled() << std::endl;
 *     
 *     // 访问进程A创建的数据（影响LRU顺序）
 *     auto count1 = restoredMultiMap.count(1);
 *     std::cout << "Key 1 has " << count1 << " values from Process A" << std::endl;
 *     
 *     auto range1 = restoredMultiMap.equal_range(1);
 *     std::cout << "Key 1 values: ";
 *     for (auto it = range1.first; it != range1.second; ++it) {
 *         std::cout << it->second << "; ";
 *     }
 *     std::cout << std::endl;
 *     
 *     // 添加进程B的数据
 *     restoredMultiMap.insert({1, "Process B - Additional"});
 *     restoredMultiMap.insert({4, "Process B - New Key"});
 *     restoredMultiMap.insert({4, "Process B - Another Value"});
 *     
 *     // 现在键1有3个值（2个来自进程A，1个来自进程B）
 *     std::cout << "Key 1 now has " << restoredMultiMap.count(1) << " values total" << std::endl;
 *     
 *     // 查看当前LRU顺序
 *     std::cout << "Current LRU order:" << std::endl;
 *     int itemCount = 0;
 *     for (auto it = restoredMultiMap.list_begin(); 
 *              it != restoredMultiMap.list_end() && itemCount < 8; 
 *              ++it, ++itemCount) {
 *         std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
 *     }
 * }
 * ```
 * 
 * @subsection stl_interop STL容器互操作
 * 
 * ```cpp
 * // 从STL容器构造
 * std::unordered_multimap<std::string, int> stdMultiMap = {
 *     {"math", 90}, {"math", 85}, {"english", 88}, {"english", 92}, {"math", 78}
 * };
 * 
 * NFShmHashMultiMapWithList<std::string, int, 1000> shmMultiMap(stdMultiMap);  // 从STL构造
 * shmMultiMap.enable_lru();
 * 
 * // 转换回STL容器（保持链表顺序）
 * std::vector<std::pair<std::string, int>> orderedData;
 * for (auto it = shmMultiMap.list_begin(); it != shmMultiMap.list_end(); ++it) {
 *     orderedData.emplace_back(it->first, it->second);
 * }
 * 
 * // 使用STL算法
 * auto mathCount = std::count_if(shmMultiMap.begin(), shmMultiMap.end(),
 *     [](const auto& pair) { return pair.first == "math"; });
 * std::cout << "Math entries: " << mathCount << std::endl;
 * 
 * // 访问某个学科（影响LRU顺序）
 * auto mathRange = shmMultiMap.equal_range("math");
 * std::cout << "Math scores: ";
 * for (auto it = mathRange.first; it != mathRange.second; ++it) {
 *     std::cout << it->second << " ";
 * }
 * std::cout << std::endl;
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
 * 
 * // 添加新数据
 * shmMultiMap.insert({"physics", 95});
 * shmMultiMap.insert({"physics", 87});
 * 
 * // 获取最近访问的前N个学科
 * std::set<std::string> recentSubjects;
 * int n = 3;
 * auto it = shmMultiMap.list_begin();
 * for (int i = 0; i < n * 3 && it != shmMultiMap.list_end(); ++it) {  // 每个学科可能有多个分数
 *     recentSubjects.insert(it->first);
 *     if (recentSubjects.size() >= n) break;
 * }
 * 
 * std::cout << "Most recent " << n << " subjects: ";
 * for (const auto& subject : recentSubjects) {
 *     std::cout << subject << " ";
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @section performance_notes 性能说明
 * 
 * - **查找性能**：O(1)平均，O(n)最坏（链表长度）
 * - **插入性能**：O(1)平均，总是成功（除非容量满），包含链表维护开销
 * - **删除性能**：O(1)平均，支持按键删除多个元素，需更新链表结构
 * - **范围查询**：equal_range() O(1)平均 + O(k)遍历（k为相同键元素数）
 * - **LRU操作**：O(1)时间复杂度的链表头尾操作
 * - **顺序遍历**：O(n)链表遍历，缓存友好
 * - **内存性能**：零碎片，预分配，额外链表节点开销
 * - **并发性能**：需要外部同步，但支持多进程读写
 * 
 * @section migration_guide 迁移指南
 * 
 * 从std::unordered_multimap迁移到NFShmHashMultiMapWithList：
 * 
 * 1. **替换类型声明**：
 *    ```cpp
 *    // 原代码
 *    std::unordered_multimap<int, std::string> multimap;
 *    
 *    // 新代码（添加容量模板参数）
 *    NFShmHashMultiMapWithList<int, std::string, 10000> multimap;
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
 * 4. **利用链表特性**：
 *    ```cpp
 *    // 启用LRU缓存语义
 *    multimap.enable_lru();
 *    
 *    // 使用链表顺序遍历
 *    for (auto it = multimap.list_begin(); it != multimap.list_end(); ++it) {
 *         // 按访问顺序处理元素
 *    }
 *    ```
 * 
 * 5. **处理容量限制**：
 *    ```cpp
 *    // 利用LRU自动淘汰机制
 *    if (multimap.full() && multimap.is_lru_enabled()) {
 *         // 插入会自动淘汰最久未使用的元素
 *         multimap.insert({key, value});
 *    }
 *    ```
 * 
 * 6. **移除不支持的操作**：
 *    ```cpp
 *    // 移除这些调用（NFShmHashMultiMapWithList不支持）
 *    // multimap[key] = value;  // operator[] 不支持
 *    // auto val = multimap.at(key);  // at() 不支持
 *    // multimap.rehash(1000);  // 动态管理不支持
 *    ```
 * 
 * 7. **充分利用新特性**：
 *    ```cpp
 *    // 检查LRU状态
 *    if (multimap.is_lru_enabled()) {
 *         // 基于访问模式的逻辑
 *    }
 *    
 *    // 动态控制LRU行为
 *    multimap.disable_lru();  // 禁用LRU，仅维护访问顺序
 *    multimap.enable_lru();   // 重新启用LRU淘汰
 *    ```
 */

/****************************************************************************
 * STL std::unordered_multimap 对比分析（WithList增强版）
 ****************************************************************************
 * 
 * 1. 内存管理策略对比：
 *    - std::unordered_multimap: 动态内存分配，使用allocator管理堆内存
 *    - NFShmHashMultiMapWithList: 固定大小共享内存，预分配所有节点，支持进程间共享
 * 
 * 2. 容量管理对比：
 *    - std::unordered_multimap: 动态扩容，load_factor超过阈值时自动rehash
 *    - NFShmHashMultiMapWithList: 固定容量MAX_SIZE，通过LRU机制处理容量满的情况
 * 
 * 3. 多重映射特性对比：
 *    - 都允许相同键的多个元素存在
 *    - 都在相同键的元素之间保持相邻性
 *    - insert操作都总是成功（除非容量不足）
 * 
 * 4. 访问顺序管理对比：
 *    - std::unordered_multimap: 无访问顺序概念，遍历顺序依赖哈希桶
 *    - NFShmHashMultiMapWithList: 内置双向链表维护访问/插入顺序，支持LRU语义
 * 
 * 5. 键值对存储对比：
 *    - std::unordered_multimap: std::pair<const Key, T>
 *    - NFShmHashMultiMapWithList: NFShmPair<Key, T>（适配共享内存环境）
 * 
 * 6. 缓存语义对比：
 *    - std::unordered_multimap: 无缓存概念，需要手动实现LRU逻辑
 *    - NFShmHashMultiMapWithList: 原生LRU支持，自动淘汰最久未使用元素
 * 
 * 7. 性能特征对比：
 *    - std::unordered_multimap: O(1)平均时间复杂度，但可能触发rehash开销
 *    - NFShmHashMultiMapWithList: O(1)平均时间复杂度，额外O(1)链表维护开销，无rehash开销
 * 
 * 8. API兼容性：
 *    - 兼容接口：insert, find, erase, begin, end, size, empty, count, equal_range等
 *    - 不同点：没有operator[]（多重映射特性决定）
 *    - 扩展接口：list_begin, list_end, enable_lru, disable_lru, is_lru_enabled等
 *    - 特有接口：full(), left_size(), CreateInit(), ResumeInit()等共享内存特有功能
 *    - 缺少接口：rehash, reserve, bucket, load_factor等动态管理接口
 * 
 * 9. 使用场景对比：
 *    - std::unordered_multimap: 通用多重键值映射，一对多关系存储
 *    - NFShmHashMultiMapWithList: 标签系统、分类缓存、访问日志、多值LRU缓存、共享内存应用
 *****************************************************************************/

// ==================== 前向声明 ====================

template <class Key, class Tp, int MAX_SIZE, class HashFcn = std::hash<Key>, class EqualKey = std::equal_to<Key>>
class NFShmHashMultiMapWithList;

template <class Key, class Tp, int MAX_SIZE, class Hf, class EqKey>
bool operator==(const NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, Hf, EqKey>& hm1,
                const NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, Hf, EqKey>& hm2);

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
class NFShmHashMultiMapWithList
{
private:
    /// @brief 底层哈希表类型，使用NFShmPair存储键值对，允许重复键
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
     * @note 与std::unordered_multimap()行为类似，但增加共享内存初始化
     */
    NFShmHashMultiMapWithList()
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
    NFShmHashMultiMapWithList(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 数组构造函数
     * @param f 起始指针
     * @param l 结束指针
     * @note 与std::unordered_multimap兼容
     */
    NFShmHashMultiMapWithList(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    NFShmHashMultiMapWithList(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始迭代器
     * @param l 结束迭代器
     */
    NFShmHashMultiMapWithList(iterator f, iterator l)
    {
        m_hashTable.insert_equal(f, l);
    }

    /**
     * @brief 从std::unordered_map构造
     * @param map 源std::unordered_map对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiMapWithList(const std::unordered_map<Key, Tp>& map)
    {
        m_hashTable.insert_equal(map.begin(), map.end());
    }

    /**
     * @brief 从std::unordered_multimap构造
     * @param map 源std::unordered_multimap对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiMapWithList(const std::unordered_multimap<Key, Tp>& map)
    {
        m_hashTable.insert_equal(map.begin(), map.end());
    }

    /**
     * @brief 从std::map构造
     * @param map 源std::map对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiMapWithList(const std::map<Key, Tp>& map)
    {
        m_hashTable.insert_equal(map.begin(), map.end());
    }

    /**
     * @brief 从std::multimap构造
     * @param map 源std::multimap对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashMultiMapWithList(const std::multimap<Key, Tp>& map)
    {
        m_hashTable.insert_equal(map.begin(), map.end());
    }

    /**
     * @brief 拷贝构造函数
     * @param x 源NFShmHashMultiMapWithList对象
     * @note 与std::unordered_multimap拷贝构造函数兼容
     */
    NFShmHashMultiMapWithList(const NFShmHashMultiMapWithList& x)
    {
        if (this != &x) m_hashTable = x.m_hashTable;
    }

    /**
     * @brief 初始化列表构造函数
     * @param list 初始化列表
     * @note 与std::unordered_multimap(std::initializer_list)兼容
     */
    NFShmHashMultiMapWithList(const std::initializer_list<value_type>& list)
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
        new(this) NFShmHashMultiMapWithList();
    }

    // ==================== 赋值操作符 ====================

    /**
     * @brief 拷贝赋值操作符
     * @param x 源NFShmHashMultiMapWithList对象
     * @return 自身引用
     * @note 与std::unordered_multimap::operator=兼容
     */
    NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& operator=(const NFShmHashMultiMapWithList& x);

    /**
     * @brief 从std::unordered_map赋值
     * @param x 源std::unordered_map对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& operator=(const std::unordered_map<Key, Tp>& x);

    /**
     * @brief 从std::map赋值
     * @param x 源std::map对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& operator=(const std::map<Key, Tp>& x);

    /**
     * @brief 从std::unordered_multimap赋值
     * @param x 源std::unordered_multimap对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& operator=(const std::unordered_multimap<Key, Tp>& x);

    /**
     * @brief 从std::multimap赋值
     * @param x 源std::multimap对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& operator=(const std::multimap<Key, Tp>& x);

    /**
     * @brief 从初始化列表赋值
     * @param x 初始化列表
     * @return 自身引用
     * @note 与std::unordered_multimap::operator=(std::initializer_list)兼容
     */
    NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& operator=(const std::initializer_list<value_type>& x);

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
     * @param hs 另一个NFShmHashMultiMapWithList对象
     * @note 与std::unordered_multimap::swap()兼容
     */
    void swap(NFShmHashMultiMapWithList& hs) noexcept { m_hashTable.swap(hs.m_hashTable); }

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
    friend bool operator==(const NFShmHashMultiMapWithList<K1, T1, O_MAX_SIZE, Hf, EqK>&,
                           const NFShmHashMultiMapWithList<K1, T1, O_MAX_SIZE, Hf, EqK>&);

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
 * @param hm1 第一个NFShmHashMultiMapWithList对象
 * @param hm2 第二个NFShmHashMultiMapWithList对象
 * @return true表示相等
 * @note 与std::unordered_multimap的operator==兼容
 * @note 比较所有键值对是否相等，顺序无关
 */
template <class Key, class Tp, int MAX_SIZE, class Hf, class EqKey>
bool operator==(const NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, Hf, EqKey>& hm1,
                const NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, Hf, EqKey>& hm2)
{
    return hm1.m_hashTable == hm2.m_hashTable;
}

template <class Key, class Tp, int MAX_SIZE, class Hf, class EqKey>
void swap(const NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, Hf, EqKey>& hm1,
                const NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, Hf, EqKey>& hm2)
{
    hm1.m_hashTable.swap(hm2.m_hashTable);
}

// ==================== 实现部分 ====================
template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const NFShmHashMultiMapWithList& x)
{
    if (this != &x)
    {
        clear();
        insert(x.begin(), x.end());
    }
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const unordered_map<Key, Tp>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const map<Key, Tp>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const unordered_multimap<Key, Tp>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const multimap<Key, Tp>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE>& NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::initializer_list<value_type>& x)
{
    clear();
    insert(x.begin(), x.end());
    return *this;
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename ... Args>
typename NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::iterator NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::emplace(const Args&... args)
{
    value_type obj(args...);
    return m_hashTable.insert_equal(obj);
}

template <class Key, class Tp, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename ... Args>
typename NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::iterator NFShmHashMultiMapWithList<Key, Tp, MAX_SIZE, HashFcn, EqualKey>::emplace_hint(const_iterator hint, const Args&... args)
{
    value_type obj(args...);
    return m_hashTable.insert_equal(obj);
}
