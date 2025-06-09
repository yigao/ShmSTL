// -------------------------------------------------------------------------
//    @FileName         :    NFShmHashTableWithList.h
//    @Author           :    gaoyi
//    @Date             :    23-2-6
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmHashTableWithList
//
// -------------------------------------------------------------------------

/**
 * @file NFShmHashTableWithList.h
 * @brief 基于共享内存的增强型哈希表实现，融合哈希查找与插入顺序链表功能
 * 
 * @section overview 概述
 * 
 * NFShmHashTableWithList 是一个创新性的共享内存容器，它将高效的哈希表查找功能
 * 与有序链表的插入顺序维护功能完美结合。该实现在API设计上高度兼容STL，同时
 * 针对共享内存环境进行了深度优化，特别适用于需要快速查找和插入顺序维护的应用场景。
 * 
 * @section features 核心特性
 * 
 * 1. **双重数据结构**：
 *    - 哈希表结构：O(1)平均查找、插入、删除性能
 *    - 插入顺序链表：维护元素插入顺序，支持FIFO访问
 *    - 链地址法解决冲突，固定桶数量设计
 *    - 双重迭代器系统：哈希迭代器 + 链表迭代器
 * 
 * 2. **智能LRU功能**：
 *    - 可控的LRU（最近最少使用）访问模式
 *    - enable_lru()/disable_lru() 动态开关控制
 *    - find/count操作可选择性触发LRU更新
 *    - 适用于缓存淘汰策略和热点数据管理
 * 
 * 3. **共享内存优化**：
 *    - 固定大小内存布局，避免动态分配
 *    - 基于索引的双重链表实现，支持进程间共享
 *    - 内存对齐优化，提高访问效率
 *    - 支持CREATE/RESUME模式初始化
 * 
 * 4. **STL高度兼容**：
 *    - 完整的前向迭代器支持（哈希遍历）
 *    - 标准的容器操作（insert、find、erase、clear等）
 *    - 扩展的链表迭代器（按插入顺序遍历）
 *    - 自定义哈希函数和比较函数支持
 * 
 * 5. **扩展功能**：
 *    - 节点池管理和回收
 *    - 容量检查功能（full()、left_size()等）
 *    - 插入顺序迭代器（list_begin()、list_end()等）
 *    - 详细的调试和诊断接口
 *    - 链表完整性验证功能
 * 
 * @section stl_comparison STL容器对比
 * 
 * | 特性 | STL unordered_map | NFShmHashTableWithList |
 * |------|-------------------|------------------------|
 * | **数据结构** | 哈希表(链地址法) | **哈希表+插入顺序链表** |
 * | **容量管理** | 动态扩容，无限制 | 固定容量MAX_SIZE，编译时确定 |
 * | **内存管理** | 堆内存，动态分配 | 共享内存，预分配节点池 |
 * | **桶管理** | 动态调整，自动rehash | 固定桶数量，无rehash |
 * | **插入删除** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 |
 * | **查找访问** | O(1)平均 | O(1)平均，可选LRU更新 |
 * | **迭代器类型** | 前向迭代器（哈希顺序） | **双重迭代器**（哈希+插入顺序） |
 * | **插入顺序** | 不维护 | **自动维护插入顺序** |
 * | **LRU功能** | 不支持 | **内置LRU访问模式** |
 * | **内存布局** | 分散分配 | 连续节点池 |
 * | **进程共享** | 不支持 | **原生支持** |
 * | **异常安全** | 强异常安全保证 | 无异常，错误码返回 |
 * | **内存碎片** | 可能产生 | **无碎片**（固定节点池） |
 * | **负载因子** | 自动维护 | 固定结构，无动态调整 |
 * | **遍历方式** | 只有哈希遍历 | **哈希遍历+插入顺序遍历** |
 * 
 * @section api_compatibility API兼容性
 * 
 * **完全兼容的接口**：
 * - size(), empty(), max_size()
 * - begin(), end() - 哈希遍历迭代器
 * - insert(), insert_unique(), insert_equal()
 * - find(), count(), equal_range()
 * - erase(), clear()
 * - bucket_count(), max_bucket_count()
 * - swap()
 * 
 * **扩展的接口（新增）**：
 * - **链表迭代器**：list_begin(), list_end(), list_cbegin(), list_cend()
 * - **LRU控制**：enable_lru(), disable_lru(), is_lru_enabled()
 * - **容量检查**：full(), left_size()
 * - **共享内存**：CreateInit(), ResumeInit()
 * - **调试接口**：print_structure(), print_detailed(), print_simple(), print_list()
 * - **节点访问**：GetValidNode(), get_iterator()
 * 
 * **行为差异的接口**：
 * - find(): 当启用LRU时，会将访问的节点移到链表尾部
 * - count(): 当启用LRU时，会触发LRU更新
 * - insert(): 新元素总是添加到链表尾部（保持插入顺序）
 * - erase(): 从哈希表和链表中同时移除
 * - max_size()：返回MAX_SIZE而非理论最大值
 * - bucket_count()：返回固定值MAX_SIZE
 * - 不支持：rehash(), reserve(), load_factor()等动态调整接口
 * 
 * @section usage_examples 使用示例
 * 
 * @subsection basic_usage 基础用法（类似std::unordered_map + 插入顺序）
 * 
 * ```cpp
 * // 定义容量为1000的字符串到整数映射
 * typedef std::pair<std::string, int> KeyValue;
 * struct KeyExtractor { const std::string& operator()(const KeyValue& kv) const { return kv.first; } };
 * 
 * NFShmHashTableWithList<KeyValue, std::string, 1000, 
 *                        std::hash<std::string>, KeyExtractor, 
 *                        std::equal_to<std::string>> table;
 * table.CreateInit();  // 创建模式初始化
 * 
 * // STL兼容的基础操作
 * table.insert_unique(KeyValue("apple", 100));
 * table.insert_unique(KeyValue("banana", 200));
 * table.insert_unique(KeyValue("orange", 300));
 * 
 * // 哈希查找操作（O(1)平均复杂度）
 * auto it = table.find("apple");
 * if (it != table.end()) {
 *     std::cout << "Found: " << it->first << " = " << it->second << std::endl;
 * }
 * 
 * // 按哈希顺序遍历（传统方式）
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     std::cout << "Hash order: " << it->first << " = " << it->second << std::endl;
 * }
 * 
 * // 按插入顺序遍历（新增功能）
 * for (auto it = table.list_begin(); it != table.list_end(); ++it) {
 *     std::cout << "Insert order: " << it->first << " = " << it->second << std::endl;
 * }
 * 
 * // C++11 范围for循环（按插入顺序）
 * for (const auto& pair : table.list_range()) {
 *     std::cout << pair.first << " = " << pair.second << std::endl;
 * }
 * ```
 * 
 * @subsection lru_usage LRU缓存实现
 * 
 * ```cpp
 * typedef std::pair<int, std::string> CacheItem;
 * struct KeyExtractor { int operator()(const CacheItem& item) const { return item.first; } };
 * 
 * NFShmHashTableWithList<CacheItem, int, 100, 
 *                        std::hash<int>, KeyExtractor, 
 *                        std::equal_to<int>> cache;
 * cache.CreateInit();
 * cache.enable_lru();  // 启用LRU功能
 * 
 * // 插入缓存项（按插入顺序添加到链表尾部）
 * cache.insert_unique(CacheItem(1, "First"));
 * cache.insert_unique(CacheItem(2, "Second"));
 * cache.insert_unique(CacheItem(3, "Third"));
 * 
 * // 访问元素（自动移到链表尾部）
 * auto it = cache.find(1);  // 元素1被访问，移到链表尾部
 * if (it != cache.end()) {
 *     std::cout << "Cache hit: " << it->second << std::endl;
 * }
 * 
 * // LRU淘汰：最少使用的元素在链表头部
 * if (cache.full()) {
 *     auto oldest = cache.list_begin();  // 获取最少使用的元素
 *     std::cout << "Evicting: " << oldest->second << std::endl;
 *     cache.erase(oldest);  // 淘汰最少使用的元素
 * }
 * 
 * // 遍历缓存（从最少使用到最近使用）
 * std::cout << "Cache items (LRU order):" << std::endl;
 * for (auto it = cache.list_begin(); it != cache.list_end(); ++it) {
 *     std::cout << "  " << it->first << ": " << it->second << std::endl;
 * }
 * ```
 * 
 * @subsection ordered_access 有序访问和FIFO队列
 * 
 * ```cpp
 * typedef std::pair<int, std::string> Task;
 * struct TaskKeyExtractor { int operator()(const Task& task) const { return task.first; } };
 * 
 * NFShmHashTableWithList<Task, int, 500, 
 *                        std::hash<int>, TaskKeyExtractor, 
 *                        std::equal_to<int>> taskQueue;
 * taskQueue.CreateInit();
 * taskQueue.disable_lru();  // 禁用LRU，保持严格的插入顺序
 * 
 * // 任务队列：按ID插入任务
 * taskQueue.insert_unique(Task(101, "Process data"));
 * taskQueue.insert_unique(Task(102, "Send email"));
 * taskQueue.insert_unique(Task(103, "Update database"));
 * 
 * // 快速按ID查找任务（O(1)哈希查找）
 * auto taskIt = taskQueue.find(102);
 * if (taskIt != taskQueue.end()) {
 *     std::cout << "Found task: " << taskIt->second << std::endl;
 * }
 * 
 * // FIFO处理：按插入顺序处理任务
 * while (!taskQueue.empty()) {
 *     auto firstTask = taskQueue.list_begin();
 *     std::cout << "Processing task " << firstTask->first 
 *               << ": " << firstTask->second << std::endl;
 *     taskQueue.erase(firstTask);  // 处理完成后移除
 * }
 * ```
 * 
 * @subsection debugging_monitoring 调试和监控
 * 
 * ```cpp
 * NFShmHashTableWithList<KeyValue, std::string, 100, 
 *                        std::hash<std::string>, KeyExtractor, 
 *                        std::equal_to<std::string>> debugTable;
 * 
 * // 容量监控
 * std::cout << "Capacity: " << debugTable.max_size() << std::endl;
 * std::cout << "Current size: " << debugTable.size() << std::endl;
 * std::cout << "Free space: " << debugTable.left_size() << std::endl;
 * std::cout << "Is full: " << (debugTable.full() ? "Yes" : "No") << std::endl;
 * 
 * // 哈希表结构调试
 * debugTable.print_structure();  // 显示桶链表结构
 * debugTable.print_detailed();   // 显示所有节点详细信息
 * debugTable.print_simple();     // 显示简化的桶信息
 * debugTable.print_list();       // 显示插入顺序链表
 * 
 * // 链表完整性验证
 * if (debugTable.ValidateListIntegrity()) {
 *     std::cout << "List integrity: OK" << std::endl;
 * } else {
 *     std::cout << "List integrity: CORRUPTED!" << std::endl;
 * }
 * 
 * // LRU状态监控
 * std::cout << "LRU enabled: " << (debugTable.is_lru_enabled() ? "Yes" : "No") << std::endl;
 * ```
 * 
 * @subsection dual_iteration 双重迭代方式对比
 * 
 * ```cpp
 * NFShmHashTableWithList<KeyValue, std::string, 100, 
 *                        std::hash<std::string>, KeyExtractor, 
 *                        std::equal_to<std::string>> dualTable;
 * 
 * // 插入一些数据
 * dualTable.insert_unique(KeyValue("zebra", 1));
 * dualTable.insert_unique(KeyValue("apple", 2));
 * dualTable.insert_unique(KeyValue("banana", 3));
 * 
 * std::cout << "=== 哈希顺序遍历（性能优化的遍历） ===" << std::endl;
 * for (auto it = dualTable.begin(); it != dualTable.end(); ++it) {
 *     std::cout << it->first << " -> " << it->second << std::endl;
 * }
 * // 输出可能是：apple->2, zebra->1, banana->3 (哈希桶顺序)
 * 
 * std::cout << "\n=== 插入顺序遍历（业务逻辑相关的遍历） ===" << std::endl;
 * for (auto it = dualTable.list_begin(); it != dualTable.list_end(); ++it) {
 *     std::cout << it->first << " -> " << it->second << std::endl;
 * }
 * // 输出：zebra->1, apple->2, banana->3 (严格按插入顺序)
 * 
 * std::cout << "\n=== 统计和性能对比 ===" << std::endl;
 * std::cout << "哈希查找时间复杂度: O(1) 平均" << std::endl;
 * std::cout << "插入顺序遍历复杂度: O(n)" << std::endl;
 * std::cout << "内存额外开销: 每节点2个索引（prev/next）" << std::endl;
 * ```
 * 
 * @section performance_characteristics 性能特征
 * 
 * | 操作 | STL unordered_map | NFShmHashTableWithList |
 * |------|-------------------|------------------------|
 * | **查找 find** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 + 可选LRU更新 |
 * | **插入 insert** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 + O(1)链表更新 |
 * | **删除 erase** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 + O(1)链表更新 |
 * | **哈希遍历** | O(n) | O(n) |
 * | **插入顺序遍历** | 不支持 | **O(n)** |
 * | **LRU访问** | 不支持 | **O(1)** |
 * | **rehash** | O(n) | **不支持**（固定结构） |
 * | **内存分配** | 动态分配 | 预分配固定内存 |
 * | **缓存友好性** | 中等（桶分散） | **较好**（连续节点池） |
 * | **内存开销** | 动态 | 固定MAX_SIZE * sizeof(Node) + 链表指针 |
 * | **负载因子** | 自动维护 | 需要预估合理MAX_SIZE |
 * 
 * @section memory_layout 内存布局
 * 
 * ```
 * NFShmHashTableWithList 内存结构：
 * ┌─────────────────┐
 * │   管理数据       │ <- 基类信息，元素数量，LRU状态等
 * ├─────────────────┤
 * │   链表管理       │ <- m_listHead, m_listTail, m_enableLRU
 * ├─────────────────┤
 * │   桶索引数组     │ <- m_bucketsFirstIdx[MAX_SIZE]
 * │   [0] -> node_x │    ├─ 桶0的首节点索引
 * │   [1] -> node_y │    ├─ 桶1的首节点索引
 * │   ...           │    ├─ ...
 * │   [MAX-1] -> -1 │    └─ 桶MAX_SIZE-1（空桶）
 * ├─────────────────┤
 * │   节点池         │ <- m_mem[MAX_SIZE] (AlignedStorage)
 * │   [0] 节点0      │    ├─ value + hash_next + list_prev + list_next + valid
 * │   [1] 节点1      │    ├─ ...
 * │   ...           │    ├─ ...
 * │   [MAX-1] 节点   │    └─ 最后一个节点
 * └─────────────────┘
 * 
 * 增强节点结构：
 * ┌─────────────────┐
 * │   m_value       │ <- 存储的键值对或值
 * │   m_next        │ <- 哈希链表中下一个节点索引
 * │   m_listPrev    │ <- 插入顺序链表中前一个节点索引
 * │   m_listNext    │ <- 插入顺序链表中下一个节点索引
 * │   m_valid       │ <- 节点是否有效
 * │   m_self        │ <- 自身索引（调试用）
 * └─────────────────┘
 * 
 * 双重链表示例：
 * 哈希桶链表（查找性能）：
 * 桶0: node5 -> node12 -> node89 -> -1
 * 桶1: node3 -> -1
 * 桶2: -1 (空桶)
 * 桶3: node7 -> node24 -> -1
 * 
 * 插入顺序链表（业务逻辑）：
 * head -> node3 -> node5 -> node7 -> node12 -> node24 -> node89 -> tail
 *         (第1个)  (第2个)  (第3个)  (第4个)   (第5个)   (第6个)
 * 
 * LRU访问后的链表变化：
 * 访问node5后: head -> node3 -> node7 -> node12 -> node24 -> node89 -> node5 -> tail
 *                      (node5移到尾部，成为最近访问)
 * 
 * 优势总结：
 * - 哈希查找：O(1)平均复杂度
 * - 插入顺序：严格维护，适合FIFO场景
 * - LRU功能：高效的缓存淘汰策略
 * - 内存效率：固定布局，无碎片
 * - 进程共享：基于索引，地址无关
 * ```
 * 
 * @section thread_safety 线程安全
 * 
 * - **非线程安全**：需要外部同步机制
 * - **共享内存兼容**：多进程可安全访问（需进程间锁）
 * - **无内部锁**：避免性能开销，由用户控制并发
 * - **迭代器稳定性**：插入删除可能影响其他迭代器
 * - **LRU操作**：find操作可能修改链表结构，需要写锁保护
 * 
 * @section migration_guide 从STL迁移指南
 * 
 * 1. **包含头文件**：
 *    ```cpp
 *    // 替换
 *    #include <unordered_map>
 *    // 为
 *    #include "NFComm/NFShmStl/NFShmHashTableWithList.h"
 *    ```
 * 
 * 2. **类型定义**：
 *    ```cpp
 *    // STL
 *    std::unordered_map<std::string, int> map;
 *    
 *    // NFShmHashTableWithList（需要完整模板参数）
 *    typedef std::pair<std::string, int> KeyValue;
 *    struct KeyExtractor { const std::string& operator()(const KeyValue& kv) const { return kv.first; } };
 *    
 *    NFShmHashTableWithList<KeyValue, std::string, 1000,
 *                           std::hash<std::string>, 
 *                           KeyExtractor,
 *                           std::equal_to<std::string>> map;
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
 *    // NFShmHashTableWithList方式
 *    map.insert_unique(KeyValue(key, value));
 *    // 或使用find_or_insert
 *    map.find_or_insert(KeyValue(key, value)) = value;
 *    ```
 * 
 * 5. **遍历方式**：
 *    ```cpp
 *    // STL方式（仅哈希顺序）
 *    for (const auto& pair : map) {
 *        // 哈希顺序遍历
 *    }
 *    
 *    // NFShmHashTableWithList方式（双重选择）
 *    // 哈希顺序遍历（兼容STL）
 *    for (auto it = map.begin(); it != map.end(); ++it) {
 *        // 哈希顺序
 *    }
 *    
 *    // 插入顺序遍历（新增功能）
 *    for (auto it = map.list_begin(); it != map.list_end(); ++it) {
 *        // 插入顺序
 *    }
 *    ```
 * 
 * 6. **LRU功能**：
 *    ```cpp
 *    // 启用LRU缓存功能
 *    map.enable_lru();
 *    
 *    // 访问元素（自动LRU更新）
 *    auto it = map.find(key);
 *    
 *    // 获取最少使用的元素
 *    auto lru = map.list_begin();
 *    
 *    // 淘汰最少使用的元素
 *    map.erase(lru);
 *    ```
 * 
 * 7. **容量管理**：
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
 * 1. **场景选择**：
 *    - **缓存系统**：启用LRU，结合哈希查找和淘汰策略
 *    - **任务队列**：禁用LRU，严格维护FIFO顺序
 *    - **调试监控**：利用插入顺序遍历观察数据变化
 *    - **数据分析**：双重遍历方式，适应不同分析需求
 * 
 * 2. **LRU管理**：
 *    - 根据使用场景选择开启或关闭LRU
 *    - LRU开启时，find/count操作有额外开销
 *    - 缓存场景优先考虑LRU，队列场景避免使用
 * 
 * 3. **容量规划**：
 *    - 根据预期数据量和负载因子选择MAX_SIZE
 *    - 考虑哈希分布质量，避免过多冲突
 *    - 预留适当空间，避免频繁的满容量状态
 * 
 * 4. **迭代器使用**：
 *    - 根据业务需求选择哈希迭代器或链表迭代器
 *    - 插入删除可能使迭代器失效，及时更新
 *    - LRU操作会改变链表迭代器顺序
 * 
 * 5. **性能优化**：
 *    - 使用高质量哈希函数，确保均匀分布
 *    - 合理设置容量，避免过高的负载因子
 *    - 批量操作时考虑LRU的性能影响
 *    - 调试时使用print_*系列函数诊断性能问题
 * 
 * 6. **调试技巧**：
 *    - 使用ValidateListIntegrity()验证数据完整性
 *    - print_list()专门调试插入顺序链表
 *    - 监控left_size()预防容量不足
 *    - 对比哈希遍历和链表遍历的结果差异
 * 
 * @warning 注意事项
 * - 固定容量限制，超出MAX_SIZE的操作会失败
 * - LRU功能会在查找时修改数据结构，需要考虑并发安全
 * - 插入顺序链表增加了额外的内存开销（每节点2个额外索引）
 * - 不支持动态rehash，需要合理预估容量和负载因子
 * - 哈希函数质量直接影响性能，避免频繁冲突
 * - 共享内存环境下需要考虑进程崩溃的数据恢复
 * - 非线程安全，多线程访问需要外部同步
 * - LRU操作可能改变链表迭代器的访问顺序
 * 
 * @see NFShmHashTable - 基础共享内存哈希表实现
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

/****************************************************************************
 * STL标准Hash实现对比分析
 ****************************************************************************
 *
 * 1. 内存管理策略对比：
 *    - STL unordered_map: 动态内存分配，使用allocator管理堆内存
 *    - NFShmHashTableWithList: 固定大小共享内存，预分配所有节点，支持进程间共享
 *
 * 2. 容量管理对比：
 *    - STL: 动态扩容，load_factor超过阈值时自动rehash，桶数量可变
 *    - NFShmHashTableWithList: 固定容量MAX_SIZE，不支持动态扩容，桶数量固定
 *
 * 3. 冲突解决策略对比：
 *    - 都使用链地址法(Separate Chaining)解决冲突
 *    - STL: 使用std::forward_list或类似结构
 *    - NFShmHashTableWithList: 使用索引链表，节点包含next索引而非指针
 *
 * 4. 迭代器实现对比：
 *    - 都实现forward_iterator，支持单向遍历
 *    - STL: 基于指针或智能指针
 *    - NFShmHashTableWithList: 基于索引，需要验证节点有效性
 *
 * 5. 线程安全对比：
 *    - STL: 非线程安全，需要外部同步
 *    - NFShmHashTableWithList: 非线程安全，但支持进程间共享
 *
 * 6. API兼容性：
 *    - 大部分接口与STL兼容：insert, find, erase, begin, end等
 *    - 特有接口：CreateInit, ResumeInit, print_*等调试接口
 *    - 缺少接口：rehash, reserve, bucket_size等动态管理接口
 *****************************************************************************/

/****************************************************************************
 * 链表功能特性说明
 ****************************************************************************
 *
 * NFShmHashTableWithList 在保持哈希表高效查找性能的同时，额外提供了链表功能：
 *
 * 1. 插入顺序维护：
 *    - 自动维护元素的插入顺序，新元素总是添加到链表尾部
 *    - 支持FIFO（先进先出）访问模式，适用于缓存和队列场景
 *    - 实现原理：每个节点包含 m_listPrev 和 m_listNext 索引
 *
 * 2. LRU功能控制：
 *    - 可开关的LRU（最近最少使用）访问模式
 *    - 当启用LRU时，find() 和 count() 操作会将访问的节点移动到链表尾部
 *    - 提供 enable_lru()、disable_lru()、is_lru_enabled() 控制接口
 *    - 适用于缓存淘汰策略实现
 *
 * 3. 链表迭代器：
 *    - list_iterator：非常量链表迭代器，支持按插入顺序遍历
 *    - const_list_iterator：常量链表迭代器
 *    - 提供 list_begin()、list_end()、list_cbegin()、list_cend() 接口
 *    - 与STL风格保持一致，支持范围for循环
 *
 * 4. 内存效率设计：
 *    - 使用索引而非指针，适合共享内存环境
 *    - 链表指针直接集成在节点中，无额外内存分配
 *    - 所有链表操作均为O(1)复杂度
 *
 * 5. 自动维护一致性：
 *    - 插入操作自动将新节点添加到链表尾部
 *    - 删除操作自动从链表中移除节点
 *    - 清空操作自动重置链表状态
 *    - 错误检查确保链表操作的安全性
 *
 * 6. 使用场景：
 *    - 缓存系统：结合哈希查找和LRU淘汰策略
 *    - 有序访问：需要按插入顺序遍历的场景
 *    - 调试和监控：按插入顺序查看数据状态
 *    - 队列实现：支持FIFO访问模式的队列
 *
 * 示例用法：
 * ```cpp
 * // 创建哈希表
 * NFShmHashTableWithList<KeyValue, int, 100, HashFunc, ExtractKey, EqualKey> hashTable;
 * 
 * // 插入数据（自动维护插入顺序）
 * hashTable.insert_unique(KeyValue(1, "First"));
 * hashTable.insert_unique(KeyValue(2, "Second"));
 * 
 * // 按插入顺序遍历
 * for (auto it = hashTable.list_begin(); it != hashTable.list_end(); ++it) {
 *     // 按插入顺序访问元素
 * }
 * 
 * // 启用LRU功能
 * hashTable.enable_lru();
 * auto it = hashTable.find(1);  // 访问元素1，将其移到链表尾部
 * ```
 *****************************************************************************/

// ==================== 前向声明 ====================

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
class NFShmHashTableWithList;

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableWithListIterator;

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableWithListConstIterator;

// 新增：链表迭代器前向声明
template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableWithListListIterator;

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableWithListConstListIterator;

// ==================== 节点定义 ====================

/**
 * @brief Hash表节点结构
 * @tparam Val 存储的值类型
 *
 * 与STL不同，使用索引而非指针构建链表，适合共享内存环境。
 * STL通常使用指针链表，而这里使用索引链表提高共享内存兼容性。
 * 
 * 新增功能：支持插入顺序链表，用于维护插入顺序和LRU访问顺序
 */
template <class Val>
struct NFShmHashTableWithListNode
{
    /**
     * @brief 构造函数，根据创建模式选择初始化方式
     *
     * 与STL节点不同，支持共享内存的创建/恢复模式：
     * - 创建模式：初始化所有成员
     * - 恢复模式：从共享内存恢复，保持现有状态
     */
    NFShmHashTableWithListNode()
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
        m_next = INVALID_ID;
        m_self = 0;
        m_listPrev = INVALID_ID; // 链表前驱索引
        m_listNext = INVALID_ID; // 链表后继索引
        return 0;
    }

    /// @brief 恢复模式初始化
    int ResumeInit()
    {
        return 0;
    }

    int m_next; ///< 下一个节点的索引（INVALID_ID表示链表末尾）
    Val m_value; ///< 存储的值
    bool m_valid; ///< 节点是否有效
    size_t m_self; ///< 节点自身索引（用于调试和验证）

    // 新增：链表指针，用于维护插入顺序和LRU访问顺序
    int m_listPrev; ///< 链表中前一个节点的索引（INVALID_ID表示链表头）
    int m_listNext; ///< 链表中下一个节点的索引（INVALID_ID表示链表尾）
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
struct NFShmHashTableWithListIterator
{
    // ==================== STL兼容类型定义 ====================
    typedef NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> Hashtable;
    typedef NFShmHashTableWithListIterator iterator;
    typedef NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> const_iterator;
    typedef NFShmHashTableWithListNode<Val> Node;

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
    NFShmHashTableWithListIterator(Node* n, Hashtable* tab)
        : m_curNode(n), m_hashTable(tab)
    {
    }

    /// @brief 默认构造函数，创建无效迭代器
    NFShmHashTableWithListIterator(): m_curNode(nullptr), m_hashTable(nullptr)
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
Val NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::m_staticError = Val();

/**
 * @brief Hash表常量迭代器
 * @note 与非常量迭代器类似，但提供const访问
 * @note 与STL const_iterator兼容
 */
template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableWithListConstIterator
{
    // ==================== STL兼容类型定义 ====================
    typedef NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> Hashtable;
    typedef NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> iterator;
    typedef NFShmHashTableWithListConstIterator const_iterator;
    typedef NFShmHashTableWithListNode<Val> Node;

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

    NFShmHashTableWithListConstIterator(const Node* n, const Hashtable* tab)
        : m_curNode(n), m_hashTable(tab)
    {
    }

    NFShmHashTableWithListConstIterator(): m_curNode(nullptr), m_hashTable(nullptr)
    {
    }

    /**
     * @brief 从非常量迭代器构造
     * @param it 非常量迭代器
     * @note STL标准要求支持此转换
     */
    NFShmHashTableWithListConstIterator(const iterator& it)
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
Val NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::m_staticError = Val();

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
class NFShmHashTableWithList
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

    typedef NFShmHashTableWithListNode<Val> Node; ///< 节点类型

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

    // 新增：链表管理成员变量
    int m_listHead; ///< 插入顺序链表头节点索引（INVALID_ID表示空链表）
    int m_listTail; ///< 插入顺序链表尾节点索引（INVALID_ID表示空链表）
    bool m_enableLRU; ///< LRU功能开关，当为true时find/count操作会将节点移到链表尾部

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

    typedef NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> iterator;
    typedef NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> const_iterator;

    // 新增：链表迭代器类型定义
    typedef NFShmHashTableWithListListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> list_iterator;
    typedef NFShmHashTableWithListConstListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> const_list_iterator;

    friend struct NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>;
    friend struct NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>;
    friend struct NFShmHashTableWithListListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>;
    friend struct NFShmHashTableWithListConstListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>;

public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 构造函数
     * @note 根据SHM_CREATE_MODE决定创建或恢复模式
     */
    NFShmHashTableWithList();

    /**
     * @brief 拷贝构造函数
     * @param ht 源哈希表
     */
    NFShmHashTableWithList(const NFShmHashTableWithList& ht);

    /**
     * @brief 赋值操作符
     * @param ht 源哈希表
     * @return 自身引用
     */
    NFShmHashTableWithList& operator=(const NFShmHashTableWithList& ht);

    /**
     * @brief 析构函数
     */
    virtual ~NFShmHashTableWithList();

    // ==================== 共享内存特有接口 ====================

    /**
     * @brief 创建模式初始化（共享内存特有）
     * @return 0表示成功，-1表示失败
     * @note 与STL对比：
     *       - STL: 无对应概念，构造函数即完成初始化
     *       - NFShmHashTableWithList: 支持共享内存CREATE/RESUME两阶段初始化
     *       - 用于共享内存首次创建时的初始化
     *       - 初始化桶数组、空闲链表和插入顺序链表
     *       - 所有节点标记为无效状态，建立空闲链表
     *       - 初始化LRU状态和链表头尾指针
     *       - 在SHM_CREATE_MODE下自动调用
     * 
     * 使用示例：
     * ```cpp
     * typedef std::pair<int, std::string> KeyValue;
     * struct KeyExtractor { int operator()(const KeyValue& kv) const { return kv.first; } };
     * 
     * NFShmHashTableWithList<KeyValue, int, 100, std::hash<int>, KeyExtractor, std::equal_to<int>> table;
     * if (table.CreateInit() == 0) {
     *     // 创建成功，可以开始使用
     *     table.insert_unique(KeyValue(42, "value"));
     *     table.enable_lru();  // 启用LRU功能
     * }
     * ```
     */
    int CreateInit();

    /**
     * @brief 恢复模式初始化（共享内存特有）
     * @return 0表示成功，-1表示失败
     * @note 与STL对比：
     *       - STL: 无对应概念
     *       - NFShmHashTableWithList: 用于从已存在的共享内存恢复容器状态
     *       - 不清空现有数据，恢复哈希表和链表结构
     *       - 重新建立节点间的链接关系（哈希链表和插入顺序链表）
     *       - 验证链表完整性和数据一致性
     *       - 恢复LRU状态和链表头尾指针
     *       - 对非平凡构造类型执行placement构造
     *       - 进程重启后恢复共享内存数据使用
     * 
     * 使用示例：
     * ```cpp
     * // 进程重启后恢复哈希表
     * NFShmHashTableWithList<KeyValue, int, 100, std::hash<int>, KeyExtractor, std::equal_to<int>> table;
     * if (table.ResumeInit() == 0) {
     *     // 恢复成功，数据已可用
     *     std::cout << "Recovered " << table.size() << " elements" << std::endl;
     *     
     *     // 检查LRU状态
     *     if (table.is_lru_enabled()) {
     *         std::cout << "LRU mode is enabled" << std::endl;
     *     }
     *     
     *     // 验证链表完整性
     *     if (table.ValidateListIntegrity()) {
     *         std::cout << "List integrity: OK" << std::endl;
     *     }
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
     * @note STL容器没有此接口，固定容量特有
     */
    bool full() const;

    /**
     * @brief 获取剩余容量
     * @return 剩余可用空间
     * @note STL容器没有此接口
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
    void swap(NFShmHashTableWithList& other) noexcept;

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

    // ==================== 链表迭代器接口（新增） ====================

    /**
     * @brief 获取链表起始迭代器（按插入顺序）
     * @return 指向链表第一个元素的迭代器
     * @note 与STL对比：
     *       - STL: 无对应接口，std::unordered_map只有哈希顺序遍历
     *       - NFShmHashTableWithList: 特有功能，按插入顺序遍历
     *       - 提供与begin()不同的遍历方式：插入顺序 vs 哈希顺序
     *       - 时间复杂度：O(1)
     *       - 支持与标准迭代器相同的操作（++、*、->等）
     *       - 适用于FIFO队列、LRU缓存、调试分析等场景
     * 
     * 使用场景对比：
     * ```cpp
     * typedef std::pair<int, std::string> KeyValue;
     * struct KeyExtractor { int operator()(const KeyValue& kv) const { return kv.first; } };
     * 
     * NFShmHashTableWithList<KeyValue, int, 100, 
     *                        std::hash<int>, KeyExtractor, 
     *                        std::equal_to<int>> table;
     * 
     * // 插入数据
     * table.insert_unique(KeyValue(3, "third"));
     * table.insert_unique(KeyValue(1, "first"));
     * table.insert_unique(KeyValue(2, "second"));
     * 
     * // 哈希顺序遍历（类似STL）
     * std::cout << "Hash order:" << std::endl;
     * for (auto it = table.begin(); it != table.end(); ++it) {
     *     std::cout << it->first << ": " << it->second << std::endl;
     * }
     * // 可能输出：1: first, 2: second, 3: third (取决于哈希函数)
     * 
     * // 插入顺序遍历（特有功能）
     * std::cout << "Insert order:" << std::endl;
     * for (auto it = table.list_begin(); it != table.list_end(); ++it) {
     *     std::cout << it->first << ": " << it->second << std::endl;
     * }
     * // 输出：3: third, 1: first, 2: second (严格按插入顺序)
     * 
     * // C++11 范围for循环支持
     * for (const auto& item : table) {
     *     // 哈希顺序遍历
     * }
     * 
     * // 链表遍历需要显式使用迭代器
     * for (auto it = table.list_begin(); it != table.list_end(); ++it) {
     *     // 插入顺序遍历
     * }
     * ```
     */
    list_iterator list_begin();

    /**
     * @brief 获取链表结束迭代器
     * @return 指向链表末尾的迭代器
     */
    list_iterator list_end();

    /**
     * @brief 获取常量链表起始迭代器
     * @return 指向链表第一个元素的常量迭代器
     */
    const_list_iterator list_begin() const;

    /**
     * @brief 获取常量链表结束迭代器
     * @return 指向链表末尾的常量迭代器
     */
    const_list_iterator list_end() const;

    /**
     * @brief 获取常量链表起始迭代器（C++11风格）
     * @return 指向链表第一个元素的常量迭代器
     */
    const_list_iterator list_cbegin() const;

    /**
     * @brief 获取常量链表结束迭代器（C++11风格）
     * @return 指向链表末尾的常量迭代器
     */
    const_list_iterator list_cend() const;

    // ==================== LRU功能控制接口（新增） ====================

    /**
     * @brief 启用LRU功能
     * @note 启用后，find/count操作会将访问的节点移动到链表尾部
     */
    void enable_lru() { m_enableLRU = true; }

    /**
     * @brief 禁用LRU功能
     * @note 禁用后，find/count操作不会移动节点位置
     */
    void disable_lru() { m_enableLRU = false; }

    /**
     * @brief 检查LRU功能是否启用
     * @return true表示启用，false表示禁用
     */
    bool is_lru_enabled() const { return m_enableLRU; }

    // ==================== 友元比较操作符 ====================

    template <class Vl, class Ky, int MaxSize, class HF, class Ex, class Eq>
    friend bool operator==(const NFShmHashTableWithList<Vl, Ky, MaxSize, HF, Ex, Eq>&,
                           const NFShmHashTableWithList<Vl, Ky, MaxSize, HF, Ex, Eq>&);

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
     * @note 与STL unordered_map::insert()兼容
     */
    std::pair<iterator, bool> insert_unique(const value_type& obj);

    /**
     * @brief 插入允许重复的元素
     * @param obj 要插入的值
     * @return 指向插入元素的迭代器
     * @note 与STL unordered_multimap::insert()兼容
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
     *       - STL: std::unordered_map::find() 行为基本一致
     *       - NFShmHashTableWithList: 额外的LRU功能支持
     *       - 时间复杂度：O(1)平均，O(n)最坏（链表长度）
     *       - 未找到时返回end()，而非抛出异常
     *       - **LRU增强**：当启用LRU时，查找成功会将节点移到链表尾部
     *       - 支持自定义哈希函数和比较函数
     * 
     * LRU行为说明：
     * - LRU禁用时：查找操作不改变插入顺序链表
     * - LRU启用时：查找成功的元素自动移到链表尾部（标记为最近使用）
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法（无LRU功能）
     * std::unordered_map<std::string, int> stdMap;
     * stdMap["apple"] = 100;
     * auto it1 = stdMap.find("apple");
     * if (it1 != stdMap.end()) {
     *     std::cout << "Found: " << it1->second << std::endl;
     * }
     * 
     * // NFShmHashTableWithList用法（支持LRU）
     * typedef std::pair<std::string, int> KeyValue;
     * struct KeyExtractor { const std::string& operator()(const KeyValue& kv) const { return kv.first; } };
     * 
     * NFShmHashTableWithList<KeyValue, std::string, 100, 
     *                        std::hash<std::string>, KeyExtractor, 
     *                        std::equal_to<std::string>> table;
     * table.insert_unique(KeyValue("apple", 100));
     * 
     * // 不启用LRU（类似STL行为）
     * table.disable_lru();
     * auto it2 = table.find("apple");  // 查找不影响插入顺序
     * 
     * // 启用LRU（增强功能）
     * table.enable_lru();
     * auto it3 = table.find("apple");  // 查找成功，"apple"移到链表尾部
     * if (it3 != table.end()) {
     *     std::cout << "Found: " << it3->second << std::endl;
     *     // 此时"apple"成为最近访问的元素
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
     *       - NFShmHashTableWithList: 额外的LRU功能支持
     *       - 对于insert_unique插入的元素，结果为0或1
     *       - 对于insert_equal插入的元素，可能大于1
     *       - 时间复杂度：O(1)平均，O(n)最坏
     *       - **LRU增强**：当启用LRU时，统计过程中会触发LRU更新
     * 
     * LRU行为说明：
     * - LRU禁用时：统计操作不改变插入顺序链表
     * - LRU启用时：每个匹配的元素都会移到链表尾部
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法
     * std::unordered_multimap<int, std::string> stdMultiMap;
     * stdMultiMap.insert({42, "first"});
     * stdMultiMap.insert({42, "second"});
     * std::cout << "Count: " << stdMultiMap.count(42) << std::endl;  // 输出：2
     * 
     * // NFShmHashTableWithList用法（支持LRU）
     * typedef std::pair<int, std::string> KeyValue;
     * struct KeyExtractor { int operator()(const KeyValue& kv) const { return kv.first; } };
     * 
     * NFShmHashTableWithList<KeyValue, int, 100, 
     *                        std::hash<int>, KeyExtractor, 
     *                        std::equal_to<int>> table;
     * table.insert_equal(KeyValue(42, "first"));
     * table.insert_equal(KeyValue(42, "second"));
     * 
     * // 不启用LRU（类似STL行为）
     * table.disable_lru();
     * std::cout << "Count: " << table.count(42) << std::endl;  // 输出：2，不影响插入顺序
     * 
     * // 启用LRU（增强功能）
     * table.enable_lru();
     * std::cout << "Count: " << table.count(42) << std::endl;  // 输出：2，所有匹配元素移到链表尾部
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

    value_type& at(const key_type& key);
    const value_type& at(const key_type& key) const;

    // ==================== 删除接口（STL兼容） ====================

    /**
     * @brief 根据键删除元素
     * @param key 要删除的键
     * @return 删除的元素数量
     * @note 与STL unordered_map::erase()兼容
     */
    size_type erase(const key_type& key);

    /**
     * @brief 根据迭代器删除元素
     * @param it 指向要删除元素的迭代器
     * @return 指向下一个元素的迭代器
     * @note 与STL unordered_map::erase()兼容
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
     *       - STL: std::unordered_map::clear() 行为基本一致
     *       - NFShmHashTableWithList: 额外清理插入顺序链表和LRU状态
     *       - 删除所有元素，重置为空状态
     *       - 所有节点回收到空闲链表，可重复使用
     *       - 桶数组重置为空状态，所有桶指向-1
     *       - **链表特有**：重置插入顺序链表头尾指针
     *       - **LRU特有**：保持LRU开关状态不变
     *       - 时间复杂度：O(n)，n为当前元素数量
     *       - 操作后size()返回0，full()返回false
     * 
     * 清理内容对比：
     * ```
     * STL clear()清理内容：
     * - 删除所有元素
     * - 重置桶数组
     * - 释放动态分配的内存
     * 
     * NFShmHashTableWithList clear()清理内容：
     * - 删除所有元素 ✓
     * - 重置桶数组 ✓
     * - 回收节点到空闲链表 ✓ (而非释放内存)
     * - 重置插入顺序链表 ✓ (STL无此功能)
     * - 保持LRU开关状态 ✓ (STL无此功能)
     * - 保持固定容量不变 ✓ (STL可能改变桶数量)
     * ```
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
     * // NFShmHashTableWithList用法
     * typedef std::pair<int, std::string> KeyValue;
     * struct KeyExtractor { int operator()(const KeyValue& kv) const { return kv.first; } };
     * 
     * NFShmHashTableWithList<KeyValue, int, 100, 
     *                        std::hash<int>, KeyExtractor, 
     *                        std::equal_to<int>> table;
     * 
     * table.enable_lru();  // 启用LRU
     * table.insert_unique(KeyValue(1, "one"));
     * table.insert_unique(KeyValue(2, "two"));
     * 
     * // 清空前状态检查
     * assert(table.size() == 2);
     * assert(!table.empty());
     * assert(table.is_lru_enabled());
     * 
     * table.clear();  // 清空所有元素和链表
     * 
     * // 清空后状态检查
     * assert(table.empty());
     * assert(table.size() == 0);
     * assert(table.left_size() == table.max_size());
     * assert(table.is_lru_enabled());  // LRU状态保持不变
     * assert(table.list_begin() == table.list_end());  // 链表为空
     * 
     * // 可以立即重新使用
     * table.insert_unique(KeyValue(3, "three"));
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

    /**
     * @brief 打印链表信息
     * @note 调试用，专门显示插入顺序链表的详细信息
     */
    void print_list() const;

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
    void CopyFrom(const NFShmHashTableWithList& ht);

    // ==================== 链表管理内部函数（新增） ====================

    /**
     * @brief 将节点添加到链表尾部
     * @param pNode 要添加的节点
     * @note 用于新插入的节点
     */
    void AddToListTail(Node* pNode);

    /**
     * @brief 从链表中移除节点
     * @param pNode 要移除的节点
     * @note 用于删除节点时
     */
    void RemoveFromList(Node* pNode);

    /**
     * @brief 将节点移动到链表尾部
     * @param pNode 要移动的节点
     * @note 用于LRU访问时
     */
    void MoveToListTail(Node* pNode);

    /**
     * @brief 初始化链表管理变量
     * @note 在InitializeBuckets中调用
     */
    void InitializeList();

    /**
     * @brief 验证链表完整性（调试用）
     * @return true表示链表完整性正确，false表示存在问题
     * @note 用于调试和错误诊断
     */
    bool ValidateListIntegrity() const;
};

// ==================== 静态成员定义 ====================

template <class Val, class Key, int MAX_SIZE, class Hf, class Ex, class Eq>
Val NFShmHashTableWithList<Val, Key, MAX_SIZE, Hf, Ex, Eq>::m_staticError = Val();

// ==================== 迭代器实现 ====================

/**
 * @brief 迭代器前置递增操作符实现
 * @note 移动到下一个有效元素，跨桶遍历
 */
template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&
NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>::operator++()
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
NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>
NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>::operator++(int)
{
    iterator tmp = *this;
    ++*this;
    return tmp;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&
NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>::operator++()
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
NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>
NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>::operator++(int)
{
    const_iterator tmp = *this;
    ++*this;
    return tmp;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Node* NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::CreateNode()
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
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::RecycleNode(Node* p)
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
NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::NFShmHashTableWithList()
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
NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::NFShmHashTableWithList(const NFShmHashTableWithList& ht)
{
    m_hash = HashFcn();
    m_equals = EqualKey();
    m_getKey = ExtractKey();
    CreateInit();
    CopyFrom(ht);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>& NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::operator=(const NFShmHashTableWithList& ht)
{
    if (&ht != this)
    {
        CopyFrom(ht);
    }
    return *this;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::~NFShmHashTableWithList()
{
    clear();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
int NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::CreateInit()
{
    InitializeBuckets();
    InitializeList(); // 新增：初始化链表管理变量
    m_enableLRU = false; // 新增：默认禁用LRU功能
    m_init = EN_NF_SHM_STL_INIT_OK;
    return 0;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
int NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::ResumeInit()
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
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Init()
{
    new(this) NFShmHashTableWithList();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::max_size() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, MAX_SIZE, "not init, TRACE_STACK:%s", TRACE_STACK());
    return MAX_SIZE;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
bool NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::empty() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, true, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size == 0;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
bool NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::full() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size == MAX_SIZE;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
size_t NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::left_size() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size >= MAX_SIZE ? 0 : MAX_SIZE - m_size;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Node* NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::GetValidNode(int idx)
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
const typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Node* NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::GetValidNode(int idx) const
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
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::get_iterator(int idx)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(idx >= 0 && idx < MAX_SIZE, iterator(nullptr, this), "Index out of range: %d, TRACE_STACK:%s", idx, TRACE_STACK());
    return iterator(GetValidNode(idx), this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::get_iterator(int idx) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(idx >= 0 && idx < MAX_SIZE, const_iterator(nullptr, this), "Index out of range: %d, TRACE_STACK:%s", idx, TRACE_STACK());
    return const_iterator(GetValidNode(idx), this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::swap(NFShmHashTableWithList& other) noexcept
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "this not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(other.m_init == EN_NF_SHM_STL_INIT_OK, , "other not init, TRACE_STACK:%s", TRACE_STACK());

    if (this != &other)
    {
        NFShmHashTableWithList temp(*this);
        CopyFrom(other);
        other.CopyFrom(temp);
    }
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::begin()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    for (size_type n = 0; n < MAX_SIZE; ++n)
        if (m_bucketsFirstIdx[n] != INVALID_ID)
            return iterator(GetValidNode(m_bucketsFirstIdx[n]), this);
    return end();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::end()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return iterator(nullptr, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::begin() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    for (size_type n = 0; n < MAX_SIZE; ++n)
        if (m_bucketsFirstIdx[n] != INVALID_ID)
            return const_iterator(GetValidNode(m_bucketsFirstIdx[n]), this);
    return end();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::end() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return const_iterator(nullptr, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::bucket_count() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return MAX_SIZE;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::max_bucket_count() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return MAX_SIZE;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::elems_in_bucket(size_type bucket) const
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
std::pair<typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator, bool> NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(const value_type& obj)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, (std::pair<iterator, bool>(iterator(nullptr, this), false)), "not init, TRACE_STACK:%s", TRACE_STACK());
    return insert_unique_noresize(obj);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(const value_type& obj)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return insert_equal_noresize(obj);
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
std::forward_iterator_tag iterator_category(const NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return std::forward_iterator_tag();
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
Val* value_type(const NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, ExK, Eqk>::difference_type* distance_type(const NFShmHashTableWithListIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
std::forward_iterator_tag iterator_category(const NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return std::forward_iterator_tag();
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
Val* value_type(const NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HF, class ExK, class Eqk>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, ExK, Eqk>::difference_type* distance_type(const NFShmHashTableWithListConstIterator<Val, Key, MAX_SIZE, HF, ExK, Eqk>&)
{
    return nullptr;
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
bool operator==(const NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht1,
                const NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht2)
{
    if (ht1.size() != ht2.size()) return false;

    typedef typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::Node Node;
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
bool operator!=(const NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht1,
                const NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht2)
{
    return !(ht1 == ht2);
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void swap(NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht1, NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>& ht2) noexcept
{
    ht1.swap(ht2);
}


template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
std::pair<typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator, bool> NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::insert_unique_noresize(const value_type& obj)
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
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::insert_equal_noresize(const value_type& obj)
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
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(InputIterator f, InputIterator l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    insert_unique(f, l, typename std::iterator_traits<InputIterator>::iterator_category());
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class InputIterator>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(InputIterator f, InputIterator l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    insert_equal(f, l, typename std::iterator_traits<InputIterator>::iterator_category());
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class InputIterator>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(InputIterator f, InputIterator l, std::input_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (; f != l; ++f)
        insert_unique(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class InputIterator>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(InputIterator f, InputIterator l, std::input_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (; f != l; ++f)
        insert_equal(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class ForwardIterator>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(ForwardIterator f, ForwardIterator l, std::forward_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = std::distance(f, l);
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTableWithList does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_unique_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
template <class ForwardIterator>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(ForwardIterator f, ForwardIterator l, std::forward_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = std::distance(f, l);
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTableWithList does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_equal_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(const value_type* f, const value_type* l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = l - f;
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTableWithList does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_unique_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(const value_type* f, const value_type* l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = l - f;
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTableWithList does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_equal_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_unique(const_iterator f, const_iterator l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = std::distance(f, l);
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTableWithList does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_unique_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::insert_equal(const_iterator f, const_iterator l)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_type n = std::distance(f, l);
    size_type left = left_size();
    if (left < n)
    {
        LOG_WARN(0, -1, "NFShmHashTableWithList does not have enough space: (left:%lu, insert:%lu), only insert left:%lu, TRACE_STACK:%s", left, n, left, TRACE_STACK());
        n = left;
    }
    for (; n > 0; --n, ++f)
        insert_equal_noresize(*f);
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::reference NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::find_or_insert(const value_type& obj)
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
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::find(const key_type& key)
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

    // 新增：如果启用LRU功能且找到节点，将节点移动到链表尾部
    if (first && m_enableLRU)
    {
        MoveToListTail(first);
    }

    return iterator(first, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::find(const key_type& key) const
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

    // 新增：如果启用LRU功能且找到节点，将节点移动到链表尾部
    if (first && m_enableLRU)
    {
        (const_cast<NFShmHashTableWithList*>(this))->MoveToListTail(const_cast<Node*>(first));
    }

    return const_iterator(first, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::count(const key_type& key) const
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

            // 新增：如果启用LRU功能且找到节点，将最后一个匹配的节点移动到链表尾部
            if (m_enableLRU)
            {
                const_cast<NFShmHashTableWithList*>(this)->MoveToListTail(const_cast<Node*>(cur));
            }
        }
    }

    return result;
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
std::pair<typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator, typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator> NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::equal_range(const key_type& key)
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
            // 新增：如果启用LRU功能，将找到的第一个匹配节点移动到链表尾部
            if (m_enableLRU)
            {
                MoveToListTail(first);
            }

            for (Node* cur = GetValidNode(first->m_next); cur; cur = GetValidNode(cur->m_next))
            {
                if (!m_equals(m_getKey(cur->m_value), key))
                {
                    return pii(iterator(first, this), iterator(cur, this));
                }
                
                // 新增：如果启用LRU功能，将找到的每个匹配节点移动到链表尾部
                if (m_enableLRU)
                {
                    MoveToListTail(cur);
                }
            }
            for (size_type m = n + 1; m < MAX_SIZE; ++m)
            {
                if (m_bucketsFirstIdx[m] != INVALID_ID)
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
std::pair<typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::const_iterator, typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::const_iterator>
NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::equal_range(const key_type& key) const
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
            // 新增：如果启用LRU功能，将找到的第一个匹配节点移动到链表尾部
            if (m_enableLRU)
            {
                (const_cast<NFShmHashTableWithList*>(this))->MoveToListTail(const_cast<Node*>(first));
            }

            for (const Node* cur = GetValidNode(first->m_next); cur; cur = GetValidNode(cur->m_next))
            {
                if (!m_equals(m_getKey(cur->m_value), key))
                {
                    return pii(const_iterator(first, this), const_iterator(cur, this));
                }
                
                // 新增：如果启用LRU功能，将找到的每个匹配节点移动到链表尾部
                if (m_enableLRU)
                {
                    (const_cast<NFShmHashTableWithList*>(this))->MoveToListTail(const_cast<Node*>(cur));
                }
            }
            for (size_type m = n + 1; m < MAX_SIZE; ++m)
            {
                if (m_bucketsFirstIdx[m] != INVALID_ID)
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
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(const key_type& key)
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
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(iterator it)
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
        while (currentNode && currentNode->m_next != INVALID_ID)
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
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(iterator first, iterator last)
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
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(const_iterator first, const_iterator last)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
    return erase(iterator(const_cast<Node*>(first.m_curNode),
                          const_cast<NFShmHashTableWithList*>(first.m_hashTable)),
                 iterator(const_cast<Node*>(last.m_curNode),
                          const_cast<NFShmHashTableWithList*>(last.m_hashTable)));
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::erase(const_iterator it)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
    return erase(iterator(const_cast<Node*>(it.m_curNode),
                          const_cast<NFShmHashTableWithList*>(it.m_hashTable)));
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>
::resize(size_type)
{
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::EraseBucket(const size_type n, Node* first, Node* last)
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
             cur = next, next = GetValidNode(cur->m_next))
        {
        }
        while (next != last)
        {
            cur->m_next = next->m_next;
            DeleteNode(next);
            next = GetValidNode(cur->m_next);
        }
    }
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::EraseBucket(const size_type n, Node* last)
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

        m_bucketsFirstIdx[n] = cur ? cur->m_self : INVALID_ID;
    }
}

template <class Val, class Key, int MAX_SIZE, class HF, class Ex, class Eq>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::clear()
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

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
    InitializeList(); // 新增：重置链表状态
}


template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::print_structure() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== NFShmHashTableWithList Structure ===\n");
    printf("Size: %zu, Max Size: %d, First Free Index: %d\n", m_size, MAX_SIZE, m_firstFreeIdx);
    printf("LRU enabled: %s, List head: %d, List tail: %d\n",
           m_enableLRU ? "Yes" : "No", m_listHead, m_listTail);

    // 验证链表完整性
    bool listIntegrityOK = ValidateListIntegrity();
    printf("List integrity: %s\n", listIntegrityOK ? "OK" : "FAILED");

    printf("=====================================\n");

    size_type totalNodes = 0;
    size_type emptyBuckets = 0;

    for (size_type i = 0; i < MAX_SIZE; ++i)
    {
        int firstIdx = m_bucketsFirstIdx[i];
        if (firstIdx == INVALID_ID)
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

            if (cur->m_next != INVALID_ID)
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
    if (m_firstFreeIdx == INVALID_ID)
    {
        printf("Empty\n");
    }
    else
    {
        auto pBuckets = GetBuckets();
        int freeIdx = m_firstFreeIdx;
        size_type freeCount = 0;
        size_type maxFreeCount = MAX_SIZE; // Prevent infinite loop

        while (freeIdx != INVALID_ID && freeCount < maxFreeCount)
        {
            printf("[%d]", freeIdx);
            freeCount++;

            if (freeIdx >= 0 && freeIdx < MAX_SIZE)
            {
                freeIdx = pBuckets[freeIdx].m_next;
                if (freeIdx != INVALID_ID)
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

    // 打印链表顺序（如果链表不为空）
    printf("\n=== Insertion Order List (Linked List) ===\n");
    if (m_listHead != INVALID_ID)
    {
        printf("List Head: %d, List Tail: %d\n", m_listHead, m_listTail);
        printf("Insertion order: ");
        auto pNode = GetValidNode(m_listHead);
        size_type listCount = 0;
        size_type maxListCount = m_size + 1;

        while (pNode && listCount < maxListCount)
        {
            printf("[%zu", pNode->m_self);

            // 打印键值信息
            try
            {
                auto key = m_getKey(pNode->m_value);
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

            // 显示链表指针信息
            printf(",p:%d,n:%d", pNode->m_listPrev, pNode->m_listNext);
            printf("]");

            listCount++;

            if (pNode->m_listNext != INVALID_ID)
            {
                printf(" -> ");
                pNode = GetValidNode(pNode->m_listNext);
            }
            else
            {
                break;
            }
        }

        if (listCount >= maxListCount)
        {
            printf(" ... (List loop detected!)");
        }

        printf("\n");
        printf("List statistics: %zu nodes", listCount);

        if (listCount != m_size)
        {
            printf(" (Warning: List count %zu != hash size %zu)", listCount, m_size);
        }

        printf("\n");

        // 打印反向链表验证
        printf("Reverse order verification: ");
        auto pTailNode = GetValidNode(m_listTail);
        size_type reverseCount = 0;

        while (pTailNode && reverseCount < maxListCount)
        {
            printf("[%zu]", pTailNode->m_self);
            reverseCount++;

            if (pTailNode->m_listPrev != INVALID_ID)
            {
                if (reverseCount < 5) // 只显示前几个，避免输出太长
                {
                    printf(" <- ");
                    pTailNode = GetValidNode(pTailNode->m_listPrev);
                }
                else
                {
                    printf(" <- ...");
                    break;
                }
            }
            else
            {
                break;
            }
        }

        if (reverseCount >= maxListCount)
        {
            printf(" (Reverse loop detected!)");
        }

        printf(" (%zu nodes)\n", reverseCount);

        if (reverseCount != listCount)
        {
            printf("Warning: Forward count %zu != Reverse count %zu\n", listCount, reverseCount);
        }
    }
    else
    {
        printf("List is empty (Head: %d, Tail: %d)\n", m_listHead, m_listTail);

        if (m_listTail != INVALID_ID)
        {
            printf("Warning: Head is INVALID_ID but tail is %d\n", m_listTail);
        }
    }

    printf("LRU Mode: %s\n", m_enableLRU ? "Enabled" : "Disabled");
    printf("==========================================\n");
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::print_detailed() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== NFShmHashTableWithList Detailed View ===\n");
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
        if (firstIdx == INVALID_ID) continue;

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

            if (cur->m_next != INVALID_ID)
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
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::print_simple() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== Hash Table Simple View ===\n");
    printf("Size: %zu/%d, Free head: %d\n", m_size, MAX_SIZE, m_firstFreeIdx);

    // Only show non-empty buckets
    size_type nonEmptyBuckets = 0;
    for (size_type i = 0; i < MAX_SIZE; ++i)
    {
        if (m_bucketsFirstIdx[i] != INVALID_ID)
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

                if (cur->m_next != INVALID_ID)
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
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::InitializeBuckets()
{
    m_size = 0;
    m_firstFreeIdx = 0;
    auto pBuckets = GetBuckets();
    for (int i = 0; i < MAX_SIZE; ++i)
    {
        pBuckets[i].m_next = i + 1;
        pBuckets[i].m_valid = false;
        pBuckets[i].m_self = i;
        pBuckets[i].m_listPrev = INVALID_ID;
        pBuckets[i].m_listNext = INVALID_ID;
    }

    pBuckets[MAX_SIZE - 1].m_next = INVALID_ID;

    for (int i = 0; i < MAX_SIZE; ++i)
    {
        m_bucketsFirstIdx[i] = INVALID_ID;
    }
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::BktNumKey(const key_type& key) const
{
    return BktNumKey(key, MAX_SIZE);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::BktNum(const value_type& obj) const
{
    return BktNumKey(m_getKey(obj));
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::BktNumKey(const key_type& key, size_t n) const
{
    CHECK_EXPR(n > 0, 0, "Bucket count cannot be zero, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(n <= MAX_SIZE, 0, "Bucket count %zu exceeds MAX_SIZE %d, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());

    size_t hashValue = m_hash(key);
    return hashValue % n;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::size_type NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::MBktNum(const value_type& obj, size_t n) const
{
    return BktNumKey(m_getKey(obj), n);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::Node* NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::NewNode(const value_type& obj)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), nullptr, "HashTable is full, cannot create new node, TRACE_STACK:%s", TRACE_STACK());

    Node* pNode = CreateNode();
    if (pNode)
    {
        CHECK_EXPR(pNode->m_valid == false, nullptr, "Node should be invalid before initialization, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(pNode->m_self >= 0 && pNode->m_self < MAX_SIZE, nullptr, "Node self index out of range: %zu, TRACE_STACK:%s", pNode->m_self, TRACE_STACK());

        pNode->m_valid = true;
        pNode->m_next = INVALID_ID;

        try
        {
            std::_Construct(&pNode->m_value, obj);

            // 新增：将新创建的节点添加到链表尾部
            AddToListTail(pNode);
        }
        catch (...)
        {
            // 如果构造失败，回滚状态
            pNode->m_valid = false;
            pNode->m_next = INVALID_ID;
            RecycleNode(pNode);
            return nullptr;
        }
    }

    return pNode;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::DeleteNode(Node* pNode)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode != nullptr, "Node pointer is null, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_valid, "Node is already invalid, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_self >= 0 && pNode->m_self < MAX_SIZE, "Node self index out of range: %zu, TRACE_STACK:%s", pNode->m_self, TRACE_STACK());

    // 新增：在删除节点前先从链表中移除
    RemoveFromList(pNode);

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
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HF, Ex, Eq>::CopyFrom(const NFShmHashTableWithList& ht)
{
    if (this == &ht) return;

    clear();

    // 复制LRU设置
    m_enableLRU = ht.m_enableLRU;

    // 按原始链表顺序复制元素以保持插入顺序
    auto pListNode = ht.GetValidNode(ht.m_listHead);
    while (pListNode)
    {
        // NewNode会自动将节点添加到链表尾部，保持插入顺序
        insert_equal(pListNode->m_value);
        pListNode = ht.GetValidNode(pListNode->m_listNext);
    }
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::value_type& NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::at(const key_type& key)
{
    iterator it = find(key);
    if (it == end())
    {
        LOG_ERR(0, -1, "NFShmHashTableWithList::at: key not found, TRACE_STACK:%s", TRACE_STACK());
        return m_staticError;
    }
    return *it;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
const typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::value_type& NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::at(const key_type& key) const
{
    const_iterator it = find(key);
    if (it == end())
    {
        LOG_ERR(0, -1, "NFShmHashTableWithList::at: key not found, TRACE_STACK:%s", TRACE_STACK());
        return m_staticError;
    }
    return *it;
}

// ==================== 链表迭代器实现 ====================

/**
 * @brief Hash表链表迭代器（按插入顺序遍历）
 * @tparam Val 值类型
 * @tparam Key 键类型
 * @tparam MAX_SIZE 最大容量
 * @tparam HashFcn 哈希函数类型
 * @tparam ExtractKey 键提取函数类型
 * @tparam EqualKey 键比较函数类型
 *
 * 用于按插入顺序遍历哈希表中的元素，提供链表式的访问接口
 */
template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableWithListListIterator
{
    // ==================== STL兼容类型定义 ====================
    typedef NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> Hashtable;
    typedef NFShmHashTableWithListListIterator list_iterator;
    typedef NFShmHashTableWithListConstListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> const_list_iterator;
    typedef NFShmHashTableWithListNode<Val> Node;

    typedef std::forward_iterator_tag iterator_category;
    typedef Val value_type;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef Val& reference;
    typedef Val* pointer;

    // ==================== 成员变量 ====================
    Node* m_curNode; ///< 当前节点指针
    Hashtable* m_hashTable; ///< 所属哈希表指针
    static value_type m_staticError; ///< 错误时返回的静态对象

    // ==================== 构造函数 ====================
    NFShmHashTableWithListListIterator(Node* n, Hashtable* tab)
        : m_curNode(n), m_hashTable(tab)
    {
    }

    NFShmHashTableWithListListIterator(): m_curNode(nullptr), m_hashTable(nullptr)
    {
    }

    // ==================== STL兼容操作符 ====================
    reference operator*() const
    {
        CHECK_EXPR(m_curNode != nullptr, m_staticError, "List iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, m_staticError, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, m_staticError, "List iterator points to invalid node, TRACE_STACK:%s", TRACE_STACK());
        return m_curNode->m_value;
    }

    pointer operator->() const
    {
        CHECK_EXPR(m_curNode != nullptr, &m_staticError, "List iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, &m_staticError, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, &m_staticError, "List iterator points to invalid node, TRACE_STACK:%s", TRACE_STACK());
        return &(m_curNode->m_value);
    }

    /// @brief 前置递增操作符，移动到链表中下一个元素
    list_iterator& operator++()
    {
        CHECK_EXPR(m_curNode != nullptr, *this, "List iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, *this, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, *this, "Current node is invalid, TRACE_STACK:%s", TRACE_STACK());

        if (m_curNode->m_listNext != INVALID_ID)
        {
            CHECK_EXPR(m_curNode->m_listNext >= 0 && m_curNode->m_listNext < MAX_SIZE, *this, "Next node index out of range: %d, valid range [0, %d), TRACE_STACK:%s", m_curNode->m_listNext, MAX_SIZE, TRACE_STACK());
            auto pNextNode = m_hashTable->GetValidNode(m_curNode->m_listNext);
            CHECK_EXPR(pNextNode != nullptr, *this, "Next node is null at index %d, TRACE_STACK:%s", m_curNode->m_listNext, TRACE_STACK());
            CHECK_EXPR(pNextNode->m_listPrev == m_curNode->m_self, *this, "Next node prev mismatch: expected %zu, got %d, TRACE_STACK:%s", m_curNode->m_self, pNextNode->m_listPrev, TRACE_STACK());
            m_curNode = pNextNode;
        }
        else
        {
            // 验证当前节点确实是尾节点
            CHECK_EXPR(m_hashTable->m_listTail == INVALID_ID || m_curNode->m_self == m_hashTable->m_listTail, *this, "Node claims to be tail but list tail is different: node:%zu, tail:%d, TRACE_STACK:%s", m_curNode->m_self, m_hashTable->m_listTail, TRACE_STACK());
            m_curNode = nullptr; // 已到达链表尾部
        }
        return *this;
    }

    /// @brief 后置递增操作符
    list_iterator operator++(int)
    {
        list_iterator tmp = *this;
        ++*this;
        return tmp;
    }

    /// @brief 相等比较操作符
    bool operator==(const list_iterator& it) const { return m_curNode == it.m_curNode; }

    /// @brief 不等比较操作符
    bool operator!=(const list_iterator& it) const { return m_curNode != it.m_curNode; }
};

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
Val NFShmHashTableWithListListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::m_staticError = Val();

/**
 * @brief Hash表常量链表迭代器
 * @note 与非常量链表迭代器类似，但提供const访问
 */
template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
struct NFShmHashTableWithListConstListIterator
{
    // ==================== STL兼容类型定义 ====================
    typedef NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> Hashtable;
    typedef NFShmHashTableWithListListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey> list_iterator;
    typedef NFShmHashTableWithListConstListIterator const_list_iterator;
    typedef NFShmHashTableWithListNode<Val> Node;

    typedef std::forward_iterator_tag iterator_category;
    typedef Val value_type;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef const Val& reference;
    typedef const Val* pointer;

    // ==================== 成员变量 ====================
    const Node* m_curNode; ///< 当前节点常量指针
    const Hashtable* m_hashTable; ///< 所属哈希表常量指针
    static value_type m_staticError;

    // ==================== 构造函数 ====================
    NFShmHashTableWithListConstListIterator(const Node* n, const Hashtable* tab)
        : m_curNode(n), m_hashTable(tab)
    {
    }

    NFShmHashTableWithListConstListIterator(): m_curNode(nullptr), m_hashTable(nullptr)
    {
    }

    /**
     * @brief 从非常量链表迭代器构造
     * @param it 非常量链表迭代器
     */
    NFShmHashTableWithListConstListIterator(const list_iterator& it)
        : m_curNode(it.m_curNode), m_hashTable(it.m_hashTable)
    {
    }

    // ==================== STL兼容操作符 ====================
    reference operator*() const
    {
        CHECK_EXPR(m_curNode != nullptr, m_staticError, "Const list iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, m_staticError, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, m_staticError, "Const list iterator points to invalid node, TRACE_STACK:%s", TRACE_STACK());
        return m_curNode->m_value;
    }

    pointer operator->() const
    {
        CHECK_EXPR(m_curNode != nullptr, &m_staticError, "Const list iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, &m_staticError, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, &m_staticError, "Const list iterator points to invalid node, TRACE_STACK:%s", TRACE_STACK());
        return &(m_curNode->m_value);
    }

    const_list_iterator& operator++()
    {
        CHECK_EXPR(m_curNode != nullptr, *this, "Const list iterator is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_hashTable != nullptr, *this, "HashTable is null, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_curNode->m_valid, *this, "Current node is invalid, TRACE_STACK:%s", TRACE_STACK());

        if (m_curNode->m_listNext != INVALID_ID)
        {
            CHECK_EXPR(m_curNode->m_listNext >= 0 && m_curNode->m_listNext < MAX_SIZE, *this, "Next node index out of range: %d, valid range [0, %d), TRACE_STACK:%s", m_curNode->m_listNext, MAX_SIZE, TRACE_STACK());
            auto pNextNode = m_hashTable->GetValidNode(m_curNode->m_listNext);
            CHECK_EXPR(pNextNode != nullptr, *this, "Next node is null at index %d, TRACE_STACK:%s", m_curNode->m_listNext, TRACE_STACK());
            CHECK_EXPR(pNextNode->m_listPrev == m_curNode->m_self, *this, "Next node prev mismatch: expected %zu, got %d, TRACE_STACK:%s", m_curNode->m_self, pNextNode->m_listPrev, TRACE_STACK());
            m_curNode = pNextNode;
        }
        else
        {
            // 验证当前节点确实是尾节点
            CHECK_EXPR(m_hashTable->m_listTail == INVALID_ID || m_curNode->m_self == m_hashTable->m_listTail, *this, "Node claims to be tail but list tail is different: node:%zu, tail:%d, TRACE_STACK:%s", m_curNode->m_self, m_hashTable->m_listTail, TRACE_STACK());
            m_curNode = nullptr; // 已到达链表尾部
        }
        return *this;
    }

    const_list_iterator operator++(int)
    {
        const_list_iterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const const_list_iterator& it) const { return m_curNode == it.m_curNode; }
    bool operator!=(const const_list_iterator& it) const { return m_curNode != it.m_curNode; }
};

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
Val NFShmHashTableWithListConstListIterator<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::m_staticError = Val();

// ==================== 链表管理函数实现 ====================

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::InitializeList()
{
    m_listHead = INVALID_ID;
    m_listTail = INVALID_ID;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::AddToListTail(Node* pNode)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "HashTable not initialized, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode != nullptr, "Node pointer is null, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_valid, "Node is invalid, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_self >= 0 && pNode->m_self < MAX_SIZE, "Node self index out of range: %zu, valid range [0, %d), TRACE_STACK:%s", pNode->m_self, MAX_SIZE, TRACE_STACK());

    // 检查节点是否已经在链表中
    CHECK_EXPR_RE_VOID(pNode->m_listPrev == INVALID_ID && pNode->m_listNext == INVALID_ID, "Node is already in list, prev:%d, next:%d, TRACE_STACK:%s", pNode->m_listPrev, pNode->m_listNext, TRACE_STACK());

    // 检查链表状态一致性
    if (m_listHead == INVALID_ID)
    {
        CHECK_EXPR_RE_VOID(m_listTail == INVALID_ID, "List head is INVALID_ID but tail is not INVALID_ID: %d, TRACE_STACK:%s", m_listTail, TRACE_STACK());
    }
    else
    {
        CHECK_EXPR_RE_VOID(m_listTail != INVALID_ID, "List head is not INVALID_ID but tail is INVALID_ID, head:%d, TRACE_STACK:%s", m_listHead, TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_listHead >= 0 && m_listHead < MAX_SIZE, "List head index out of range: %d, valid range [0, %d), TRACE_STACK:%s", m_listHead, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_listTail >= 0 && m_listTail < MAX_SIZE, "List tail index out of range: %d, valid range [0, %d), TRACE_STACK:%s", m_listTail, MAX_SIZE, TRACE_STACK());
    }

    pNode->m_listPrev = m_listTail;
    pNode->m_listNext = INVALID_ID;

    if (m_listTail != INVALID_ID)
    {
        // 链表不为空，更新尾节点的next指针
        auto pTailNode = GetValidNode(m_listTail);
        CHECK_EXPR_RE_VOID(pTailNode != nullptr, "Tail node is null at index %d, TRACE_STACK:%s", m_listTail, TRACE_STACK());
        CHECK_EXPR_RE_VOID(pTailNode->m_listNext == INVALID_ID, "Tail node next should be INVALID_ID but is %d, TRACE_STACK:%s", pTailNode->m_listNext, TRACE_STACK());

        pTailNode->m_listNext = pNode->m_self;
    }
    else
    {
        // 链表为空，更新头指针
        m_listHead = pNode->m_self;
    }

    m_listTail = pNode->m_self;

    // 验证链表操作后的状态
    CHECK_EXPR_RE_VOID(m_listHead != INVALID_ID && m_listTail != INVALID_ID, "List head or tail is INVALID_ID after adding node, head:%d, tail:%d, TRACE_STACK:%s", m_listHead, m_listTail, TRACE_STACK());
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::RemoveFromList(Node* pNode)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "HashTable not initialized, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode != nullptr, "Node pointer is null, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_self >= 0 && pNode->m_self < MAX_SIZE, "Node self index out of range: %zu, valid range [0, %d), TRACE_STACK:%s", pNode->m_self, MAX_SIZE, TRACE_STACK());

    // 检查链表不为空
    CHECK_EXPR_RE_VOID(m_listHead != INVALID_ID && m_listTail != INVALID_ID, "Cannot remove from empty list, head:%d, tail:%d, TRACE_STACK:%s", m_listHead, m_listTail, TRACE_STACK());

    // 检查链表索引范围
    CHECK_EXPR_RE_VOID(m_listHead >= 0 && m_listHead < MAX_SIZE, "List head index out of range: %d, valid range [0, %d), TRACE_STACK:%s", m_listHead, MAX_SIZE, TRACE_STACK());
    CHECK_EXPR_RE_VOID(m_listTail >= 0 && m_listTail < MAX_SIZE, "List tail index out of range: %d, valid range [0, %d), TRACE_STACK:%s", m_listTail, MAX_SIZE, TRACE_STACK());

    if (pNode->m_listPrev != INVALID_ID)
    {
        CHECK_EXPR_RE_VOID(pNode->m_listPrev >= 0 && pNode->m_listPrev < MAX_SIZE, "Node prev index out of range: %d, valid range [0, %d), TRACE_STACK:%s", pNode->m_listPrev, MAX_SIZE, TRACE_STACK());
        auto pPrevNode = GetValidNode(pNode->m_listPrev);
        CHECK_EXPR_RE_VOID(pPrevNode != nullptr, "Previous node is null at index %d, TRACE_STACK:%s", pNode->m_listPrev, TRACE_STACK());
        CHECK_EXPR_RE_VOID(pPrevNode->m_listNext == pNode->m_self, "Previous node next mismatch: expected %zu, got %d, TRACE_STACK:%s", pNode->m_self, pPrevNode->m_listNext, TRACE_STACK());

        pPrevNode->m_listNext = pNode->m_listNext;
    }
    else
    {
        // 删除的是头节点
        CHECK_EXPR_RE_VOID(m_listHead == pNode->m_self, "Node is not head but has no prev, head:%d, node:%zu, TRACE_STACK:%s", m_listHead, pNode->m_self, TRACE_STACK());
        m_listHead = pNode->m_listNext;
    }

    if (pNode->m_listNext != INVALID_ID)
    {
        CHECK_EXPR_RE_VOID(pNode->m_listNext >= 0 && pNode->m_listNext < MAX_SIZE, "Node next index out of range: %d, valid range [0, %d), TRACE_STACK:%s", pNode->m_listNext, MAX_SIZE, TRACE_STACK());
        auto pNextNode = GetValidNode(pNode->m_listNext);
        CHECK_EXPR_RE_VOID(pNextNode != nullptr, "Next node is null at index %d, TRACE_STACK:%s", pNode->m_listNext, TRACE_STACK());
        CHECK_EXPR_RE_VOID(pNextNode->m_listPrev == pNode->m_self, "Next node prev mismatch: expected %zu, got %d, TRACE_STACK:%s", pNode->m_self, pNextNode->m_listPrev, TRACE_STACK());

        pNextNode->m_listPrev = pNode->m_listPrev;
    }
    else
    {
        // 删除的是尾节点
        CHECK_EXPR_RE_VOID(m_listTail == pNode->m_self, "Node is not tail but has no next, tail:%d, node:%zu, TRACE_STACK:%s", m_listTail, pNode->m_self, TRACE_STACK());
        m_listTail = pNode->m_listPrev;
    }

    pNode->m_listPrev = INVALID_ID;
    pNode->m_listNext = INVALID_ID;

    // 验证链表操作后的状态
    if (m_listHead == INVALID_ID)
    {
        CHECK_EXPR_RE_VOID(m_listTail == INVALID_ID, "List head is INVALID_ID but tail is not INVALID_ID after removal: %d, TRACE_STACK:%s", m_listTail, TRACE_STACK());
    }
    else
    {
        CHECK_EXPR_RE_VOID(m_listTail != INVALID_ID, "List head is not INVALID_ID but tail is INVALID_ID after removal, head:%d, TRACE_STACK:%s", m_listHead, TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_listHead >= 0 && m_listHead < MAX_SIZE, "List head index out of range after removal: %d, valid range [0, %d), TRACE_STACK:%s", m_listHead, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_listTail >= 0 && m_listTail < MAX_SIZE, "List tail index out of range after removal: %d, valid range [0, %d), TRACE_STACK:%s", m_listTail, MAX_SIZE, TRACE_STACK());
    }
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::MoveToListTail(Node* pNode)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "HashTable not initialized, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode != nullptr, "Node pointer is null, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_valid, "Node is invalid, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_self >= 0 && pNode->m_self < MAX_SIZE, "Node self index out of range: %zu, valid range [0, %d), TRACE_STACK:%s", pNode->m_self, MAX_SIZE, TRACE_STACK());

    // 检查链表不为空
    CHECK_EXPR_RE_VOID(m_listHead != INVALID_ID && m_listTail != INVALID_ID, "Cannot move node in empty list, head:%d, tail:%d, TRACE_STACK:%s", m_listHead, m_listTail, TRACE_STACK());

    // 检查链表索引范围
    CHECK_EXPR_RE_VOID(m_listHead >= 0 && m_listHead < MAX_SIZE, "List head index out of range: %d, valid range [0, %d), TRACE_STACK:%s", m_listHead, MAX_SIZE, TRACE_STACK());
    CHECK_EXPR_RE_VOID(m_listTail >= 0 && m_listTail < MAX_SIZE, "List tail index out of range: %d, valid range [0, %d), TRACE_STACK:%s", m_listTail, MAX_SIZE, TRACE_STACK());

    // 如果已经是尾节点，无需移动
    if (pNode->m_self == m_listTail)
    {
        // 验证尾节点的一致性
        CHECK_EXPR_RE_VOID(pNode->m_listNext == INVALID_ID, "Tail node next should be INVALID_ID but is %d, TRACE_STACK:%s", pNode->m_listNext, TRACE_STACK());
        return;
    }

    // 先从当前位置移除
    RemoveFromList(pNode);

    // 然后添加到尾部
    AddToListTail(pNode);
}

// ==================== 链表迭代器接口实现 ====================

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::list_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::list_begin()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, list_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return list_iterator(GetValidNode(m_listHead), this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::list_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::list_end()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, list_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return list_iterator(nullptr, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_list_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::list_begin() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_list_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return const_list_iterator(GetValidNode(m_listHead), this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_list_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::list_end() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_list_iterator(nullptr, this), "not init, TRACE_STACK:%s", TRACE_STACK());
    return const_list_iterator(nullptr, this);
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_list_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::list_cbegin() const
{
    return list_begin();
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
typename NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::const_list_iterator NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::list_cend() const
{
    return list_end();
}

// ==================== 链表完整性验证函数实现 ====================

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
bool NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::ValidateListIntegrity() const
{
    if (m_init != EN_NF_SHM_STL_INIT_OK)
    {
        return false;
    }

    // 检查空链表情况
    if (m_listHead == INVALID_ID && m_listTail == INVALID_ID)
    {
        return true; // 空链表是合法的
    }

    // 检查非空链表的基本一致性
    if (m_listHead == INVALID_ID || m_listTail == INVALID_ID)
    {
        return false; // 头尾指针不一致
    }

    if (m_listHead < 0 || m_listHead >= MAX_SIZE ||
        m_listTail < 0 || m_listTail >= MAX_SIZE)
    {
        return false; // 头尾指针超出范围
    }

    // 从头到尾遍历链表，检查一致性
    int currentIdx = m_listHead;
    int prevIdx = INVALID_ID;
    size_t forwardCount = 0;
    const size_t maxIterations = m_size + 1;

    while (currentIdx != INVALID_ID && forwardCount < maxIterations)
    {
        auto pNode = GetValidNode(currentIdx);
        if (!pNode)
        {
            return false; // 节点无效
        }

        if (pNode->m_listPrev != prevIdx)
        {
            return false; // 前驱指针不匹配
        }

        prevIdx = currentIdx;
        currentIdx = pNode->m_listNext;
        forwardCount++;
    }

    if (forwardCount >= maxIterations)
    {
        return false; // 可能存在循环
    }

    if (prevIdx != m_listTail)
    {
        return false; // 最后一个节点不是尾节点
    }

    // 从尾到头遍历链表，再次验证
    currentIdx = m_listTail;
    int nextIdx = INVALID_ID;
    size_t backwardCount = 0;

    while (currentIdx != INVALID_ID && backwardCount < maxIterations)
    {
        auto pNode = GetValidNode(currentIdx);
        if (!pNode)
        {
            return false; // 节点无效
        }

        if (pNode->m_listNext != nextIdx)
        {
            return false; // 后继指针不匹配
        }

        nextIdx = currentIdx;
        currentIdx = pNode->m_listPrev;
        backwardCount++;
    }

    if (backwardCount >= maxIterations)
    {
        return false; // 可能存在循环
    }

    if (nextIdx != m_listHead)
    {
        return false; // 最后一个节点不是头节点
    }

    if (forwardCount != backwardCount)
    {
        return false; // 前向和后向遍历的节点数不匹配
    }

    // 验证链表中的所有节点都是有效的，且在哈希表中存在
    currentIdx = m_listHead;
    size_t validNodeCount = 0;

    while (currentIdx != INVALID_ID)
    {
        auto pNode = GetValidNode(currentIdx);
        if (!pNode || !pNode->m_valid)
        {
            return false;
        }

        validNodeCount++;
        currentIdx = pNode->m_listNext;
    }

    // 检查所有有效节点是否都在链表中
    size_t hashTableValidNodes = 0;
    for (int i = 0; i < MAX_SIZE; i++)
    {
        auto pNode = GetValidNode(i);
        if (pNode && pNode->m_valid)
        {
            hashTableValidNodes++;
        }
    }

    if (validNodeCount != hashTableValidNodes)
    {
        return false; // 链表中的有效节点数与哈希表中的不匹配
    }

    return true;
}

template <class Val, class Key, int MAX_SIZE, class HashFcn, class ExtractKey, class EqualKey>
void NFShmHashTableWithList<Val, Key, MAX_SIZE, HashFcn, ExtractKey, EqualKey>::print_list() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== Insertion Order List Details ===\n");
    printf("LRU Mode: %s\n", m_enableLRU ? "Enabled" : "Disabled");
    printf("List Head: %d, List Tail: %d\n", m_listHead, m_listTail);

    // 验证链表完整性
    bool listIntegrityOK = ValidateListIntegrity();
    printf("List Integrity: %s\n", listIntegrityOK ? "OK" : "FAILED");

    if (m_listHead != INVALID_ID)
    {
        printf("\n--- Forward Traversal (Insertion Order) ---\n");
        auto pNode = GetValidNode(m_listHead);
        size_type listCount = 0;
        size_type maxListCount = m_size + 1;

        while (pNode && listCount < maxListCount)
        {
            printf("Node[%zu]: ", pNode->m_self);

            // 打印键值信息
            try
            {
                auto key = m_getKey(pNode->m_value);
                printf("Key=");
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
                    printf("(complex)");
                }
            }
            catch (...)
            {
                printf("Key=(error)");
            }

            // 显示链表指针信息
            printf(", Prev=%d, Next=%d", pNode->m_listPrev, pNode->m_listNext);

            // 计算在哪个桶中
            try
            {
                size_type bucket = BktNum(pNode->m_value);
                printf(", Bucket=%zu", bucket);
            }
            catch (...)
            {
                printf(", Bucket=(error)");
            }

            printf("\n");

            listCount++;

            if (pNode->m_listNext != INVALID_ID)
            {
                pNode = GetValidNode(pNode->m_listNext);
            }
            else
            {
                break;
            }
        }

        if (listCount >= maxListCount)
        {
            printf("... (Loop detected, stopped at %zu nodes)\n", listCount);
        }

        printf("\nForward traversal: %zu nodes\n", listCount);

        if (listCount != m_size)
        {
            printf("WARNING: List count %zu != hash table size %zu\n", listCount, m_size);
        }

        // 打印反向遍历验证
        printf("\n--- Backward Traversal (LRU Order) ---\n");
        auto pTailNode = GetValidNode(m_listTail);
        size_type reverseCount = 0;

        while (pTailNode && reverseCount < maxListCount)
        {
            printf("Node[%zu]: ", pTailNode->m_self);

            // 打印键值信息
            try
            {
                auto key = m_getKey(pTailNode->m_value);
                printf("Key=");
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
                    printf("(complex)");
                }
            }
            catch (...)
            {
                printf("Key=(error)");
            }

            printf(", Prev=%d, Next=%d\n", pTailNode->m_listPrev, pTailNode->m_listNext);

            reverseCount++;

            if (pTailNode->m_listPrev != INVALID_ID)
            {
                pTailNode = GetValidNode(pTailNode->m_listPrev);
            }
            else
            {
                break;
            }
        }

        if (reverseCount >= maxListCount)
        {
            printf("... (Reverse loop detected, stopped at %zu nodes)\n", reverseCount);
        }

        printf("\nBackward traversal: %zu nodes\n", reverseCount);

        if (reverseCount != listCount)
        {
            printf("WARNING: Forward count %zu != Backward count %zu\n", listCount, reverseCount);
        }

        // 打印简化的插入顺序
        printf("\n--- Insertion Order Summary ---\n");
        printf("First -> Last: ");
        pNode = GetValidNode(m_listHead);
        size_type summaryCount = 0;

        while (pNode && summaryCount < 10) // 只显示前10个
        {
            printf("%zu", pNode->m_self);
            summaryCount++;

            if (pNode->m_listNext != INVALID_ID && summaryCount < 10)
            {
                printf(" -> ");
                pNode = GetValidNode(pNode->m_listNext);
            }
            else
            {
                if (pNode->m_listNext != INVALID_ID)
                {
                    printf(" -> ... -> %d", m_listTail);
                }
                break;
            }
        }
        printf("\n");
    }
    else
    {
        printf("List is empty\n");

        if (m_listTail != INVALID_ID)
        {
            printf("WARNING: Head is INVALID_ID but tail is %d\n", m_listTail);
        }

        // 检查是否有孤立的链表节点
        printf("\nChecking for orphaned list nodes...\n");
        bool foundOrphans = false;
        for (int i = 0; i < MAX_SIZE; i++)
        {
            auto pNode = GetValidNode(i);
            if (pNode && (pNode->m_listPrev != INVALID_ID || pNode->m_listNext != INVALID_ID))
            {
                if (!foundOrphans)
                {
                    printf("WARNING: Found orphaned list nodes:\n");
                    foundOrphans = true;
                }
                printf("  Node[%d]: Prev=%d, Next=%d\n", i, pNode->m_listPrev, pNode->m_listNext);
            }
        }

        if (!foundOrphans)
        {
            printf("No orphaned list nodes found.\n");
        }
    }

    printf("=====================================\n");
}

