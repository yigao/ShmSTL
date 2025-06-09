# NFShmStl - C++热更新共享内存STL容器库

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-11%2F14%2F17-green.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-lightgrey.svg)]()

## 📖 项目简介

NFShmStl 是一个专为C++热更新（Hot Reload）场景设计的高性能STL容器库。该库通过共享内存技术，使程序在代码更新时能够保持运行状态，实现真正的"无缝"热更新。当程序需要更新代码时，旧进程的状态会保存在共享内存中，新进程启动后可以从共享内存恢复完整的程序状态，从而避免了传统热更新中状态丢失的问题。

### 🎯 核心特性

- **🔥 热更新支持**：专为C++热更新设计，程序更新时状态零丢失
- **🔄 状态保持**：CREATE/RESUME模式，新进程自动恢复更新前的完整状态  
- **🚀 高性能**：基于连续内存布局，访问效率接近原生STL容器
- **🔧 STL兼容**：提供与std::vector、std::map、std::set等高度兼容的API
- **💾 内存安全**：固定内存池设计，避免内存碎片和野指针问题
- **🛡️ 异常安全**：采用错误码机制，避免异常传播带来的问题
- **⚡ 零拷贝**：状态恢复无需数据拷贝，瞬间完成程序状态迁移



### 🏗️ 支持的容器

| 容器类型 | 共享内存版本 | 对应STL容器 | 主要特性 |
|---------|-------------|------------|---------|
| **顺序容器** | `NFShmVector` | `std::vector` | 动态数组，随机访问O(1) |
| | `NFShmList` | `std::list` | 双向链表，插入删除O(1) |
| | `NFShmString` | `std::string` | 字符串容器，优化的字符操作 |
| **关联容器** | `NFShmMap` | `std::map` | 有序键值对，基于红黑树 |
| | `NFShmMultiMap` | `std::multimap` | 允许重复键的有序映射 |
| | `NFShmSet` | `std::set` | 有序唯一元素集合 |
| | `NFShmMultiSet` | `std::multiset` | 允许重复元素的有序集合 |
| **哈希容器** | `NFShmHashMap` | `std::unordered_map` | 哈希键值对，O(1)查找 |
| | `NFShmHashSet` | `std::unordered_set` | 哈希唯一元素集合 |
| | `NFShmHashMultiMap` | `std::unordered_multimap` | 允许重复键的哈希映射 |
| | `NFShmHashMultiSet` | `std::unordered_multiset` | 允许重复元素的哈希集合 |
| **适配器容器** | `NFShmStack` | `std::stack` | 栈容器适配器 |
| | `NFShmQueue` | `std::queue` | 队列容器适配器 |
| | `NFShmPriorityQueue` | `std::priority_queue` | 优先队列适配器 |
| **工具类** | `NFShmPair` | `std::pair` | 键值对 |
| | `NFShmBitSet` | `std::bitset` | 位集合 |
| | `NFShmBitMap` | - | 位图工具类 |

## 🚀 快速开始

## 📋 容器详细对比

### 🔢 顺序容器

#### NFShmVector vs std::vector

**相同点**：
- 连续内存存储，支持随机访问O(1)
- 支持完整的迭代器（正向、反向、常量迭代器）
- 兼容的API：`push_back()`, `pop_back()`, `insert()`, `erase()`, `size()`, `empty()`等

**关键差异**：

| 特性 | std::vector | NFShmVector |
|------|-------------|-------------|
| **容量管理** | 动态扩容，2倍增长策略 | 固定容量，编译时确定 |
| **内存分配** | 堆内存，可能重新分配 | 共享内存，位置固定 |
| **容量满处理** | 自动扩容 | 返回错误，不自动扩容 |
| **热更新支持** | ❌ 状态丢失 | ✅ 状态保持 |
| **异常处理** | 抛出异常 | 错误码返回 |

**独有功能**：
```cpp
// 热更新专用
vec.CreateInit();  // 首次创建
vec.ResumeInit();  // 热更新恢复

// 扩展算法
vec.binary_search(value);     // 二分查找
vec.binary_insert(value);     // 有序插入  
vec.sort();                   // 就地排序
vec.unique();                 // 去重
vec.full();                   // 容量检查
```

#### NFShmList vs std::list

**相同点**：
- 双向链表结构，插入删除O(1)
- 支持双向迭代器
- 兼容的API：`push_front()`, `push_back()`, `insert()`, `splice()`等

**关键差异**：

| 特性 | std::list | NFShmList |
|------|-----------|-----------|
| **节点分配** | 动态分配，可能分散 | 预分配节点池，连续内存 |
| **容量限制** | 仅受内存限制 | 固定最大节点数 |
| **内存碎片** | 可能产生碎片 | 无碎片，固定池管理 |
| **热更新支持** | ❌ 状态丢失 | ✅ 状态保持 |
| **指针有效性** | 稳定 | 稳定（共享内存内） |

#### NFShmString vs std::string

**相同点**：
- 字符串操作API：`append()`, `substr()`, `find()`, `replace()`等
- 支持字符访问和迭代器
- 支持C风格字符串兼容

**关键差异**：

| 特性 | std::string | NFShmString |
|------|-------------|-------------|
| **容量管理** | 动态扩容，SSO优化 | 固定容量缓冲区 |
| **内存布局** | 可能堆分配 | 连续共享内存 |
| **编码支持** | 实现定义 | UTF-8友好 |
| **热更新支持** | ❌ 状态丢失 | ✅ 状态保持 |

### 🗺️ 关联容器

#### NFShmMap vs std::map

**相同点**：
- 基于红黑树实现，自动排序
- 查找、插入、删除O(log n)
- 兼容的API：`insert()`, `find()`, `lower_bound()`, `upper_bound()`等

**关键差异**：

| 特性 | std::map | NFShmMap |
|------|----------|----------|
| **节点分配** | 动态分配 | 预分配节点池 |
| **容量限制** | 仅受内存限制 | 固定最大节点数 |
| **比较器** | 可自定义 | 可自定义，需编译时确定 |
| **热更新支持** | ❌ 状态丢失 | ✅ 状态保持 |
| **内存访问** | 可能缓存不友好 | 连续内存，缓存友好 |

```cpp
// 使用示例
NFShmMap<string, PlayerData, 10000> players;
players.CreateInit();
players["player1"] = {100, 200};  // 等价于std::map操作
```

#### NFShmSet vs std::set

**相同点**：
- 基于红黑树，元素唯一且有序
- 查找、插入、删除O(log n)
- 支持范围查询：`lower_bound()`, `upper_bound()`

**关键差异**：与NFShmMap类似，主要在内存管理和热更新支持方面

### 🏃 哈希容器

#### NFShmHashMap vs std::unordered_map

**相同点**：
- 哈希表实现，平均O(1)查找
- 支持自定义哈希函数和相等比较
- 兼容的API：`insert()`, `find()`, `bucket_count()`等

**关键差异**：

| 特性 | std::unordered_map | NFShmHashMap |
|------|-------------------|--------------|
| **哈希表大小** | 动态调整 | 固定大小，编译时确定 |
| **冲突处理** | 通常链式哈希 | 可配置链式或开放寻址 |
| **load_factor** | 自动rehash | 固定，需手动规划 |
| **热更新支持** | ❌ 状态丢失 | ✅ 状态保持 |

```cpp
// 哈希容器优势
NFShmHashMap<int, string, 100000> userNames;
userNames.CreateInit();
userNames[12345] = "张三";  // O(1)平均时间
```

#### NFShmHashSet vs std::unordered_set

**相同点**：
- 哈希集合，元素唯一
- 平均O(1)查找和插入
- 支持自定义哈希函数

**关键差异**：与NFShmHashMap类似

### 📦 适配器容器

#### NFShmStack vs std::stack

**相同点**：
- LIFO（后进先出）语义
- 兼容的API：`push()`, `pop()`, `top()`, `empty()`

**关键差异**：

| 特性 | std::stack | NFShmStack |
|------|------------|------------|
| **底层容器** | 可选（deque/vector/list） | 基于NFShmVector |
| **容量限制** | 依赖底层容器 | 固定最大容量 |
| **热更新支持** | ❌ 状态丢失 | ✅ 状态保持 |

#### NFShmQueue vs std::queue

**相同点**：
- FIFO（先进先出）语义
- 兼容的API：`push()`, `pop()`, `front()`, `back()`

**关键差异**：与NFShmStack类似

#### NFShmPriorityQueue vs std::priority_queue

**相同点**：
- 堆结构，自动维护优先级
- 兼容的API：`push()`, `pop()`, `top()`

**关键差异**：

| 特性 | std::priority_queue | NFShmPriorityQueue |
|------|--------------------|--------------------|
| **底层容器** | 通常vector | 基于NFShmVector |
| **比较器** | 可自定义 | 可自定义，编译时确定 |
| **热更新支持** | ❌ 状态丢失 | ✅ 状态保持 |

### 🔧 工具类

#### NFShmPair vs std::pair

**相同点**：
- 两个值的组合：`first`, `second`
- 支持比较操作和`make_pair()`

**关键差异**：

| 特性 | std::pair | NFShmPair |
|------|-----------|-----------|
| **内存布局** | 编译器优化 | 保证共享内存兼容 |
| **构造方式** | 多种构造函数 | 简化构造，适合共享内存 |

#### NFShmBitSet vs std::bitset

**相同点**：
- 位操作：`set()`, `reset()`, `flip()`, `test()`
- 位运算支持：`&`, `|`, `^`, `~`

**关键差异**：

| 特性 | std::bitset | NFShmBitSet |
|------|-------------|-------------|
| **大小确定** | 编译时模板参数 | 编译时模板参数 |
| **内存布局** | 实现定义 | 保证共享内存兼容 |
| **热更新支持** | ❌ 状态丢失 | ✅ 状态保持 |

#### NFShmBitMap（独有）

NFShmStl特有的位图工具类，提供高效的位操作：

```cpp
NFShmBitMap<1000000> largeBitmap;  // 100万位的位图
largeBitmap.CreateInit();
largeBitmap.SetBit(12345, true);   // 设置位
bool isSet = largeBitmap.TestBit(12345);  // 测试位
```

### 📊 选择指南

| 需求场景 | 推荐容器 | 原因 |
|---------|----------|------|
| **频繁随机访问** | NFShmVector | O(1)访问，缓存友好 |
| **频繁插入删除** | NFShmList | O(1)插入删除 |
| **有序查找** | NFShmMap/Set | O(log n)，自动排序 |
| **快速查找** | NFShmHashMap/Set | O(1)平均，哈希优化 |
| **字符串处理** | NFShmString | 专门优化的字符串操作 |
| **栈/队列语义** | NFShmStack/Queue | 标准LIFO/FIFO操作 |
| **优先级处理** | NFShmPriorityQueue | 自动优先级维护 |
| **位操作** | NFShmBitSet/BitMap | 高效位级操作 |

## 🤝 贡献指南

我们欢迎社区贡献！请遵循以下步骤：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 开发规范

- 遵循现有的代码风格
- 添加适当的注释和文档
- 确保所有测试通过
- 新功能需要添加对应的测试用例

## 📄 许可证

本项目采用 Apache License 2.0 许可证 - 详情请参阅 [LICENSE](LICENSE) 文件。

## 👥 作者信息

- **主要作者**: gaoyi
- **邮箱**: 445267987@qq.com
- **创建日期**: 2023年

## 🔗 相关链接

- [问题反馈](../../issues)
- [功能请求](../../issues/new)
- [更新日志](CHANGELOG.md)

## 📈 更新历史

- **v1.0.0** (2023-04-20): 初始发布版本
  - 支持基础容器：Vector, List, Map, Set
  - 完整的STL API兼容性
  - 双模式初始化支持

- **v1.1.0** (2023-08-19): 哈希容器支持
  - 新增哈希表系列容器
  - 性能优化和bug修复
  - 完善的测试覆盖

---

⭐ 如果这个项目对您有帮助，请给我们一个星标！ 