// -------------------------------------------------------------------------
//    @FileName         :    NFShmHashSetWithList.h
//    @Author           :    gaoyi
//    @Date             :    23-2-11
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmHashSetWithList
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmHashTableWithList.h"
#include <set>
#include <unordered_set>

/**
 * @file NFShmHashSetWithList.h
 * @brief 基于共享内存的无序集合容器实现（链表增强版），与STL std::unordered_set高度兼容
 * 
 * @section overview 概述
 * 
 * NFShmHashSetWithList 是一个专为共享内存环境设计的无序集合容器，在NFShmHashSet基础上
 * 增加了双向链表支持，提供LRU（最近最少使用）缓存语义和顺序遍历功能。它保证元素的唯一性，
 * 同时支持基于访问模式的自动淘汰机制。在API设计上与STL std::unordered_set高度兼容，
 * 但在内存管理、容量控制和访问模式方面针对共享内存场景进行了深度优化。
 * 
 * @section features 核心特性
 * 
 * 1. **集合语义**：
 *    - 保证元素唯一性，不允许重复元素
 *    - 插入重复元素时返回现有元素的迭代器
 *    - 支持高效的成员检查（contains语义）
 *    - 基于哈希的快速查找和插入
 * 
 * 2. **双向链表增强**：
 *    - 内建双向链表，维护元素访问/插入顺序
 *    - list_begin()、list_end()提供顺序遍历
 *    - LRU缓存语义，自动淘汰最久未使用元素
 *    - enable_lru()/disable_lru()动态控制LRU行为
 * 
 * 3. **STL高度兼容**：
 *    - 完整的std::unordered_set API支持
 *    - 标准的迭代器接口和类型定义
 *    - find()、count()、insert()等常用操作
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
 * @section stl_comparison 与STL std::unordered_set对比
 * 
 * | 特性 | std::unordered_set | NFShmHashSetWithList |
 * |------|-------------------|---------------------|
 * | **内存管理** | 动态堆内存分配 | 固定共享内存预分配 |
 * | **容量限制** | 无限制，动态扩容 | 固定容量MAX_SIZE |
 * | **扩容机制** | 自动rehash扩容 | **不支持扩容** |
 * | **元素存储** | 直接存储元素值 | 直接存储元素值 |
 * | **唯一性保证** | 完全保证 | 完全保证 |
 * | **插入语义** | 重复元素插入失败 | 重复元素插入失败 |
 * | **查找性能** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 |
 * | **成员检查** | find() != end() | find() != end() |
 * | **count操作** | 返回0或1 | 返回0或1 |
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
 * - **查找访问**：find(), count()
 * - **修改操作**：insert(), emplace(), erase(), clear()
 * - **容器操作**：swap()
 * - **桶接口**：bucket_count(), max_bucket_count(), elems_in_bucket()
 * 
 * **扩展的接口（新增）**：
 * - **容量检查**：full(), left_size()
 * - **共享内存**：CreateInit(), ResumeInit()
 * - **链表遍历**：list_begin(), list_end(), list_cbegin(), list_cend()
 * - **LRU控制**：enable_lru(), disable_lru(), is_lru_enabled()
 * - **STL转换**：从std::unordered_set和std::set构造
 * 
 * **不支持的接口**：
 * - **动态管理**：rehash(), reserve(), max_load_factor()
 * - **哈希策略**：load_factor(), bucket_size()
 * - **自定义分配器**：get_allocator()
 * 
 * @section usage_examples 使用示例
 * 
 * @subsection basic_usage 基础用法（类似std::unordered_set）
 * 
 * ```cpp
 * // 定义容量为1000的整数集合（带链表）
 * NFShmHashSetWithList<int, 1000> numberCacheSet;
 * numberCacheSet.CreateInit();  // 创建模式初始化
 * 
 * // 插入元素（保证唯一性）
 * auto result1 = numberCacheSet.insert(42);
 * std::cout << "Insert 42: " << (result1.second ? "success" : "already exists") << std::endl;
 * 
 * auto result2 = numberCacheSet.insert(42);  // 重复插入
 * std::cout << "Insert 42 again: " << (result2.second ? "success" : "already exists") << std::endl;
 * 
 * // 批量插入
 * std::vector<int> numbers = {1, 2, 3, 2, 4, 3, 5};  // 包含重复元素
 * numberCacheSet.insert(numbers.begin(), numbers.end());
 * 
 * std::cout << "Set size: " << numberCacheSet.size() << std::endl;  // 输出：6 (去重后)
 * 
 * // 成员检查
 * if (numberCacheSet.find(42) != numberCacheSet.end()) {
 *     std::cout << "42 is in the set" << std::endl;
 * }
 * 
 * // count操作（对于集合，只返回0或1）
 * std::cout << "Count of 42: " << numberCacheSet.count(42) << std::endl;  // 输出：1
 * std::cout << "Count of 99: " << numberCacheSet.count(99) << std::endl;  // 输出：0
 * 
 * // 哈希遍历（无序）
 * std::cout << "Hash order traversal:" << std::endl;
 * for (const auto& num : numberCacheSet) {
 *     std::cout << num << " ";
 * }
 * std::cout << std::endl;
 * 
 * // 链表遍历（按访问/插入顺序）
 * std::cout << "List order traversal:" << std::endl;
 * for (auto it = numberCacheSet.list_begin(); it != numberCacheSet.list_end(); ++it) {
 *     std::cout << *it << " ";
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @subsection lru_set_cache LRU集合缓存
 * 
 * ```cpp
 * // 构建热门文章ID的LRU缓存集合
 * NFShmHashSetWithList<int, 100> hotArticlesCache;
 * hotArticlesCache.CreateInit();
 * hotArticlesCache.enable_lru();  // 启用LRU
 * 
 * // 批量添加热门文章ID
 * for (int articleId = 1001; articleId <= 1100; ++articleId) {
 *     hotArticlesCache.insert(articleId);
 * }
 * 
 * std::cout << "Hot articles cache size: " << hotArticlesCache.size() << std::endl;  // 100
 * std::cout << "Cache is full: " << hotArticlesCache.full() << std::endl;  // true
 * 
 * // 模拟用户访问一些文章（会更新它们在LRU链表中的位置）
 * std::vector<int> accessedArticles = {1001, 1010, 1025, 1050, 1075};
 * for (int articleId : accessedArticles) {
 *     auto it = hotArticlesCache.find(articleId);
 *     if (it != hotArticlesCache.end()) {
 *         std::cout << "Accessed article " << articleId << std::endl;
 *         // find操作会自动更新LRU位置
 *     }
 * }
 * 
 * // 插入新的热门文章（会自动淘汰最久未访问的文章）
 * std::vector<int> newHotArticles = {2001, 2002, 2003, 2004, 2005};
 * for (int articleId : newHotArticles) {
 *     auto result = hotArticlesCache.insert(articleId);
 *     if (result.second) {
 *         std::cout << "Added new hot article " << articleId << std::endl;
 *     }
 * }
 * 
 * std::cout << "Cache size after LRU: " << hotArticlesCache.size() << std::endl;  // 仍然100
 * 
 * // 检查最近访问的文章仍然存在
 * for (int articleId : accessedArticles) {
 *     bool exists = hotArticlesCache.count(articleId) > 0;
 *     std::cout << "Article " << articleId << " still exists: " << exists << std::endl;
 * }
 * 
 * // 查看当前最热门的前10篇文章（按LRU顺序）
 * std::cout << "Top 10 hottest articles (by recent access):" << std::endl;
 * int count = 0;
 * for (auto it = hotArticlesCache.list_begin(); 
 *          it != hotArticlesCache.list_end() && count < 10; 
 *          ++it, ++count) {
 *     std::cout << "Article " << *it << std::endl;
 * }
 * ```
 * 
 * @subsection access_pattern_tracking 访问模式跟踪
 * 
 * ```cpp
 * // 构建用户访问页面的追踪集合
 * NFShmHashSetWithList<std::string, 200> visitedPagesSet;
 * visitedPagesSet.disable_lru();  // 仅跟踪访问顺序，不淘汰
 * 
 * // 模拟用户访问不同页面
 * std::vector<std::string> pageVisits = {
 *     "/home", "/login", "/profile", "/settings", 
 *     "/home", "/products", "/about", "/contact",
 *     "/profile", "/home", "/logout"
 * };
 * 
 * std::cout << "Tracking page visits:" << std::endl;
 * for (const std::string& page : pageVisits) {
 *     auto result = visitedPagesSet.insert(page);
 *     if (result.second) {
 *         std::cout << "First visit to: " << page << std::endl;
 *     } else {
 *         std::cout << "Revisited: " << page << " (moved to front of access list)" << std::endl;
 *     }
 * }
 * 
 * std::cout << "Total unique pages visited: " << visitedPagesSet.size() << std::endl;
 * 
 * // 按访问顺序查看页面（最后访问的在链表头部）
 * std::cout << "Pages in access order (most recent first):" << std::endl;
 * for (auto it = visitedPagesSet.list_begin(); it != visitedPagesSet.list_end(); ++it) {
 *     std::cout << *it << std::endl;
 * }
 * 
 * // 检查特定页面是否被访问过
 * std::vector<std::string> checkPages = {"/home", "/admin", "/help"};
 * for (const std::string& page : checkPages) {
 *     bool visited = visitedPagesSet.count(page) > 0;
 *     std::cout << "Page " << page << " visited: " << visited << std::endl;
 * }
 * 
 * // 获取最近访问的5个页面
 * std::cout << "5 most recently accessed pages:" << std::endl;
 * int pageCount = 0;
 * for (auto it = visitedPagesSet.list_begin(); 
 *          it != visitedPagesSet.list_end() && pageCount < 5; 
 *          ++it, ++pageCount) {
 *     std::cout << (pageCount + 1) << ". " << *it << std::endl;
 * }
 * ```
 * 
 * @subsection capacity_management 容量管理和去重
 * 
 * ```cpp
 * NFShmHashSetWithList<int, 50> limitedSet;
 * 
 * // 容量检查（STL没有的功能）
 * std::cout << "Max size: " << limitedSet.max_size() << std::endl;      // 50
 * std::cout << "Current size: " << limitedSet.size() << std::endl;       // 0
 * std::cout << "Is full: " << limitedSet.full() << std::endl;           // false
 * std::cout << "Left space: " << limitedSet.left_size() << std::endl;   // 50
 * 
 * // 启用LRU以处理容量满的情况
 * limitedSet.enable_lru();
 * 
 * // 模拟数据流，包含重复元素
 * std::vector<int> dataStream = {
 *     1, 2, 3, 4, 5, 1, 6, 7, 8, 9, 10,
 *     11, 12, 13, 14, 15, 2, 16, 17, 18, 19, 20,
 *     21, 22, 23, 24, 25, 3, 26, 27, 28, 29, 30,
 *     31, 32, 33, 34, 35, 4, 36, 37, 38, 39, 40,
 *     41, 42, 43, 44, 45, 5, 46, 47, 48, 49, 50,
 *     51, 52, 53, 54, 55, 1, 56, 57, 58, 59, 60
 * };
 * 
 * std::cout << "Processing data stream..." << std::endl;
 * int insertCount = 0, duplicateCount = 0, evictedCount = 0;
 * 
 * for (int value : dataStream) {
 *     auto result = limitedSet.insert(value);
 *     if (result.second) {
 *         insertCount++;
 *         if (limitedSet.size() > 50) {
 *             evictedCount++;
 *         }
 *     } else {
 *         duplicateCount++;
 *         std::cout << "Duplicate value " << value << " accessed, moved to front" << std::endl;
 *     }
 * }
 * 
 * std::cout << "Processing complete:" << std::endl;
 * std::cout << "Total insertions: " << insertCount << std::endl;
 * std::cout << "Duplicate accesses: " << duplicateCount << std::endl;
 * std::cout << "Final set size: " << limitedSet.size() << std::endl;
 * std::cout << "Set is full: " << limitedSet.full() << std::endl;
 * 
 * // 检查最近访问的元素是否还在集合中
 * std::vector<int> recentValues = {1, 2, 3, 4, 5};
 * std::cout << "Checking if recently accessed values are still in set:" << std::endl;
 * for (int value : recentValues) {
 *     bool exists = limitedSet.count(value) > 0;
 *     std::cout << "Value " << value << ": " << (exists ? "exists" : "evicted") << std::endl;
 * }
 * 
 * // 显示当前集合内容（按LRU顺序）
 * std::cout << "Current set contents (LRU order):" << std::endl;
 * int displayCount = 0;
 * for (auto it = limitedSet.list_begin(); 
 *          it != limitedSet.list_end() && displayCount < 20; 
 *          ++it, ++displayCount) {
 *     std::cout << *it << " ";
 *     if ((displayCount + 1) % 10 == 0) std::cout << std::endl;
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @subsection shared_memory_usage 共享内存使用
 * 
 * ```cpp
 * // 进程A：创建共享的集合
 * NFShmHashSetWithList<int, 1000> sharedSet;
 * if (sharedSet.CreateInit() == 0) {  // 创建成功
 *     sharedSet.enable_lru();  // 启用LRU
 *     
 *     // 添加一些数据
 *     std::vector<int> initialData = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
 *     sharedSet.insert(initialData.begin(), initialData.end());
 *     
 *     std::cout << "Created shared set with " << sharedSet.size() << " elements" << std::endl;
 * }
 * 
 * // 进程B：恢复已存在的共享内存集合
 * NFShmHashSetWithList<int, 1000> restoredSet;
 * if (restoredSet.ResumeInit() == 0) {  // 恢复成功
 *     std::cout << "Restored set with " << restoredSet.size() << " elements" << std::endl;
 *     std::cout << "LRU enabled: " << restoredSet.is_lru_enabled() << std::endl;
 *     
 *     // 检查进程A创建的数据
 *     for (int i = 1; i <= 10; ++i) {
 *         if (restoredSet.count(i) > 0) {
 *             std::cout << "Element " << i << " exists from Process A" << std::endl;
 *         }
 *     }
 * 
 *     // 访问一些元素（影响LRU顺序）
 *     std::vector<int> accessElements = {2, 5, 8};
 *     for (int elem : accessElements) {
 *         auto it = restoredSet.find(elem);
 *         if (it != restoredSet.end()) {
 *             std::cout << "Accessed element " << elem << std::endl;
 *         }
 *     }
 *     
 *     // 添加进程B的数据
 *     std::vector<int> newData = {11, 12, 13, 14, 15};
 *     restoredSet.insert(newData.begin(), newData.end());
 *     
 *     std::cout << "After Process B additions: " << restoredSet.size() << " elements" << std::endl;
 *     
 *     // 查看当前LRU顺序
 *     std::cout << "Current LRU order (first 10):" << std::endl;
 *     int elemCount = 0;
 *     for (auto it = restoredSet.list_begin(); 
 *              it != restoredSet.list_end() && elemCount < 10; 
 *              ++it, ++elemCount) {
 *         std::cout << *it << " ";
 *     }
 *     std::cout << std::endl;
 * }
 * ```
 * 
 * @subsection stl_interop STL容器互操作
 * 
 * ```cpp
 * // 从STL容器构造
 * std::unordered_set<std::string> stdSet = {
 *     "apple", "banana", "cherry", "apple"  // 注意：apple重复，STL会自动去重
 * };
 * 
 * NFShmHashSetWithList<std::string, 1000> shmSet(stdSet);  // 从STL构造
 * shmSet.enable_lru();
 * 
 * std::cout << "Converted from STL: " << shmSet.size() << " elements" << std::endl;
 * 
 * // 转换回STL容器（保持链表顺序）
 * std::vector<std::string> orderedElements;
 * for (auto it = shmSet.list_begin(); it != shmSet.list_end(); ++it) {
 *     orderedElements.push_back(*it);
 * }
 * 
 * // 使用STL算法
 * auto longWordCount = std::count_if(shmSet.begin(), shmSet.end(),
 *     [](const std::string& word) { return word.length() > 5; });
 * std::cout << "Words longer than 5 chars: " << longWordCount << std::endl;
 * 
 * // 模拟访问（影响LRU顺序）
 * std::vector<std::string> accessWords = {"apple", "cherry"};
 * for (const std::string& word : accessWords) {
 *     auto it = shmSet.find(word);
 *     if (it != shmSet.end()) {
 *         std::cout << "Accessed word: " << word << std::endl;
 *     }
 * }
 * 
 * // 添加新元素
 * std::vector<std::string> newWords = {"date", "elderberry", "fig"};
 * for (const std::string& word : newWords) {
 *     auto result = shmSet.insert(word);
 *     if (result.second) {
 *         std::cout << "Added new word: " << word << std::endl;
 *     }
 * }
 * 
 * // 基于访问频率的集合运算（模拟）
 * std::unordered_set<std::string> favoriteWords = {"apple", "banana", "date"};
 * 
 * // 找出收藏夹中存在的单词
 * std::vector<std::string> foundFavorites;
 * for (const std::string& word : favoriteWords) {
 *     if (shmSet.count(word) > 0) {
 *         foundFavorites.push_back(word);
 *     }
 * }
 * 
 * std::cout << "Favorite words found in set: ";
 * for (const std::string& word : foundFavorites) {
 *     std::cout << word << " ";
 * }
 * std::cout << std::endl;
 * 
 * // 获取最近访问的前N个元素
 * std::vector<std::string> recentWords;
 * int n = 3;
 * auto it = shmSet.list_begin();
 * for (int i = 0; i < n && it != shmSet.list_end(); ++i, ++it) {
 *     recentWords.push_back(*it);
 * }
 * 
 * std::cout << "Most recent " << n << " words: ";
 * for (const std::string& word : recentWords) {
 *     std::cout << word << " ";
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @section performance_notes 性能说明
 * 
 * - **查找性能**：O(1)平均，O(n)最坏（链表长度）
 * - **插入性能**：O(1)平均，重复元素快速拒绝，包含链表维护开销
 * - **删除性能**：O(1)平均，需更新链表结构
 * - **唯一性检查**：插入时自动进行，无额外开销
 * - **LRU操作**：O(1)时间复杂度的链表头尾操作
 * - **顺序遍历**：O(n)链表遍历，缓存友好
 * - **内存性能**：零碎片，预分配，额外链表节点开销
 * - **并发性能**：需要外部同步，但支持多进程读写
 * 
 * @section migration_guide 迁移指南
 * 
 * 从std::unordered_set迁移到NFShmHashSetWithList：
 * 
 * 1. **替换类型声明**：
 *    ```cpp
 *    // 原代码
 *    std::unordered_set<int> set;
 *    
 *    // 新代码（添加容量模板参数）
 *    NFShmHashSetWithList<int, 10000> set;
 *    ```
 * 
 * 2. **添加初始化**：
 *    ```cpp
 *    // 添加共享内存初始化
 *    set.CreateInit();  // 或 ResumeInit()
 *    ```
 * 
 * 3. **保持集合语义**：
 *    ```cpp
 *    // 集合的核心操作保持不变
 *    auto result = set.insert(element);
 *    bool inserted = result.second;  // 是否成功插入
 *    
 *    bool exists = set.count(element) > 0;  // 成员检查
 *    auto it = set.find(element);           // 查找元素
 *    ```
 * 
 * 4. **利用链表特性**：
 *    ```cpp
 *    // 启用LRU缓存语义
 *    set.enable_lru();
 *    
 *    // 使用链表顺序遍历
 *    for (auto it = set.list_begin(); it != set.list_end(); ++it) {
 *         // 按访问顺序处理元素
 *    }
 *    ```
 * 
 * 5. **处理容量限制**：
 *    ```cpp
 *    // 利用LRU自动淘汰机制
 *    if (set.full() && set.is_lru_enabled()) {
 *         // 插入会自动淘汰最久未使用的元素
 *         set.insert(element);
 *    }
 *    ```
 * 
 * 6. **移除不支持的操作**：
 *    ```cpp
 *    // 移除这些调用（NFShmHashSetWithList不支持）
 *    // set.rehash(1000);     // 动态管理不支持
 *    // set.reserve(500);     // 动态管理不支持
 *    // set.max_load_factor(0.8);  // 负载因子管理不支持
 *    ```
 * 
 * 7. **充分利用新特性**：
 *    ```cpp
 *    // 检查LRU状态
 *    if (set.is_lru_enabled()) {
 *         // 基于访问模式的逻辑
 *    }
 *    
 *    // 动态控制LRU行为
 *    set.disable_lru();  // 禁用LRU，仅维护访问顺序
 *    set.enable_lru();   // 重新启用LRU淘汰
 *    
 *    // 获取最近访问的元素
 *    auto recent_it = set.list_begin();  // 最近访问的元素
 *    ```
 */

/****************************************************************************
 * STL std::unordered_set 对比分析（WithList增强版）
 ****************************************************************************
 * 
 * 1. 内存管理策略对比：
 *    - std::unordered_set: 动态内存分配，使用allocator管理堆内存
 *    - NFShmHashSetWithList: 固定大小共享内存，预分配所有节点，支持进程间共享
 * 
 * 2. 容量管理对比：
 *    - std::unordered_set: 动态扩容，load_factor超过阈值时自动rehash
 *    - NFShmHashSetWithList: 固定容量MAX_SIZE，通过LRU机制处理容量满的情况
 * 
 * 3. 访问顺序管理对比：
 *    - std::unordered_set: 无访问顺序概念，遍历顺序依赖哈希桶
 *    - NFShmHashSetWithList: 内置双向链表维护访问/插入顺序，支持LRU语义
 * 
 * 4. 元素存储对比：
 *    - std::unordered_set: 直接存储元素值
 *    - NFShmHashSetWithList: 同样直接存储元素值，使用std::_Identity函数提取键
 * 
 * 5. 唯一性保证对比：
 *    - 都保证元素唯一性，不允许重复元素
 *    - 插入重复元素时都返回现有元素的迭代器
 * 
 * 6. 缓存语义对比：
 *    - std::unordered_set: 无缓存概念，需要手动实现LRU逻辑
 *    - NFShmHashSetWithList: 原生LRU支持，自动淘汰最久未使用元素
 * 
 * 7. 性能特征对比：
 *    - std::unordered_set: O(1)平均时间复杂度，但可能触发rehash开销
 *    - NFShmHashSetWithList: O(1)平均时间复杂度，额外O(1)链表维护开销，无rehash开销
 * 
 * 8. API兼容性：
 *    - 兼容接口：insert, find, erase, begin, end, size, empty, count等
 *    - 扩展接口：list_begin, list_end, enable_lru, disable_lru, is_lru_enabled等
 *    - 特有接口：full(), left_size(), CreateInit(), ResumeInit()等共享内存特有功能
 *    - 缺少接口：rehash, reserve, bucket, load_factor等动态管理接口
 * 
 * 9. 使用场景对比：
 *    - std::unordered_set: 通用集合容器，去重和成员检查
 *    - NFShmHashSetWithList: 缓存系统、访问跟踪、热点数据管理、LRU淘汰策略、共享内存应用
 *****************************************************************************/

// ==================== 前向声明 ====================

template <class Value, int MAX_SIZE,
          class HashFcn = std::hash<Value>,
          class EqualKey = std::equal_to<Value>>
class NFShmHashSetWithList;

template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
bool operator==(const NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>& hs2);

// ==================== 主要容器类 ====================

/**
 * @brief 基于共享内存的无序集合容器
 * @tparam Value 元素类型
 * @tparam MAX_SIZE 最大容量（固定）
 * @tparam HashFcn 哈希函数类型，默认std::hash<Value>
 * @tparam EqualKey 元素比较函数类型，默认std::equal_to<Value>
 *
 * 设计特点：
 * 1. 固定容量，不支持动态扩容（与STL主要区别）
 * 2. 基于共享内存，支持进程间共享
 * 3. API设计尽量兼容STL std::unordered_set
 * 4. 保证元素唯一性，不允许重复元素
 *
 * 与std::unordered_set的主要差异：
 * - 容量限制：固定大小 vs 动态扩容
 * - 内存管理：共享内存 vs 堆内存
 * - 性能特征：无rehash开销 vs 动态性能优化
 * - 进程支持：进程间共享 vs 单进程内使用
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
class NFShmHashSetWithList
{
private:
    /// @brief 底层哈希表类型，使用std::_Identity函数提取键（键即值本身）
    typedef NFShmHashTableWithList<Value, Value, MAX_SIZE, HashFcn, std::stl__Identity<Value>, EqualKey> HashTable;
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
     * @note 与std::unordered_set()行为类似，但增加共享内存初始化
     */
    NFShmHashSetWithList()
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
     * @note 与std::unordered_set(first, last)兼容
     */
    template <class InputIterator>
    NFShmHashSetWithList(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 数组构造函数
     * @param f 起始指针
     * @param l 结束指针
     * @note 与std::unordered_set兼容
     */
    NFShmHashSetWithList(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    NFShmHashSetWithList(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 拷贝构造函数
     * @param x 源NFShmHashSetWithList对象
     * @note 与std::unordered_set拷贝构造函数兼容
     */
    NFShmHashSetWithList(const NFShmHashSetWithList& x)
    {
        if (this != &x) m_hashTable = x.m_hashTable;
    }

    /**
     * @brief 从std::unordered_set构造
     * @param set 源std::unordered_set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashSetWithList(const std::unordered_set<Value>& set)
    {
        m_hashTable.insert_unique(set.begin(), set.end());
    }

    /**
     * @brief 从std::set构造
     * @param set 源std::set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashSetWithList(const std::set<Value>& set)
    {
        m_hashTable.insert_unique(set.begin(), set.end());
    }

    /**
     * @brief 初始化列表构造函数
     * @param list 初始化列表
     * @note 与std::unordered_set(std::initializer_list)兼容
     */
    NFShmHashSetWithList(const std::initializer_list<Value>& list)
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
        new(this) NFShmHashSetWithList();
    }

    // ==================== 赋值操作符 ====================

    /**
     * @brief 拷贝赋值操作符
     * @param x 源NFShmHashSetWithList对象
     * @return 自身引用
     * @note 与std::unordered_set::operator=兼容
     */
    NFShmHashSetWithList<Value, MAX_SIZE>& operator=(const NFShmHashSetWithList<Value, MAX_SIZE>& x);

    /**
     * @brief 从std::unordered_set赋值
     * @param x 源std::unordered_set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashSetWithList<Value, MAX_SIZE>& operator=(const std::unordered_set<Value>& x);

    /**
     * @brief 从std::set赋值
     * @param x 源std::set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashSetWithList<Value, MAX_SIZE>& operator=(const std::set<Value>& x);

    /**
     * @brief 从初始化列表赋值
     * @param x 初始化列表
     * @return 自身引用
     * @note 与std::unordered_set::operator=(std::initializer_list)兼容
     */
    NFShmHashSetWithList<Value, MAX_SIZE>& operator=(const std::initializer_list<Value>& x);

public:
    // ==================== 容量相关接口（STL兼容） ====================

    /**
     * @brief 获取当前元素数量
     * @return 元素数量
     * @note 与std::unordered_set::size()兼容
     */
    size_type size() const { return m_hashTable.size(); }

    /**
     * @brief 获取最大容量
     * @return 最大容量MAX_SIZE
     * @note 与std::unordered_set::max_size()不同，返回固定值而非理论最大值
     */
    size_type max_size() const { return m_hashTable.max_size(); }

    /**
     * @brief 判断是否为空
     * @return true表示空
     * @note 与std::unordered_set::empty()兼容
     */
    bool empty() const { return m_hashTable.empty(); }

    /**
     * @brief 交换两个容器的内容
     * @param hs 另一个NFShmHashSetWithList对象
     * @note 与std::unordered_set::swap()兼容
     */
    void swap(NFShmHashSetWithList& hs) noexcept { m_hashTable.swap(hs.m_hashTable); }

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

    template <class Val, int X_MAX_SIZE, class Hf, class EqK>
    friend bool operator==(const NFShmHashSetWithList<Val, X_MAX_SIZE, Hf, EqK>&,
                           const NFShmHashSetWithList<Val, X_MAX_SIZE, Hf, EqK>&);

    // ==================== STL兼容迭代器接口 ====================

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     * @note 与std::unordered_set::begin()兼容
     */
    iterator begin() { return m_hashTable.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     * @note 与std::unordered_set::end()兼容
     */
    iterator end() { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_set::begin() const兼容
     */
    const_iterator begin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_set::end() const兼容
     */
    const_iterator end() const { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_set::cbegin()兼容
     */
    const_iterator cbegin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_set::cend()兼容
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
     * @brief 插入元素
     * @param obj 要插入的元素
     * @return pair<iterator, bool>，迭代器指向元素，bool表示是否插入成功
     * @note 与std::unordered_set::insert()兼容
     */
    std::pair<iterator, bool> insert(const value_type& obj)
    {
        return m_hashTable.insert_unique(obj);
    }

    /**
     * @brief 带提示插入元素
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param obj 要插入的元素
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_set::insert(const_iterator, const value_type&)兼容
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
     * @note 与std::unordered_set::emplace()兼容
     */
    template <typename... Args>
    std::pair<iterator, bool> emplace(const Args&... args);

    /**
     * @brief 带提示就地构造元素
     * @tparam Args 构造参数类型包
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_set::emplace_hint()兼容
     */
    template <typename... Args>
    iterator emplace_hint(const_iterator hint, const Args&... args);

    /**
     * @brief 范围插入
     * @tparam InputIterator 输入迭代器类型
     * @param f 起始迭代器
     * @param l 结束迭代器
     * @note 与std::unordered_set::insert(first, last)兼容
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

    // ==================== 查找接口（STL兼容） ====================

    /**
     * @brief 查找元素
     * @param key 要查找的键（元素值）
     * @return 指向元素的迭代器，未找到返回end()
     * @note 与std::unordered_set::find()兼容
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
     * @return 元素数量（0或1，因为是unique容器）
     * @note 与std::unordered_set::count()兼容
     */
    size_type count(const key_type& key) const
    {
        return m_hashTable.count(key);
    }

    /**
     * @brief 获取指定键的元素范围
     * @param key 要查找的键
     * @return pair<iterator, iterator>表示范围
     * @note 与std::unordered_set::equal_range()兼容
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
     * @brief 根据键删除元素
     * @param key 要删除的键
     * @return 删除的元素数量
     * @note 与std::unordered_set::erase()兼容
     */
    size_type erase(const key_type& key)
    {
        return m_hashTable.erase(key);
    }

    /**
     * @brief 根据迭代器删除元素
     * @param it 指向要删除元素的迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_set::erase()兼容
     */
    iterator erase(iterator it)
    {
        return m_hashTable.erase(it);
    }

    /**
     * @brief 根据常量迭代器删除元素
     * @param it 指向要删除元素的常量迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_set::erase()兼容
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
     * @note 与std::unordered_set::erase(first, last)兼容
     */
    iterator erase(const_iterator f, const_iterator l)
    {
        return m_hashTable.erase(f, l);
    }

    /**
     * @brief 清空所有元素
     * @note 与std::unordered_set::clear()兼容
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
     * @note 与std::unordered_set::rehash()类似但实际不执行操作（固定容量）
     */
    void resize(size_type hint)
    {
        m_hashTable.resize(hint);
    }

    /**
     * @brief 获取桶数量
     * @return 桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_set::bucket_count()兼容，但返回固定值
     */
    size_type bucket_count() const
    {
        return m_hashTable.bucket_count();
    }

    /**
     * @brief 获取最大桶数量
     * @return 最大桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_set::max_bucket_count()兼容
     */
    size_type max_bucket_count() const
    {
        return m_hashTable.max_bucket_count();
    }

    /**
     * @brief 获取指定桶中的元素数量
     * @param n 桶索引
     * @return 该桶中的元素数量
     * @note 与std::unordered_set::bucket_size()类似
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
NFShmHashSetWithList<Value, MAX_SIZE>& NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::set<Value>& x)
{
    clear();
    m_hashTable.insert_unique(x.begin(), x.end());
    return *this;
}

/**
 * @brief 从std::unordered_set赋值实现
 * @param x 源std::unordered_set对象
 * @return 自身引用
 * @note STL容器没有此接口，为方便从STL无序容器转换而提供
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashSetWithList<Value, MAX_SIZE>& NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::unordered_set<Value>& x)
{
    clear();
    m_hashTable.insert_unique(x.begin(), x.end());
    return *this;
}

/**
 * @brief 拷贝赋值操作符实现
 * @param x 源NFShmHashSetWithList对象
 * @return 自身引用
 * @note 与std::unordered_set::operator=兼容
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashSetWithList<Value, MAX_SIZE>& NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const NFShmHashSetWithList<Value, MAX_SIZE>& x)
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
 * @note 与std::unordered_set::operator=(std::initializer_list)兼容
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashSetWithList<Value, MAX_SIZE>& NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::initializer_list<Value>& x)
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
 * @note 与std::unordered_set::emplace()兼容
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename... Args>
std::pair<typename NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::iterator, bool>
NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::emplace(const Args&... args)
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
 * @note 与std::unordered_set::emplace_hint()兼容，但忽略提示参数
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename... Args>
typename NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::iterator
NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>::emplace_hint(const_iterator hint, const Args&... args)
{
    (void)hint; // 忽略提示参数
    value_type obj(args...);
    return m_hashTable.insert_unique(obj).first;
}

// ==================== 全局操作符 ====================

/**
 * @brief 相等比较操作符
 * @param hs1 第一个NFShmHashSetWithList对象
 * @param hs2 第二个NFShmHashSetWithList对象
 * @return true表示相等
 * @note 与std::unordered_set的operator==兼容
 * @note 比较所有元素是否相等，顺序无关
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
bool operator==(const NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>& hs2)
{
    return hs1.m_hashTable == hs2.m_hashTable;
}

template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
void swap(const NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashSetWithList<Value, MAX_SIZE, HashFcn, EqualKey>& hs2)
{
    hs1.m_hashTable.swap(hs2.m_hashTable);
}
