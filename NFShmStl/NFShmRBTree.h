// -------------------------------------------------------------------------
//    @FileName         :    NFShmRBTree.h
//    @Author           :    gaoyi
//    @Date             :    2025/5/19
//    @Email            :    445267987@qq.com
//    @Module           :    NFShmRBTree
// 基础实现有AI搞定，后续不断改进而来
// -------------------------------------------------------------------------

/**
 * @file NFShmRBTree.h
 * @brief 基于共享内存的红黑树实现，提供有序关联容器功能
 * 
 * @section overview 概述
 * 
 * NFShmRBTree 是一个专为共享内存环境设计的红黑树实现，为std::map和std::set
 * 等有序关联容器提供底层支持。在API设计上尽量保持与STL兼容，但在内存管理、
 * 容量限制等方面针对共享内存环境进行了优化。
 * 
 * @section features 核心特性
 * 
 * 1. **红黑树数据结构**：
 *    - 自平衡二叉搜索树，保证O(log n)的查找、插入、删除性能
 *    - 严格维护红黑树的五个基本性质
 *    - 支持双向迭代器，提供有序遍历
 * 
 * 2. **共享内存优化**：
 *    - 固定大小内存布局，适合进程间共享
 *    - 使用索引而非指针，避免地址空间问题
 *    - 支持CREATE/RESUME模式初始化
 *    - 内存池管理，避免动态分配
 * 
 * 3. **STL兼容接口**：
 *    - 完整的迭代器支持（正向、反向、常量迭代器）
 *    - 标准的容器操作（insert、erase、find、count等）
 *    - 支持唯一键和重复键两种模式
 *    - C++11风格的emplace操作
 * 
 * 4. **调试和验证**：
 *    - 内置红黑树性质验证
 *    - 丰富的调试打印功能
 *    - 图形化树结构显示
 * 
 * @section stl_comparison STL容器对比
 * 
 * | 特性 | STL map/set | NFShmRBTree |
 * |------|-------------|-------------|
 * | **数据结构** | 红黑树（通常） | 红黑树 |
 * | **容量管理** | 动态扩容，无限制 | 固定容量MAX_SIZE，编译时确定 |
 * | **内存管理** | 堆内存，自动分配/释放 | 共享内存，手动管理 |
 * | **节点引用** | 指针 | 索引（适合共享内存） |
 * | **时间复杂度** | O(log n) 查找/插入/删除 | O(log n) 查找/插入/删除 |
 * | **迭代器类型** | 双向迭代器 | 双向迭代器 |
 * | **元素顺序** | 严格有序（比较器定义） | 严格有序（比较器定义） |
 * | **重复键支持** | map(No)/multimap(Yes) | 可配置（unique/equal模式） |
 * | **进程共享** | 不支持 | **原生支持** |
 * | **异常安全** | 强异常安全保证 | 无异常，错误码返回 |
 * | **内存碎片** | 可能产生 | **无碎片**（固定内存池） |
 * 
 * @section api_compatibility API兼容性
 * 
 * **完全兼容的接口**：
 * - size(), empty(), max_size()
 * - begin(), end(), rbegin(), rend()
 * - find(), count(), lower_bound(), upper_bound(), equal_range()
 * - insert(), erase(), clear()
 * - key_comp()
 * - swap()
 * 
 * **扩展的接口（新增）**：
 * - full() - 检查容器是否已满
 * - CreateInit(), ResumeInit() - 共享内存初始化
 * - RbVerify(), __rb_verify() - 红黑树完整性验证
 * - print_structure(), print_detailed(), print_simple() - 调试打印
 * 
 * **行为差异的接口**：
 * - max_size()：返回MAX_SIZE而非理论最大值
 * - insert()：容量满时返回失败而非自动扩容
 * - 构造函数：需要显式调用CreateInit()或ResumeInit()
 * 
 * @section usage_examples 使用示例
 * 
 * @subsection basic_usage 基础用法（类似std::map）
 * 
 * ```cpp
 * // 定义键值对类型
 * struct KeyValue {
 *     int key;
 *     std::string value;
 *     KeyValue(int k, const std::string& v) : key(k), value(v) {}
 * };
 * 
 * // 键提取器
 * struct ExtractKey {
 *     int operator()(const KeyValue& kv) const { return kv.key; }
 * };
 * 
 * // 定义红黑树类型（最大1000个元素）
 * using MyRBTree = NFShmRBTree<
 *     int,                    // 键类型
 *     KeyValue,               // 值类型
 *     ExtractKey,             // 键提取器
 *     1000,                   // 最大容量
 *     std::less<int>          // 比较器
 * >;
 * 
 * MyRBTree tree;
 * tree.CreateInit();
 * 
 * // STL兼容的基础操作
 * tree.insert_unique({1, "one"});
 * tree.insert_unique({3, "three"});
 * tree.insert_unique({2, "two"});
 * 
 * auto it = tree.find(2);
 * if (it != tree.end()) {
 *     std::cout << "Found: " << it->value << std::endl;
 * }
 * ```
 * 
 * @subsection ordered_iteration 有序遍历
 * 
 * ```cpp
 * // 插入一些元素
 * tree.insert_unique({50, "fifty"});
 * tree.insert_unique({10, "ten"});
 * tree.insert_unique({30, "thirty"});
 * tree.insert_unique({20, "twenty"});
 * 
 * // 正向遍历（升序）
 * std::cout << "Ascending order: ";
 * for (auto it = tree.begin(); it != tree.end(); ++it) {
 *     std::cout << it->key << " ";  // 输出: 10 20 30 50
 * }
 * 
 * // 反向遍历（降序）
 * std::cout << "\\nDescending order: ";
 * for (auto it = tree.rbegin(); it != tree.rend(); ++it) {
 *     std::cout << it->key << " ";  // 输出: 50 30 20 10
 * }
 * ```
 * 
 * @subsection range_operations 范围操作
 * 
 * ```cpp
 * // 查找范围
 * auto lower = tree.lower_bound(15);  // >= 15的第一个元素
 * auto upper = tree.upper_bound(35);  // > 35的第一个元素
 * 
 * std::cout << "Range [15, 35]: ";
 * for (auto it = lower; it != upper; ++it) {
 *     std::cout << it->key << " ";  // 输出: 20 30
 * }
 * 
 * // equal_range：获取等于某个键的范围
 * auto range = tree.equal_range(20);
 * size_t count = std::distance(range.first, range.second);
 * std::cout << "Count of 20: " << count << std::endl;
 * ```
 * 
 * @subsection multikey_usage 多重键模式（类似std::multimap）
 * 
 * ```cpp
 * // 允许重复键的插入
 * tree.insert_equal({10, "ten-1"});
 * tree.insert_equal({10, "ten-2"});
 * tree.insert_equal({10, "ten-3"});
 * 
 * // 查找所有匹配的元素
 * auto range = tree.equal_range(10);
 * std::cout << "All values for key 10: ";
 * for (auto it = range.first; it != range.second; ++it) {
 *     std::cout << it->value << " ";  // 输出: ten-1 ten-2 ten-3
 * }
 * 
 * // 删除特定键的所有元素
 * size_t erased = tree.erase(10);  // 返回删除的元素数量
 * std::cout << "Erased " << erased << " elements" << std::endl;
 * ```
 * 
 * @section performance_characteristics 性能特征
 * 
 * | 操作 | STL map/set | NFShmRBTree |
 * |------|-------------|-------------|
 * | **查找 find()** | O(log n) | O(log n) |
 * | **插入 insert()** | O(log n) | O(log n) |
 * | **删除 erase()** | O(log n) | O(log n) |
 * | **范围查找** | O(log n + k) | O(log n + k) |
 * | **有序遍历** | O(n) | O(n) |
 * | **内存分配** | 动态分配节点 | 预分配内存池 |
 * | **缓存友好性** | 较差（指针跳跃） | **较好**（索引访问） |
 * | **内存开销** | 每节点约32字节 | 每节点约40字节（含索引） |
 * 
 * @section red_black_properties 红黑树性质
 * 
 * 本实现严格维护红黑树的五个基本性质：
 * 
 * 1. **节点着色**：每个节点要么是红色，要么是黑色
 * 2. **根节点**：根节点必须是黑色
 * 3. **叶子节点**：所有叶子节点（NIL节点）都是黑色
 * 4. **红色约束**：红色节点的子节点必须是黑色（不能有连续的红色节点）
 * 5. **黑高一致**：从任一节点到其每个叶子的所有路径都包含相同数量的黑色节点
 * 
 * 这些性质保证了树的平衡性，确保最坏情况下的查找时间复杂度为O(log n)。
 * 
 * @section memory_layout 内存布局
 * 
 * ```
 * NFShmRBTree 内存结构：
 * ┌─────────────────┐
 * │   管理数据       │ <- m_size, m_freeStart, m_init等
 * ├─────────────────┤
 * │   节点存储区     │ <- m_mem[MAX_SIZE + 1] (AlignedStorage)
 * │   [0]           │    ├─ 数据节点 [0..MAX_SIZE-1]
 * │   [1]           │    └─ 头节点 [MAX_SIZE]
 * │   ...           │
 * │   [MAX_SIZE-1]  │
 * │   [MAX_SIZE]    │ <- 头节点（哨兵）
 * └─────────────────┘
 * 
 * 每个节点包含：
 * - m_data: 用户数据（仅数据节点）
 * - m_parent: 父节点索引
 * - m_left: 左子节点索引
 * - m_right: 右子节点索引
 * - m_color: 节点颜色（红/黑）
 * - m_valid: 节点有效标志
 * - m_self: 自身索引
 * ```
 * 
 * @section thread_safety 线程安全
 * 
 * - **非线程安全**：需要外部同步机制
 * - **共享内存兼容**：多进程可安全访问（需进程间锁）
 * - **无内部锁**：避免性能开销，由用户控制并发
 * 
 * @section migration_guide 从STL迁移指南
 * 
 * 1. **包含头文件**：
 *    ```cpp
 *    // 替换
 *    #include <map>
 *    #include <set>
 *    // 为
 *    #include "NFComm/NFShmStl/NFShmRBTree.h"
 *    ```
 * 
 * 2. **类型定义**：
 *    ```cpp
 *    // STL
 *    std::map<int, std::string> map;
 *    std::set<int> set;
 *    
 *    // NFShmRBTree（需要更多模板参数）
 *    NFShmRBTree<int, std::pair<int,std::string>, ExtractFirst, 1000> map;
 *    NFShmRBTree<int, int, Identity, 1000> set;
 *    ```
 * 
 * 3. **初始化**：
 *    ```cpp
 *    // 添加初始化调用
 *    tree.CreateInit();  // 或 ResumeInit() 用于共享内存恢复
 *    ```
 * 
 * 4. **容量管理**：
 *    ```cpp
 *    // 添加容量检查
 *    if (tree.full()) {
 *        // 处理容量已满的情况
 *    }
 *    
 *    // 移除无限容量假设
 *    // while (condition) tree.insert(...);  // 可能失败
 *    ```
 * 
 * 5. **错误处理**：
 *    ```cpp
 *    // STL异常处理
 *    try {
 *        tree.insert(value);
 *    } catch (const std::bad_alloc&) {
 *        // 处理内存不足
 *    }
 *    
 *    // NFShmRBTree错误检查
 *    auto result = tree.insert_unique(value);
 *    if (!result.second) {
 *        // 处理插入失败（容量满或键已存在）
 *    }
 *    ```
 * 
 * @section best_practices 最佳实践
 * 
 * 1. **容量规划**：根据预期数据量选择合适的MAX_SIZE，预留20-30%余量
 * 2. **键设计**：选择高效的比较器，避免昂贵的比较操作
 * 3. **内存对齐**：确保值类型满足对齐要求，提高访问效率
 * 4. **调试验证**：在开发阶段使用RbVerify()验证树的完整性
 * 5. **错误检查**：始终检查insert操作的返回值
 * 
 * @author gaoyi
 * @date 2025-05-19
 */

#pragma once

#include "NFComm/NFShmStl/NFShmStl.h"
#include <iterator>
#include <string>

enum NFRBTreeColor
{
    RB_RED = 0, // 红色节点
    RB_BLACK = 1 // 黑色节点
};

struct NFShmRBTreeNodeBase
{
    NFShmRBTreeNodeBase();
    ~NFShmRBTreeNodeBase();
    int CreateInit(); // 创建模式初始化
    int ResumeInit(); // 恢复模式初始化

    ptrdiff_t m_parent; // 父节点索引
    ptrdiff_t m_left; // 左子节点索引
    ptrdiff_t m_right; // 右子节点索引
    NFRBTreeColor m_color; // 节点颜色
    ptrdiff_t m_self; // 自身索引，便于定位
};

inline NFShmRBTreeNodeBase::NFShmRBTreeNodeBase()
{
    if (SHM_CREATE_MODE) // 根据模式选择初始化方法
    {
        CreateInit();
    }
    else
    {
        ResumeInit();
    }
}

inline NFShmRBTreeNodeBase::~NFShmRBTreeNodeBase()
{
    m_parent = INVALID_ID; // 初始化为无效索引
    m_left = INVALID_ID;
    m_right = INVALID_ID;
    m_color = RB_RED; // 默认新节点为红色
    m_self = INVALID_ID;
}

inline int NFShmRBTreeNodeBase::CreateInit()
{
    m_parent = INVALID_ID; // 初始化为无效索引
    m_left = INVALID_ID;
    m_right = INVALID_ID;
    m_color = RB_RED; // 默认新节点为红色
    m_self = INVALID_ID;
    return 0;
}

inline int NFShmRBTreeNodeBase::ResumeInit()
{
    return 0; // 恢复模式不需要特别处理
}

template <class KeyValue>
struct NFShmRBTreeNode : public NFShmRBTreeNodeBase
{
    NFShmRBTreeNode();
    ~NFShmRBTreeNode();
    int CreateInit();
    int ResumeInit();

    KeyValue m_data;
    bool m_valid;
};

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
class NFShmRBTree;

template <class Container>
struct NFShmRBTreeIteratorBase
{
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::bidirectional_iterator_tag iterator_category;

    Container* m_pContainer;
    NFShmRBTreeNodeBase* m_node;

    explicit NFShmRBTreeIteratorBase(const Container* pContainer, size_t iPos);
    explicit NFShmRBTreeIteratorBase(const Container* pContainer, const NFShmRBTreeNodeBase* pNode);
    NFShmRBTreeIteratorBase();

    void Increment();
    void Decrement();

    bool operator==(const NFShmRBTreeIteratorBase& x) const
    {
        return m_pContainer == x.m_pContainer && m_node == x.m_node;
    }

    bool operator!=(const NFShmRBTreeIteratorBase& x) const
    {
        return m_pContainer != x.m_pContainer || m_node != x.m_node;
    }
};

template <class KeyValue>
NFShmRBTreeNode<KeyValue>::NFShmRBTreeNode()
{
    // 根据模式选择不同的初始化方法
    if (SHM_CREATE_MODE)
    {
        CreateInit();
    }
    else
    {
        ResumeInit();
    }
}

template <class KeyValue>
NFShmRBTreeNode<KeyValue>::~NFShmRBTreeNode()
{
    m_valid = false;
}

template <class KeyValue>
int NFShmRBTreeNode<KeyValue>::CreateInit()
{
    m_valid = false; // 创建时默认无效
    return 0;
}

template <class KeyValue>
int NFShmRBTreeNode<KeyValue>::ResumeInit()
{
    return 0; // 恢复时不需要特殊处理
}

template <class Container>
NFShmRBTreeIteratorBase<Container>::NFShmRBTreeIteratorBase(const Container* pContainer, size_t iPos): m_pContainer(const_cast<Container*>(pContainer))
{
    m_node = m_pContainer->GetNode(iPos); // 根据索引获取节点
}

template <class Container>
NFShmRBTreeIteratorBase<Container>::NFShmRBTreeIteratorBase(const Container* pContainer, const NFShmRBTreeNodeBase* pNode): m_pContainer(const_cast<Container*>(pContainer)), m_node(const_cast<NFShmRBTreeNodeBase*>(pNode))
{
}

template <class Container>
NFShmRBTreeIteratorBase<Container>::NFShmRBTreeIteratorBase(): m_pContainer(nullptr), m_node(nullptr)
{
}

template <class Container>
void NFShmRBTreeIteratorBase<Container>::Increment()
{
    CHECK_EXPR_RE_VOID(m_node, "m_node == nullptr, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(m_pContainer, "m_pContainer == nullptr, TRACE_STACK:%s", TRACE_STACK());

    // 如果有右子树，找右子树中最左的节点
    if (m_pContainer->GetNode(m_node->m_right) != nullptr)
    {
        m_node = m_pContainer->GetNode(m_node->m_right);
        while (m_pContainer->GetNode(m_node->m_left) != nullptr)
            m_node = m_pContainer->GetNode(m_node->m_left);
    }
    else
    {
        // 如果没有右子树，向上找到第一个作为左子节点的祖先
        NFShmRBTreeNodeBase* y = m_pContainer->GetNode(m_node->m_parent);
        CHECK_EXPR_RE_VOID(y, "y == nullptr, TRACE_STACK:%s", TRACE_STACK());
        while (m_node == m_pContainer->GetNode(y->m_right))
        {
            m_node = y;
            y = m_pContainer->GetNode(y->m_parent);
            CHECK_EXPR_RE_VOID(y, "y == nullptr, TRACE_STACK:%s", TRACE_STACK());
        }
        if (m_pContainer->GetNode(m_node->m_right) != y)
            m_node = y;
    }
}

template <class Container>
void NFShmRBTreeIteratorBase<Container>::Decrement()
{
    CHECK_EXPR_RE_VOID(m_node, "m_node == nullptr, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(m_pContainer, "m_pContainer == nullptr, TRACE_STACK:%s", TRACE_STACK());

    // 处理当前节点是header的特殊情况（此时需要找最大节点）
    if (m_node->m_color == RB_RED &&
        m_pContainer->GetNode(m_node->m_parent) && m_pContainer->GetNode(m_pContainer->GetNode(m_node->m_parent)->m_parent) == m_node)
    {
        m_node = m_pContainer->GetNode(m_node->m_right);
    }
    // 如果有左子树，找左子树中最右的节点
    else if (m_pContainer->GetNode(m_node->m_left) != nullptr)
    {
        NFShmRBTreeNodeBase* y = m_pContainer->GetNode(m_node->m_left);
        while (m_pContainer->GetNode(y->m_right) != nullptr)
            y = m_pContainer->GetNode(y->m_right);
        m_node = y;
    }
    else
    {
        // 如果没有左子树，向上找到第一个作为右子节点的祖先
        NFShmRBTreeNodeBase* y = m_pContainer->GetNode(m_node->m_parent);
        CHECK_EXPR_RE_VOID(y, "y == nullptr, TRACE_STACK:%s", TRACE_STACK());
        while (m_node == m_pContainer->GetNode(y->m_left))
        {
            m_node = y;
            y = m_pContainer->GetNode(y->m_parent);
            CHECK_EXPR_RE_VOID(y, "y == nullptr, TRACE_STACK:%s", TRACE_STACK());
        }
        m_node = y;
    }
}

template <class KeyValue, class Ref, class Ptr, class Container>
struct NFShmRBTreeIterator : public NFShmRBTreeIteratorBase<Container>
{
    typedef KeyValue value_type;
    typedef Ref reference;
    typedef Ptr pointer;
    typedef NFShmRBTreeIterator<KeyValue, KeyValue&, KeyValue*, Container> iterator;
    typedef NFShmRBTreeIterator<KeyValue, const KeyValue&, const KeyValue*, Container> const_iterator;
    typedef NFShmRBTreeIterator Self;

    typedef NFShmRBTreeNode<KeyValue> Node;

    using NFShmRBTreeIteratorBase<Container>::m_node;
    using NFShmRBTreeIteratorBase<Container>::m_pContainer;

    static value_type m_staticError;

    explicit NFShmRBTreeIterator(const Container* pContainer, size_t iPos);
    explicit NFShmRBTreeIterator(const Container* pContainer, const NFShmRBTreeNodeBase* pNode);
    NFShmRBTreeIterator();
    NFShmRBTreeIterator(const iterator& x);

    reference operator*() const; // 解引用操作
    pointer operator->() const; // 成员访问操作
    Self& operator++(); // 前缀递增
    Self operator++(int); // 后缀递增
    Self& operator--(); // 前缀递减
    Self operator--(int); // 后缀递减
};

template <class KeyValue, class Ref, class Ptr, class Container>
KeyValue NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::m_staticError = KeyValue();

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare = std::less<Key>>
class NFShmRBTree
{
    friend struct NFShmRBTreeIteratorBase<NFShmRBTree>;

protected:
    typedef NFShmRBTreeNode<KeyValue> Node;
    typedef NFShmRBTreeNodeBase NodeBase;

public:
    typedef Key key_type; // 键类型
    typedef KeyValue value_type; // 值类型
    typedef Compare key_compare; // 键比较器类型
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    // 迭代器类型定义
    typedef NFShmRBTreeIterator<value_type, reference, pointer, NFShmRBTree> iterator;
    typedef NFShmRBTreeIterator<value_type, const_reference, const_pointer, NFShmRBTree> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

protected:
    // 使用定长内存池存储节点，避免动态内存分配
    typedef typename std::aligned_storage<sizeof(Node), alignof(Node)>::type AlignedStorage;
    AlignedStorage m_mem[MAX_SIZE + 1]; // +1 for header node，额外一个用于头节点
    ptrdiff_t m_freeStart; // 空闲节点链表的起始位置
    size_t m_size; // 树中节点数量
    int m_init; // 初始化标志
#ifdef NF_DEBUG_MODE
    Node* m_ptr; // 调试模式下的指针
#endif
    Compare m_keyCompare;

private:
    static Key m_staticDefaultKey;

public:
    // ==================== STL兼容构造和析构接口 ====================

    /**
     * @brief 默认构造函数
     * @note 与STL对比：
     *       - STL: std::map<K,V> map; 可直接使用
     *       - 本实现: 构造后需调用CreateInit()或ResumeInit()
     */
    NFShmRBTree(); // 默认构造函数

    /**
     * @brief 拷贝构造函数
     * @param tree 源红黑树
     * @note 与STL对比：
     *       - STL: 深拷贝，动态分配内存
     *       - 本实现: 固定内存池拷贝，需要容量足够
     */
    NFShmRBTree(const NFShmRBTree& tree); // 拷贝构造函数

    /**
     * @brief 析构函数
     * @note 与STL对比：
     *       - STL: 自动释放所有动态分配的内存
     *       - 本实现: 仅清理对象状态，内存池保持不变
     */
    virtual ~NFShmRBTree();

    // ==================== 共享内存专用初始化接口 ====================

    /**
     * @brief 创建模式初始化
     * @return 0表示成功
     * @note STL没有对应接口，共享内存特有功能
     *       - 用于首次创建时初始化内存池和管理结构
     *       - 必须在首次使用前调用
     */
    int CreateInit(); // 创建模式初始化

    /**
     * @brief 恢复模式初始化
     * @return 0表示成功
     * @note STL没有对应接口，共享内存特有功能
     *       - 用于从已有共享内存恢复红黑树状态
     *       - 进程重启后必须调用此函数
     */
    int ResumeInit(); // 恢复模式初始化

    // ==================== STL兼容迭代器接口 ====================

    /**
     * @brief 返回指向第一个元素的迭代器
     * @return 双向迭代器，指向最小键值元素
     * @note 与STL对比：
     *       - STL: std::map::begin()
     *       - 行为完全一致，但迭代器实现基于索引而非指针
     *       - 复杂度：O(log n) - 需要找到最左节点
     */
    iterator begin(); // 返回指向第一个元素的迭代器
    const_iterator begin() const;

    /**
     * @brief 返回指向最后一个元素之后的迭代器
     * @return 双向迭代器，作为结束标记
     * @note 与STL对比：
     *       - STL: std::map::end()
     *       - 行为完全一致，指向头节点（哨兵）
     *       - 复杂度：O(1)
     */
    iterator end(); // 返回指向最后一个元素之后的迭代器
    const_iterator end() const;

    /**
     * @brief 返回反向迭代器（从最大元素开始）
     * @return 反向迭代器
     * @note 与STL对比：
     *       - STL: std::map::rbegin()
     *       - 行为完全一致，支持反向遍历
     *       - 复杂度：O(log n) - 需要找到最右节点
     */
    reverse_iterator rbegin(); // 返回反向迭代器
    const_reverse_iterator rbegin() const;

    /**
     * @brief 返回反向结束迭代器
     * @return 反向迭代器结束标记
     * @note 与STL对比：
     *       - STL: std::map::rend()
     *       - 行为完全一致，复杂度：O(1)
     */
    reverse_iterator rend();
    const_reverse_iterator rend() const;

    // ==================== STL兼容容量查询接口 ====================

    /**
     * @brief 判断红黑树是否为空
     * @return true表示空，false表示非空
     * @note 与STL对比：
     *       - STL: std::map::empty()
     *       - 行为完全一致，复杂度：O(1)
     */
    bool empty() const; // 判断树是否为空

    /**
     * @brief 检查红黑树是否已满
     * @return true表示已满，false表示未满
     * @note 与STL对比：
     *       - STL: 无对应接口（动态容量）
     *       - 固定容量容器特有，复杂度：O(1)
     */
    bool full() const; // 判断树是否已满

    /**
     * @brief 返回当前元素数量
     * @return 当前存储的元素数量
     * @note 与STL对比：
     *       - STL: std::map::size()
     *       - 行为完全一致，复杂度：O(1)
     */
    size_type size() const; // 返回树中元素数量

    /**
     * @brief 返回最大容量
     * @return 最大可存储的元素数量
     * @note 与STL对比：
     *       - STL: std::map::max_size() 返回理论最大值
     *       - 本实现: 返回编译时确定的MAX_SIZE
     *       - 复杂度：O(1)
     */
    size_type max_size() const; // 返回树可容纳的最大元素数量

    // ==================== STL兼容插入接口（唯一键模式） ====================

    /**
     * @brief 插入元素（唯一键模式）
     * @param v 要插入的值
     * @return pair<iterator, bool> 迭代器指向插入位置，bool表示是否插入成功
     * @note 与STL对比：
     *       - STL: std::map::insert()
     *       - 行为相似，但容量满时返回失败而非抛异常
     *       - 复杂度：O(log n)
     */
    std::pair<iterator, bool> insert_unique(const value_type& v);

    /**
     * @brief 批量插入（指针范围，唯一键模式）
     * @param first 起始指针
     * @param last 结束指针
     * @note 与STL对比：
     *       - STL: std::map::insert(first, last)
     *       - 容量不足时部分插入，不抛异常
     */
    void insert_unique(const value_type* first, const value_type* last);

    /**
     * @brief 位置提示插入（唯一键模式）
     * @param position 位置提示迭代器
     * @param v 要插入的值
     * @return 指向插入位置的迭代器
     * @note 与STL对比：
     *       - STL: std::map::insert(hint, value)
     *       - 提示正确时可优化为O(1)，否则退化为O(log n)
     */
    iterator insert_unique(const_iterator position, const value_type& v);

    /**
     * @brief 批量插入（迭代器范围，唯一键模式）
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @note 模板参数自动推导迭代器类型
     */
    void insert_unique(const_iterator first, const_iterator last);
    template <class InputIterator>
    void insert_unique(InputIterator first, InputIterator last);

    /**
     * @brief 就地构造插入（唯一键模式）
     * @param args 构造参数
     * @return pair<iterator, bool> 插入结果
     * @note 与STL对比：
     *       - STL: std::map::emplace()
     *       - C++11特性，减少拷贝构造开销
     */
    template <class... Args>
    std::pair<iterator, bool> emplace_unique(const Args&... args);

    /**
     * @brief 位置提示就地构造（唯一键模式）
     * @param position 位置提示
     * @param args 构造参数
     * @return 指向插入位置的迭代器
     * @note 与STL对比：
     *       - STL: std::map::emplace_hint()
     *       - C++11特性，结合位置提示和就地构造
     */
    template <class... Args>
    iterator emplace_hint_unique(const_iterator position, const Args&... args);

    // ==================== STL兼容插入接口（允许重复键模式） ====================

    /**
     * @brief 插入元素（允许重复键）
     * @param v 要插入的值
     * @return 指向插入位置的迭代器
     * @note 与STL对比：
     *       - STL: std::multimap::insert()
     *       - 总是成功插入（除非容量满）
     *       - 复杂度：O(log n)
     */
    iterator insert_equal(const value_type& v);

    /**
     * @brief 位置提示插入（允许重复键）
     * @param position 位置提示迭代器
     * @param v 要插入的值
     * @return 指向插入位置的迭代器
     * @note 类似std::multimap::insert(hint, value)
     */
    iterator insert_equal(const_iterator position, const value_type& v);

    /**
     * @brief 批量插入接口（允许重复键）
     */
    void insert_equal(const_iterator first, const_iterator last);
    void insert_equal(const value_type* first, const value_type* last);
    template <class InputIterator>
    void insert_equal(InputIterator first, InputIterator last);

    /**
     * @brief 就地构造插入（允许重复键）
     * @param args 构造参数
     * @return 指向插入位置的迭代器
     * @note 类似std::multimap::emplace()
     */
    template <class... Args>
    iterator emplace_equal(const Args&... args);

    /**
     * @brief 位置提示就地构造（允许重复键）
     * @param position 位置提示
     * @param args 构造参数
     * @return 指向插入位置的迭代器
     * @note 类似std::multimap::emplace_hint()
     */
    template <class... Args>
    iterator emplace_hint_equal(const_iterator position, const Args&... args);

    // ==================== STL兼容删除接口 ====================

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的元素位置
     * @return 指向下一个元素的迭代器
     * @note 与STL对比：
     *       - STL: std::map::erase(iterator)
     *       - 行为完全一致，复杂度：O(log n)
     */
    iterator erase(iterator position);
    iterator erase(const_iterator position);

    /**
     * @brief 删除指定键的所有元素
     * @param k 要删除的键
     * @return 删除的元素数量
     * @note 与STL对比：
     *       - STL: std::map::erase(key) / std::multimap::erase(key)
     *       - 行为完全一致，复杂度：O(log n + k) k为删除元素数
     */
    size_type erase(const key_type& k);

    /**
     * @brief 删除范围内的元素
     * @param first 起始位置
     * @param last 结束位置
     * @return 指向删除范围后下一个元素的迭代器
     * @note 与STL对比：
     *       - STL: std::map::erase(first, last)
     *       - 行为完全一致，复杂度：O(log n + k)
     */
    iterator erase(const_iterator first, const_iterator last);

    /**
     * @brief 删除键数组中的所有键
     * @param first 键数组起始指针
     * @param last 键数组结束指针
     * @note STL没有直接对应接口，便利性扩展
     */
    void erase(const key_type* first, const key_type* last);

    /**
     * @brief 清空所有元素
     * @note 与STL对比：
     *       - STL: std::map::clear()
     *       - 行为一致，但不释放内存池，复杂度：O(n)
     */
    void clear(); // 清空树

    /**
     * @brief 交换两个红黑树的内容
     * @param x 另一个红黑树
     * @note 与STL对比：
     *       - STL: std::map::swap()
     *       - 由于固定内存布局，需要逐个元素交换
     *       - 复杂度：O(MAX_SIZE) 而非STL的O(1)
     */
    void swap(NFShmRBTree& x) noexcept; // 交换两棵树的内容

    // ==================== STL兼容查询接口 ====================

    /**
     * @brief 返回键比较器
     * @return 键比较器对象
     * @note 与STL对比：
     *       - STL: std::map::key_comp()
     *       - 行为完全一致，复杂度：O(1)
     */
    key_compare key_comp() const; // 返回键比较器

    /**
     * @brief 查找指定键的元素
     * @param k 要查找的键
     * @return 指向找到元素的迭代器，未找到返回end()
     * @note 与STL对比：
     *       - STL: std::map::find()
     *       - 行为完全一致，复杂度：O(log n)
     */
    iterator find(const key_type& k); // 查找指定键的元素
    const_iterator find(const key_type& k) const;

    /**
     * @brief 计算指定键的元素数量
     * @param k 要统计的键
     * @return 该键的元素数量
     * @note 与STL对比：
     *       - STL: std::map::count() 返回0或1, std::multimap::count() 可能>1
     *       - 本实现支持两种模式，复杂度：O(log n + k)
     */
    size_type count(const key_type& k) const; // 计算指定键的元素数量

    /**
     * @brief 查找第一个不小于k的元素
     * @param k 查找的键
     * @return 指向不小于k的第一个元素的迭代器
     * @note 与STL对比：
     *       - STL: std::map::lower_bound()
     *       - 行为完全一致，复杂度：O(log n)
     */
    iterator lower_bound(const key_type& k); // 返回不小于k的第一个位置
    const_iterator lower_bound(const key_type& k) const;

    /**
     * @brief 查找第一个大于k的元素
     * @param k 查找的键
     * @return 指向大于k的第一个元素的迭代器
     * @note 与STL对比：
     *       - STL: std::map::upper_bound()
     *       - 行为完全一致，复杂度：O(log n)
     */
    iterator upper_bound(const key_type& k); // 返回大于k的第一个位置
    const_iterator upper_bound(const key_type& k) const;

    /**
     * @brief 返回与指定键相等的元素范围
     * @param k 查找的键
     * @return pair<iterator, iterator> 表示[first, last)范围
     * @note 与STL对比：
     *       - STL: std::map::equal_range()
     *       - 行为完全一致，复杂度：O(log n)
     *       - 对于unique模式：范围最多包含1个元素
     *       - 对于equal模式：范围可能包含多个元素
     */
    std::pair<iterator, iterator> equal_range(const key_type& k); // 返回与k相等的范围
    std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const;

    // ==================== 红黑树特有验证接口 ====================

    /**
     * @brief 验证红黑树性质（STL兼容接口）
     * @return true表示满足红黑树性质
     * @note 与STL对比：
     *       - STL: std::map没有公开验证接口
     *       - 用于调试和测试，生产环境可关闭
     */
    bool __rb_verify() const;

    /**
     * @brief 验证红黑树性质（推荐接口）
     * @return true表示满足红黑树性质
     * @note STL没有对应接口，调试特有功能
     *       - 验证所有红黑树性质
     *       - 检查节点指针完整性
     *       - 验证有序性
     */
    bool RbVerify() const; // 验证红黑树性质

protected:
    // 内部节点访问与管理函数
    Node* node(); // 获取节点数组
    const Node* node() const;
    NodeBase* GetNode(int index); // 根据索引获取节点
    const NodeBase* GetNode(int index) const;

protected:
    template <typename... Args>
    Node* CreateNode(const Args&... args); // 创建新节点
    void RecycleNode(Node* pNode); // 回收节点

    // 树结构管理函数
    NodeBase* GetHeader(); // 获取头节点
    const NodeBase* GetHeader() const;
    int GetHeaderIndex() const;

    NodeBase* GetRoot(); // 获取根节点
    const NodeBase* GetRoot() const;
    int GetRootIndex() const;
    void SetRootIndex(int iIndex);

    void Erase(NodeBase* x); // 递归删除子树

    NodeBase* Minimum(const NodeBase* x) const; // 获取子树最小节点
    NodeBase* Maximum(const NodeBase* x) const; // 获取子树最大节点

    static const Key& GetKey(const NodeBase* x); // 获取节点键值
protected:
    // 红黑树平衡操作
    void RotateLeft(NodeBase* x); // 左旋操作
    void RotateRight(NodeBase* x); // 右旋操作
    void RebalanceForInsert(NodeBase* x); // 插入后重平衡
    void RebalanceForErase(NodeBase* x, NodeBase* xParent); // 删除后重平衡
protected:
    iterator InsertNode(NodeBase* x, NodeBase* y, const value_type& v); // 插入节点的内部实现
    int BlackCount(const NFShmRBTreeNodeBase* node, const NFShmRBTreeNodeBase* root) const; // 计算黑色节点数
    // 删除函数
    void EraseAux(const key_type* first, const key_type* last);
    void EraseAux(const_iterator position); // 删除指定位置的元素
    size_type EraseAux(const key_type& k); // 删除指定键的元素，返回删除的元素数量
    void EraseAux(const_iterator first, const_iterator last); // 删除范围内的元素

private:
    // ==================== 打印辅助函数 ====================

    /**
     * @brief 递归打印子树结构
     * @param node 当前节点
     * @param prefix 当前行的前缀字符串
     * @param isLast 是否是最后一个子节点
     * @param isRoot 是否是根节点
     */
    void print_subtree(const NodeBase* node, const std::string& prefix, bool isLast, bool isRoot) const;

    /**
     * @brief 获取节点显示信息
     * @param node 节点指针
     * @return 节点的字符串表示
     */
    std::string get_node_info(const NodeBase* node) const;

    /**
     * @brief 计算子树高度
     * @param node 节点指针
     * @return 子树高度
     */
    int calculate_height(const NodeBase* node) const;

    /**
     * @brief 统计节点颜色数量
     * @param node 节点指针
     * @param redCount 红色节点计数
     * @param blackCount 黑色节点计数
     */
    void count_colors(const NodeBase* node, int& redCount, int& blackCount) const;
public:
    // ==================== 调试和诊断接口（STL扩展） ====================

    /**
     * @brief 打印红黑树结构（图形化显示）
     * @note 与STL对比：
     *       - STL: 无对应接口
     *       - 调试特有功能，以ASCII图形显示树结构
     *       - 显示节点颜色、键值、索引信息
     *       - 展示父子关系和树的层次结构
     * 
     * 输出示例：
     * ```
     * Tree structure (Left=smaller, Right=larger):
     * Format: Key(Color)[Index]
     * 
     * 50(B)[3]
     * ├── 20(R)[1]
     * │   ├── 10(B)[0]
     * │   └── 30(B)[2]
     * └── 70(R)[4]
     *     ├── 60(B)[5]
     *     └── 80(B)[6]
     * ```
     */
    void print_structure() const;

    /**
     * @brief 打印详细信息
     * @note 与STL对比：
     *       - STL: 无对应接口
     *       - 显示所有节点的完整状态和属性
     *       - 包含内存布局、空闲链表、节点索引表
     *       - 提供红黑树完整性验证结果
     * 
     * 输出内容：
     * - 节点状态表（Valid, Color, Parent, Left, Right, SelfRef, Key Info）
     * - 空闲链表状态
     * - 树统计信息（高度、红黑节点数量、验证结果）
     */
    void print_detailed() const;

    /**
     * @brief 打印简化信息
     * @note 与STL对比：
     *       - STL: 无对应接口
     *       - 仅显示树的基本统计信息和简化的有序遍历
     *       - 适合快速查看树的状态
     * 
     * 输出内容：
     * - 容量信息（Size/MaxSize）
     * - 树高度和颜色统计
     * - 前10个元素的有序列表
     * - 红黑树有效性验证结果
     */
    void print_simple() const;
};

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
Key NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::m_staticDefaultKey = Key();

template <class KeyValue, class Ref, class Ptr, class Container>
NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::NFShmRBTreeIterator(const Container* pContainer, size_t iPos): NFShmRBTreeIteratorBase<Container>(pContainer, iPos)
{
}

template <class KeyValue, class Ref, class Ptr, class Container>
NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::NFShmRBTreeIterator(const Container* pContainer, const NFShmRBTreeNodeBase* pNode): NFShmRBTreeIteratorBase<Container>(pContainer, pNode)
{
}

template <class KeyValue, class Ref, class Ptr, class Container>
NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::NFShmRBTreeIterator()
{
}

template <class KeyValue, class Ref, class Ptr, class Container>
NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::NFShmRBTreeIterator(const iterator& x): NFShmRBTreeIteratorBase<Container>(x.m_pContainer, x.m_node)
{
}

template <class KeyValue, class Ref, class Ptr, class Container>
typename NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::reference NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::operator*() const
{
    CHECK_EXPR(m_node, m_staticError, "TRACE_STACK:%s", TRACE_STACK());
    return ((Node*)m_node)->m_data;
}

template <class KeyValue, class Ref, class Ptr, class Container>
typename NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::pointer NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::operator->() const
{
    return &(operator*());
}

template <class KeyValue, class Ref, class Ptr, class Container>
typename NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::Self& NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::operator++()
{
    this->Increment();
    return *this;
}

template <class KeyValue, class Ref, class Ptr, class Container>
typename NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::Self NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::operator++(int)
{
    Self tmp = *this;
    this->Increment();
    return tmp;
}

template <class KeyValue, class Ref, class Ptr, class Container>
typename NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::Self& NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::operator--()
{
    this->Decrement();
    return *this;
}

template <class KeyValue, class Ref, class Ptr, class Container>
typename NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::Self NFShmRBTreeIterator<KeyValue, Ref, Ptr, Container>::operator--(int)
{
    Self tmp = *this;
    this->Decrement();
    return tmp;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NFShmRBTree()
{
    // 根据模式选择初始化方法
    if (SHM_CREATE_MODE)
    {
        CreateInit();
    }
    else
    {
        ResumeInit();
    }
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NFShmRBTree(const NFShmRBTree& tree)
{
    m_init = EN_NF_SHM_STL_INIT_OK;
    if (tree.GetRoot() == nullptr)
    {
        CreateInit(); // 源树为空，执行创建初始化
    }
    else
    {
        // 直接复制树的内容
        m_size = tree.m_size;
        m_freeStart = tree.m_freeStart;

        for (size_t i = 0; i <= MAX_SIZE; i++)
        {
            m_mem[i] = tree.m_mem[i];
        }
    }
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
int NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::CreateInit()
{
    m_size = 0;
    m_freeStart = 0;
    memset(m_mem, 0, sizeof(m_mem));

    auto pNode = node();
    // 初始化空闲节点链表，形成一个单向链表
    for (size_t i = 0; i < MAX_SIZE; i++)
    {
        pNode[i].m_parent = INVALID_ID;
        pNode[i].m_left = INVALID_ID;
        pNode[i].m_right = i + 1; // 指向下一个节点
        pNode[i].m_color = RB_RED;
        pNode[i].m_valid = false;
        pNode[i].m_self = i;
    }

    pNode[MAX_SIZE - 1].m_right = INVALID_ID; // 最后一个节点的右指针为空

    // 初始化头节点（哨兵节点）
    pNode[MAX_SIZE].m_parent = INVALID_ID; // 父节点指向根节点
    pNode[MAX_SIZE].m_left = MAX_SIZE; // 左指针指向最小节点
    pNode[MAX_SIZE].m_right = MAX_SIZE; // 右指针指向最大节点
    pNode[MAX_SIZE].m_color = RB_RED;
    pNode[MAX_SIZE].m_valid = true;
    pNode[MAX_SIZE].m_self = MAX_SIZE;

    m_init = EN_NF_SHM_STL_INIT_OK;
#ifdef NF_DEBUG_MODE
    m_ptr = (Node*)m_mem;
#endif
    return 0;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
int NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::ResumeInit()
{
    if (m_init == EN_NF_SHM_STL_INIT_OK)
    {
        auto pNode = node();
        // 对于非平凡构造的类型，恢复时需要调用构造函数
        if (!std::stl_is_trivially_default_constructible<KeyValue>::value)
        {
            for (size_t i = 0; i < MAX_SIZE; i++)
            {
                if (pNode[i].m_valid)
                {
                    std::_Construct(&pNode[i].m_data);
                }
            }
        }
    }

#ifdef NF_DEBUG_MODE
    m_ptr = (Node*)m_mem;
#endif
    return 0;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::Node* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::node()
{
    return reinterpret_cast<Node*>(m_mem); // 将内存池转换为节点数组
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
const typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::Node* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::node() const
{
    return reinterpret_cast<const Node*>(m_mem);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NodeBase* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::GetNode(int index)
{
    if (index >= 0 && index <= MAX_SIZE)
    {
        auto pNode = node();
        return &pNode[index];
    }
    CHECK_EXPR(index == INVALID_ID, nullptr, "invalid node index:%d, expected INVALID_ID or valid range [0, %d], TRACE_STACK:%s", index, (int)MAX_SIZE, TRACE_STACK());
    return nullptr;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
const typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NodeBase* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::GetNode(int index) const
{
    if (index >= 0 && index <= MAX_SIZE)
    {
        auto pNode = node();
        return &pNode[index];
    }
    CHECK_EXPR(index == INVALID_ID, nullptr, "invalid node index:%d, expected INVALID_ID or valid range [0, %d], TRACE_STACK:%s", index, (int)MAX_SIZE, TRACE_STACK());
    return nullptr;
}

// 创建新节点
template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
template <typename... Args>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::Node* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::CreateNode(const Args&... args)
{
    CHECK_EXPR(m_freeStart >= 0 && m_freeStart < MAX_SIZE, nullptr, "no free node available, free start index:%d, TRACE_STACK:%s", (int)m_freeStart, TRACE_STACK());

    // 从空闲链表头取一个节点
    ptrdiff_t iSelf = m_freeStart;
    auto pNode = node();
    m_freeStart = pNode[m_freeStart].m_right; // 更新空闲链表头

    // 构造数据
    std::_Construct(&pNode[iSelf].m_data, args...);

    CHECK_EXPR(!pNode[iSelf].m_valid, nullptr, "node already valid, index:%d, TRACE_STACK:%s", (int)iSelf, TRACE_STACK());
    CHECK_EXPR(pNode[iSelf].m_self == iSelf, nullptr, "node self index mismatch: expected %d, got %d, TRACE_STACK:%s", (int)iSelf, (int)pNode[iSelf].m_self, TRACE_STACK());

    // 初始化节点属性
    pNode[iSelf].m_valid = true;
    pNode[iSelf].m_parent = INVALID_ID;
    pNode[iSelf].m_left = INVALID_ID;
    pNode[iSelf].m_right = INVALID_ID;

    return &pNode[iSelf];
}

// 回收节点
template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::RecycleNode(Node* pNode)
{
    CHECK_EXPR_RE_VOID(pNode, "attempt to recycle null node, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(pNode->m_valid, "attempt to recycle invalid node, index:%d, TRACE_STACK:%s", (int)pNode->m_self, TRACE_STACK());

    // 销毁数据
    std::_Destroy(&(pNode->m_data));

    // 将节点插入空闲链表头部
    pNode->m_valid = false;
    pNode->m_right = m_freeStart;
    m_freeStart = pNode->m_self;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NodeBase* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::GetHeader()
{
    return GetNode(MAX_SIZE);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
const typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NodeBase* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::GetHeader() const
{
    return GetNode(MAX_SIZE);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
int NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::GetHeaderIndex() const
{
    return MAX_SIZE;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NodeBase* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::GetRoot()
{
    return GetNode(GetHeader()->m_parent);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
const typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NodeBase* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::GetRoot() const
{
    return GetNode(GetHeader()->m_parent);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
int NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::GetRootIndex() const
{
    return GetHeader()->m_parent;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::SetRootIndex(int iIndex)
{
    GetHeader()->m_parent = iIndex;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::Erase(NodeBase* x)
{
    // 递归删除以x为根的子树
    while (x != nullptr)
    {
        // 先递归删除右子树
        if (x->m_right != INVALID_ID)
        {
            NodeBase* rightNode = GetNode(x->m_right);
            if (rightNode != nullptr)
            {
                Erase(rightNode);
            }
        }

        // 保存左子节点，因为当前节点即将被删除
        NodeBase* y = (x->m_left != INVALID_ID) ? GetNode(x->m_left) : nullptr;

        // 回收当前节点
        RecycleNode((Node*)x);

        // 移动到左子节点继续处理
        x = y;
    }
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NodeBase* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::Minimum(const NodeBase* x) const
{
    CHECK_EXPR(x != nullptr, nullptr, "input node is null, TRACE_STACK:%s", TRACE_STACK());

    while (x != nullptr && x->m_left != INVALID_ID)
    {
        const NodeBase* left = GetNode(x->m_left);
        if (left == nullptr)
            break;
        x = left;
    }
    return const_cast<NodeBase*>(x);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::NodeBase* NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::Maximum(const NodeBase* x) const
{
    CHECK_EXPR(x != nullptr, nullptr, "input node is null, TRACE_STACK:%s", TRACE_STACK());

    while (x != nullptr && x->m_right != INVALID_ID)
    {
        const NodeBase* right = GetNode(x->m_right);
        if (right == nullptr)
            break;
        x = right;
    }
    return const_cast<NodeBase*>(x);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
const Key& NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::GetKey(const NodeBase* x)
{
    CHECK_EXPR(x != nullptr, m_staticDefaultKey, "input node is null, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(((const Node*)x)->m_valid, m_staticDefaultKey, "node is not valid, TRACE_STACK:%s", TRACE_STACK());
    return KeyOfValue()(((const Node*)x)->m_data);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::RotateLeft(NodeBase* x)
{
    CHECK_EXPR_RE_VOID(x, "x == nullptr, TRACE_STACK:%s", TRACE_STACK());
    NodeBase* y = GetNode(x->m_right); // 获取右子节点
    CHECK_EXPR_RE_VOID(y, "y == nullptr, TRACE_STACK:%s", TRACE_STACK());

    // 左旋示意图:
    //
    //     P                 P
    //     |                 |
    //     x                 y
    //    / \      -->      / \
    //   α   y             x   γ
    //      / \           / \
    //     β   γ         α   β
    //
    // 步骤1: 将y的左子树β变为x的右子树
    x->m_right = y->m_left;
    if (GetNode(y->m_left))
        GetNode(y->m_left)->m_parent = x->m_self;

    // 步骤2: 更新y的父节点为x的父节点
    y->m_parent = x->m_parent;

    // 步骤3: 根据x的位置更新父节点的子节点指针
    if (x == GetRoot())
        SetRootIndex(y->m_self); // x是根节点，更新根节点为y
    else
    {
        CHECK_EXPR_RE_VOID(GetNode(x->m_parent), "GetNode(x->m_parent) == nullptr, TRACE_STACK:%s", TRACE_STACK());
        if (x == GetNode(GetNode(x->m_parent)->m_left))
            GetNode(x->m_parent)->m_left = y->m_self; // x是左子节点
        else
            GetNode(x->m_parent)->m_right = y->m_self; // x是右子节点
    }

    // 步骤4: 设置x为y的左子节点
    y->m_left = x->m_self;
    x->m_parent = y->m_self;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::RotateRight(NodeBase* x)
{
    CHECK_EXPR_RE_VOID(x, "x == nullptr, TRACE_STACK:%s", TRACE_STACK());
    NodeBase* y = GetNode(x->m_left); // 获取左子节点
    CHECK_EXPR_RE_VOID(y, "y == nullptr, TRACE_STACK:%s", TRACE_STACK());

    // 右旋示意图:
    //
    //       P                P
    //       |                |
    //       x                y
    //      / \     -->      / \
    //     y   γ            α   x
    //    / \                  / \
    //   α   β                β   γ
    //
    // 步骤1: 将y的右子树β变为x的左子树
    x->m_left = y->m_right;
    if (GetNode(y->m_right))
    {
        GetNode(y->m_right)->m_parent = x->m_self;
    }

    // 步骤2: 更新y的父节点为x的父节点
    y->m_parent = x->m_parent;

    // 步骤3: 根据x的位置更新父节点的子节点指针
    if (x == GetRoot())
    {
        SetRootIndex(y->m_self); // x是根节点，更新根节点为y
    }
    else
    {
        CHECK_EXPR_RE_VOID(GetNode(x->m_parent), "GetNode(x->m_parent) == nullptr, TRACE_STACK:%s", TRACE_STACK());
        if (x == GetNode(GetNode(x->m_parent)->m_right))
        {
            GetNode(x->m_parent)->m_right = y->m_self; // x是右子节点
        }
        else
        {
            GetNode(x->m_parent)->m_left = y->m_self; // x是左子节点
        }
    }
    // 步骤4: 设置x为y的右子节点
    y->m_right = x->m_self;
    x->m_parent = y->m_self;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::RebalanceForInsert(NodeBase* x)
{
    CHECK_EXPR_RE_VOID(x, "x == nullptr, TRACE_STACK:%s", TRACE_STACK());

    // 将新插入的节点着色为红色
    x->m_color = RB_RED;

    // 只有当节点不是根节点且父节点是红色时才需要调整（红黑树性质4：红色节点的子节点必须是黑色）
    while (x != GetRoot())
    {
        // 检查父节点是否存在且为红色
        NodeBase* parent = GetNode(x->m_parent);
        if (!parent || parent->m_color != RB_RED)
            break;

        //父节点是红色的且不是根节点，所以父节点的父节点（祖父节点）一定存在
        NodeBase* grandparent = GetNode(parent->m_parent);
        CHECK_EXPR_RE_VOID(grandparent, "grandparent == nullptr, TRACE_STACK:%s", TRACE_STACK());

        // 情况A：父节点是祖父节点的左子节点
        if (parent == GetNode(grandparent->m_left))
        {
            NodeBase* y = GetNode(grandparent->m_right); // 叔叔节点

            // 情况A1：叔叔节点存在且为红色
            // 图例：
            //      祖父(黑)                               祖父(红)
            //     /        \                            /        \
            //   父(红)     叔叔(红)     ------>      父(黑)      叔叔(黑)
            //   /                                    /
            // 当前(红)                             当前(红)
            // 处理：父节点和叔叔节点变黑，祖父节点变红，然后向上递归处理祖父节点
            if (y != nullptr && y->m_color == RB_RED)
            {
                parent->m_color = RB_BLACK; // 父节点变黑
                y->m_color = RB_BLACK; // 叔叔节点变黑
                grandparent->m_color = RB_RED; // 祖父节点变红
                x = grandparent; // 将祖父节点作为新的当前节点继续向上调整
            }
            else // 叔叔节点不存在或为黑色
            {
                // 情况A2：当前节点是父节点的右子节点（需要先左旋转为情况A3）
                // 图例：
                //      祖父(黑)                               祖父(黑)
                //     /        \                            /        \
                //   父(红)     叔叔(黑)     ------>      父(红)      叔叔(黑)
                //      \                                /
                //     当前(红)                        当前(红)
                // 处理：左旋父节点，转化为情况A3
                if (x == GetNode(parent->m_right))
                {
                    x = parent;
                    RotateLeft(x); // 左旋
                    parent = GetNode(x->m_parent); // 更新父节点指针
                    CHECK_EXPR_RE_VOID(parent, "parent == nullptr after rotation, TRACE_STACK:%s", TRACE_STACK());
                    grandparent = GetNode(parent->m_parent);
                    CHECK_EXPR_RE_VOID(grandparent, "grandparent == nullptr after rotation, TRACE_STACK:%s", TRACE_STACK());
                }

                // 情况A3：当前节点是父节点的左子节点
                // 图例（左旋转后）：
                //      祖父(黑)                               父(黑)
                //     /        \                            /        \
                //   父(红)     叔叔(黑)     ------>      当前(红)     祖父(红)
                //   /                                                  \
                // 当前(红)                                            叔叔(黑)
                // 处理：父节点变黑，祖父节点变红，然后右旋祖父节点

                parent->m_color = RB_BLACK; // 父节点变黑
                grandparent->m_color = RB_RED; // 祖父节点变红
                RotateRight(grandparent); // 右旋祖父节点
            }
        }
        else // 情况B：父节点是祖父节点的右子节点（与情况A对称）
        {
            NodeBase* y = GetNode(grandparent->m_left); // 叔叔节点

            // 情况B1：叔叔节点存在且为红色
            // 图例：
            //        祖父(黑)                              祖父(红)
            //       /        \                           /        \
            //   叔叔(红)     父(红)     ------>     叔叔(黑)      父(黑)
            //                   \                                   \
            //                 当前(红)                             当前(红)
            // 处理：父节点和叔叔节点变黑，祖父节点变红，然后向上递归处理祖父节点
            if (y != nullptr && y->m_color == RB_RED)
            {
                parent->m_color = RB_BLACK; // 父节点变黑
                y->m_color = RB_BLACK; // 叔叔节点变黑
                grandparent->m_color = RB_RED; // 祖父节点变红
                x = grandparent; // 将祖父节点作为新的当前节点继续向上调整
            }
            else // 叔叔节点不存在或为黑色
            {
                // 情况B2：当前节点是父节点的左子节点（需要先右旋转为情况B3）
                // 图例：
                //        祖父(黑)                               祖父(黑)
                //       /        \                            /        \
                //   叔叔(黑)     父(红)      ------>      叔叔(黑)     父(红)
                //               /                                         \
                //           当前(红)                                     当前(红)
                // 处理：右旋父节点，转化为情况B3
                if (x == GetNode(parent->m_left))
                {
                    x = parent;
                    RotateRight(x); // 右旋
                    parent = GetNode(x->m_parent); // 更新父节点指针
                    CHECK_EXPR_RE_VOID(parent, "parent == nullptr after rotation, TRACE_STACK:%s", TRACE_STACK());
                    grandparent = GetNode(parent->m_parent);
                    CHECK_EXPR_RE_VOID(grandparent, "grandparent == nullptr after rotation, TRACE_STACK:%s", TRACE_STACK());
                }

                // 情况B3：当前节点是父节点的右子节点
                // 图例（右旋转后）：
                //        祖父(黑)                               父(黑)
                //       /        \                            /        \
                //   叔叔(黑)     父(红)      ------>      祖父(红)    当前(红)
                //                   \                      /
                //                 当前(红)               叔叔(黑)
                // 处理：父节点变黑，祖父节点变红，然后左旋祖父节点
                parent->m_color = RB_BLACK; // 父节点变黑
                grandparent->m_color = RB_RED; // 祖父节点变红
                RotateLeft(grandparent); // 左旋祖父节点
            }
        }
    }

    // 红黑树性质2：根节点必须是黑色的
    NodeBase* root = GetRoot();
    CHECK_EXPR_RE_VOID(root, "GetRoot() == nullptr, TRACE_STACK:%s", TRACE_STACK());
    root->m_color = RB_BLACK;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::RebalanceForErase(NodeBase* x, NodeBase* xParent)
{
    CHECK_EXPR_RE_VOID(xParent, "xParent == nullptr, TRACE_STACK:%s", TRACE_STACK());

    // 需要调整的情况：x为空或为黑色节点，且不是根节点
    // 删除黑色节点会破坏红黑树性质5（从任一节点到其每个叶子的所有路径都包含相同数量的黑色节点）
    while (x != GetRoot() && (!x || x->m_color == RB_BLACK))
    {
        // 情况A：x是父节点的左子节点
        if (x == GetNode(xParent->m_left))
        {
            NodeBase* w = GetNode(xParent->m_right); // 兄弟节点
            CHECK_EXPR_RE_VOID(w, "w == nullptr, TRACE_STACK:%s", TRACE_STACK());

            // 情况A1：兄弟节点为红色
            // 图例：
            //     父(任意色)                               父(红)
            //    /        \                            /        \
            //  x(黑)     兄弟(红)     ------>      x(黑)       兄弟子(黑)
            //                \                                    \
            //             兄弟子(黑)                             兄弟(红)
            // 处理：兄弟节点变黑，父节点变红，对父节点进行左旋，转换为其他情况继续处理
            if (w->m_color == RB_RED)
            {
                w->m_color = RB_BLACK; // 兄弟节点变黑
                xParent->m_color = RB_RED; // 父节点变红
                RotateLeft(xParent); // 左旋父节点
                w = GetNode(xParent->m_right); // 更新兄弟节点
                CHECK_EXPR_RE_VOID(w, "w == nullptr after rotation, TRACE_STACK:%s", TRACE_STACK());
            }

            // 情况A2：兄弟节点为黑色，且兄弟节点的两个子节点都是黑色
            // 图例：
            //     父(任意色)                               父(黑)
            //    /        \                            /        \
            //  x(黑)     兄弟(黑)     ------>      x(黑)       兄弟(红)
            //           /     \                              /     \
            //        侄左(黑) 侄右(黑)                    侄左(黑) 侄右(黑)
            // 处理：将兄弟节点变红，问题转移到父节点，继续向上调整（父节点作为新的x节点）
            if ((GetNode(w->m_left) == nullptr || GetNode(w->m_left)->m_color == RB_BLACK) &&
                (GetNode(w->m_right) == nullptr || GetNode(w->m_right)->m_color == RB_BLACK))
            {
                w->m_color = RB_RED; // 兄弟节点变红
                x = xParent; // 向上调整
                xParent = GetNode(x->m_parent);
                CHECK_EXPR_RE_VOID(xParent, "xParent == nullptr, TRACE_STACK:%s", TRACE_STACK());
            }
            else
            {
                // 情况A3：兄弟节点为黑色，兄弟的左子节点为红色，右子节点为黑色
                // 图例：
                //     父(任意色)                              父(任意色)
                //    /        \                            /           \
                //  x(黑)     兄弟(黑)     ------>      x(黑)         侄左(黑)
                //           /     \                                   /    \
                //        侄左(红) 侄右(黑)                        兄弟(红)  侄右(黑)
                // 处理：兄弟节点变红，兄弟的左子节点变黑，对兄弟节点进行右旋，转换为情况A4
                if (GetNode(w->m_right) == nullptr || GetNode(w->m_right)->m_color == RB_BLACK)
                {
                    if (GetNode(w->m_left))
                        GetNode(w->m_left)->m_color = RB_BLACK; // 左子节点变黑
                    w->m_color = RB_RED; // 兄弟节点变红
                    RotateRight(w); // 右旋兄弟节点
                    w = GetNode(xParent->m_right);
                    CHECK_EXPR_RE_VOID(w, "w == nullptr after right rotation, TRACE_STACK:%s", TRACE_STACK());
                }

                // 情况A4：兄弟节点为黑色，兄弟的右子节点为红色
                // 图例：
                //     父(任意色)                              兄弟(父的颜色)
                //    /        \                            /           \
                //  x(黑)     兄弟(黑)     ------>      父(黑)         侄右(黑)
                //           /     \                     /
                //        侄左(任意) 侄右(红)          x(黑)
                // 处理：兄弟节点继承父节点的颜色，父节点变黑，兄弟的右子节点变黑，对父节点进行左旋
                w->m_color = xParent->m_color; // 兄弟节点继承父节点颜色
                xParent->m_color = RB_BLACK; // 父节点变黑
                if (GetNode(w->m_right))
                    GetNode(w->m_right)->m_color = RB_BLACK; // 右子节点变黑
                RotateLeft(xParent); // 左旋父节点
                break;
            }
        }
        else // 情况B：x是父节点的右子节点（与情况A对称）
        {
            NodeBase* w = GetNode(xParent->m_left); // 兄弟节点
            CHECK_EXPR_RE_VOID(w, "w == nullptr, TRACE_STACK:%s", TRACE_STACK());

            // 情况B1：兄弟节点为红色
            // 图例：
            //       父(任意色)                               父(红)
            //      /        \                            /        \
            //  兄弟(红)     x(黑)     ------>      兄弟子(黑)       x(黑)
            //    /                                   /
            // 兄弟子(黑)                          兄弟(红)
            // 处理：兄弟节点变黑，父节点变红，对父节点进行右旋，转换为其他情况继续处理
            if (w->m_color == RB_RED)
            {
                w->m_color = RB_BLACK; // 兄弟节点变黑
                xParent->m_color = RB_RED; // 父节点变红
                RotateRight(xParent); // 右旋父节点
                w = GetNode(xParent->m_left); // 更新兄弟节点
                CHECK_EXPR_RE_VOID(w, "w == nullptr after rotation, TRACE_STACK:%s", TRACE_STACK());
            }

            // 情况B2：兄弟节点为黑色，且兄弟节点的两个子节点都是黑色
            // 图例：
            //       父(任意色)                               父(黑)
            //      /        \                            /        \
            //  兄弟(黑)     x(黑)     ------>      兄弟(红)       x(黑)
            //  /     \                            /     \
            // 侄左(黑) 侄右(黑)                  侄左(黑) 侄右(黑)
            // 处理：将兄弟节点变红，问题转移到父节点，继续向上调整（父节点作为新的x节点）
            if ((GetNode(w->m_right) == nullptr || GetNode(w->m_right)->m_color == RB_BLACK) &&
                (GetNode(w->m_left) == nullptr || GetNode(w->m_left)->m_color == RB_BLACK))
            {
                w->m_color = RB_RED; // 兄弟节点变红
                x = xParent; // 向上调整
                xParent = GetNode(x->m_parent);
                CHECK_EXPR_RE_VOID(xParent, "xParent == nullptr, TRACE_STACK:%s", TRACE_STACK());
            }
            else
            {
                // 情况B3：兄弟节点为黑色，兄弟的右子节点为红色，左子节点为黑色
                // 图例：
                //       父(任意色)                              父(任意色)
                //      /        \                            /           \
                //  兄弟(黑)     x(黑)     ------>        侄右(黑)        x(黑)
                //  /     \                                /    \
                // 侄左(黑) 侄右(红)                    兄弟(红)  侄左(黑)
                // 处理：兄弟节点变红，兄弟的右子节点变黑，对兄弟节点进行左旋，转换为情况B4
                if (GetNode(w->m_left) == nullptr || GetNode(w->m_left)->m_color == RB_BLACK)
                {
                    if (w->m_right != INVALID_ID && GetNode(w->m_right))
                        GetNode(w->m_right)->m_color = RB_BLACK; // 右子节点变黑
                    w->m_color = RB_RED; // 兄弟节点变红
                    RotateLeft(w); // 左旋兄弟节点
                    w = GetNode(xParent->m_left);
                    CHECK_EXPR_RE_VOID(w, "w == nullptr after left rotation, TRACE_STACK:%s", TRACE_STACK());
                }

                // 情况B4：兄弟节点为黑色，兄弟的左子节点为红色
                // 图例：
                //       父(任意色)                            兄弟(父的颜色)
                //      /        \                            /           \
                //  兄弟(黑)     x(黑)     ------>        侄左(黑)        父(黑)
                //  /     \                                               /
                // 侄左(红) 侄右(任意)                                  x(黑)
                // 处理：兄弟节点继承父节点的颜色，父节点变黑，兄弟的左子节点变黑，对父节点进行右旋
                w->m_color = xParent->m_color; // 兄弟节点继承父节点颜色
                xParent->m_color = RB_BLACK; // 父节点变黑
                if (GetNode(w->m_left))
                    GetNode(w->m_left)->m_color = RB_BLACK; // 左子节点变黑
                RotateRight(xParent); // 右旋父节点
                break;
            }
        }
    }

    // 确保x节点为黑色，如果x是根节点或者已经是红色，则变为黑色满足红黑树性质
    if (x)
        x->m_color = RB_BLACK;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::begin()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    return iterator(this, GetHeader()->m_left);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::const_iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::begin() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    return const_iterator(this, GetHeader()->m_left);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::end()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    return iterator(this, MAX_SIZE);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::const_iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::end() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    return const_iterator(this, MAX_SIZE);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::reverse_iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::rbegin()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, reverse_iterator(iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
    return reverse_iterator(end());
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::const_reverse_iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::rbegin() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(const_iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
    return const_reverse_iterator(end());
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::reverse_iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::rend()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, reverse_iterator(iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
    return reverse_iterator(begin());
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::const_reverse_iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::rend() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(const_iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
    return const_reverse_iterator(begin());
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::empty() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, true, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size == 0;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::full() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, true, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size >= MAX_SIZE;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::size_type NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::size() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_size;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::size_type NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::max_size() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
    return MAX_SIZE;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
std::pair<typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator, bool> NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_unique(const value_type& v)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::make_pair(iterator(this, MAX_SIZE), false), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), std::make_pair(end(), false), "tree is full, TRACE_STACK:%s", TRACE_STACK());

    NodeBase* y = GetHeader();
    NodeBase* x = GetRoot();
    bool comp = true;

    while (x != nullptr)
    {
        y = x;
        comp = m_keyCompare(KeyOfValue()(v), GetKey(x));
        x = GetNode(comp ? x->m_left : x->m_right);
    }

    iterator j = iterator(this, y);

    if (comp)
    {
        if (j == begin())
            return std::pair<iterator, bool>(InsertNode(x, y, v), true);
        else
            --j;
    }

    if (m_keyCompare(GetKey(j.m_node), KeyOfValue()(v)))
        return std::pair<iterator, bool>(InsertNode(x, y, v), true);

    return std::pair<iterator, bool>(j, false);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_equal(const value_type& v)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), end(), "tree is full, TRACE_STACK:%s", TRACE_STACK());

    // 允许重复键的插入，从根节点开始查找插入位置
    NodeBase* y = GetHeader(); // 辅助节点，最终会成为新节点的父节点
    NodeBase* x = GetRoot(); // 从根节点开始查找

    // 查找适合的插入位置
    while (x != nullptr)
    {
        y = x;
        // 根据比较结果，向左或向右查找
        x = GetNode(m_keyCompare(KeyOfValue()(v), GetKey(x)) ? x->m_left : x->m_right);
    }

    // 直接插入节点，不需要检查重复键
    return InsertNode(x, y, v);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
template <class... Args>
std::pair<typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator, bool> NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::emplace_unique(const Args&... args)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::make_pair(iterator(this, MAX_SIZE), false), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), std::make_pair(end(), false), "tree is full, TRACE_STACK:%s", TRACE_STACK());

    value_type v = value_type(args...);
    return insert_unique(v);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
template <class... Args>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::emplace_equal(const Args&... args)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), end(), "tree is full, TRACE_STACK:%s", TRACE_STACK());

    value_type v = value_type(args...);

    return insert_equal(v);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
template <class... Args>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::emplace_hint_unique(const_iterator position, const Args&... args)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), end(), "tree is full, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_pContainer == this, end(), "invalid iterator container, TRACE_STACK:%s", TRACE_STACK());

    value_type v = value_type(args...);

    return insert_unique(position, v);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
template <class... Args>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::emplace_hint_equal(const_iterator position, const Args&... args)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), end(), "tree is full, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_pContainer == this, end(), "invalid iterator container, TRACE_STACK:%s", TRACE_STACK());

    value_type v = value_type(args...);

    return insert_equal(position, v);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::EraseAux(const_iterator position)
{
    NodeBase* z = position.m_node;
    NodeBase* y = z;
    NodeBase* x = nullptr;
    NodeBase* xParent = nullptr;

    // 确定要删除的节点y和替代节点x
    if (GetNode(y->m_left) == nullptr)
    {
        x = GetNode(y->m_right);
    }
    else if (GetNode(y->m_right) == nullptr)
    {
        x = GetNode(y->m_left);
    }
    else
    {
        y = GetNode(y->m_right);
        while (GetNode(y->m_left) != nullptr)
            y = GetNode(y->m_left);
        x = GetNode(y->m_right);
    }

    if (y != z)
    {
        y->m_left = z->m_left;
        if (GetNode(z->m_left))
            GetNode(z->m_left)->m_parent = y->m_self;

        if (y != GetNode(z->m_right))
        {
            xParent = GetNode(y->m_parent);
            CHECK_EXPR_RE_VOID(xParent, "xParent == nullptr, TRACE_STACK:%s", TRACE_STACK());
            if (x) x->m_parent = y->m_parent;

            CHECK_EXPR_RE_VOID(GetNode(y->m_parent), "y->m_parent == nullptr, TRACE_STACK:%s", TRACE_STACK());
            GetNode(y->m_parent)->m_left = x ? x->m_self : INVALID_ID;
            y->m_right = z->m_right;
            CHECK_EXPR_RE_VOID(GetNode(z->m_right), "GetNode(z->m_right) == nullptr, TRACE_STACK:%s", TRACE_STACK());
            GetNode(z->m_right)->m_parent = y->m_self;
        }
        else
        {
            xParent = y;
        }

        if (GetRoot() == z)
        {
            SetRootIndex(y->m_self);
        }
        else
        {
            CHECK_EXPR_RE_VOID(GetNode(z->m_parent), "GetNode(z->m_parent) == nullptr, TRACE_STACK:%s", TRACE_STACK());
            if (GetNode(z->m_parent)->m_left == z->m_self)
            {
                GetNode(z->m_parent)->m_left = y->m_self;
            }
            else
            {
                GetNode(z->m_parent)->m_right = y->m_self;
            }
        }


        y->m_parent = z->m_parent;
        std::swap(y->m_color, z->m_color);
        y = z;
    }
    else
    {
        // y == z
        xParent = GetNode(y->m_parent);
        CHECK_EXPR_RE_VOID(xParent, "xParent == nullptr, TRACE_STACK:%s", TRACE_STACK());
        if (x) x->m_parent = y->m_parent;

        if (GetRoot() == z)
        {
            SetRootIndex(x ? x->m_self : INVALID_ID);
        }
        else
        {
            CHECK_EXPR_RE_VOID(GetNode(z->m_parent), "GetNode(z->m_parent) == nullptr, TRACE_STACK:%s", TRACE_STACK());
            if (GetNode(GetNode(z->m_parent)->m_left) == z)
            {
                GetNode(z->m_parent)->m_left = x ? x->m_self : INVALID_ID;
            }
            else
            {
                GetNode(z->m_parent)->m_right = x ? x->m_self : INVALID_ID;
            }
        }


        if (GetNode(GetHeader()->m_left) == z)
        {
            GetHeader()->m_left = (GetNode(z->m_right) == nullptr) ? z->m_parent : (x ? Minimum(x)->m_self : GetHeaderIndex());
        }
        if (GetNode(GetHeader()->m_right) == z)
        {
            GetHeader()->m_right = (GetNode(z->m_left) == nullptr) ? z->m_parent : (x ? Maximum(x)->m_self : GetHeaderIndex());
        }
    }

    if (y->m_color != RB_RED)
        RebalanceForErase(x, xParent);

    RecycleNode((Node*)y);
    --m_size;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::size_type NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::EraseAux(const key_type& k)
{
    std::pair<iterator, iterator> p = equal_range(k);
    size_type n = std::distance(p.first, p.second);
    erase(p.first, p.second);
    return n;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::EraseAux(const_iterator first, const_iterator last)
{
    if (first == begin() && last == end())
        clear();
    else
        while (first != last) erase(first++);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::clear()
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    auto pNode = node();
    // 遍历所有节点，回收有效节点
    for (size_t i = 0; i < MAX_SIZE; i++)
    {
        if (pNode[i].m_valid)
        {
            RecycleNode(&pNode[i]);
        }
    }
    // 重置树状态
    CreateInit();
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::InsertNode(NodeBase* x, NodeBase* y, const value_type& v)
{
    Node* z = CreateNode(v);
    if (!z)
        return end();

    if (y == GetHeader() ||
        x != nullptr ||
        m_keyCompare(KeyOfValue()(v), GetKey(y)))
    {
        y->m_left = z->m_self;
        if (y == GetHeader())
        {
            GetHeader()->m_parent = z->m_self;
            GetHeader()->m_right = z->m_self;
        }
        else if (y == GetNode(GetHeader()->m_left))
            GetHeader()->m_left = z->m_self;
    }
    else
    {
        y->m_right = z->m_self;
        if (y == GetNode(GetHeader()->m_right))
            GetHeader()->m_right = z->m_self;
    }
    z->m_parent = y->m_self;
    z->m_left = INVALID_ID;
    z->m_right = INVALID_ID;
    RebalanceForInsert(z);
    ++m_size;
    return iterator(this, z);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::lower_bound(const key_type& k)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    // 查找第一个不小于k的节点
    NodeBase* y = GetHeader(); // 结果初始化为header节点
    NodeBase* x = GetRoot(); // 从根节点开始搜索

    // 二分查找过程
    while (x != nullptr)
    {
        if (!m_keyCompare(GetKey(x), k)) // x.key >= k
        {
            // x的key不小于k，更新结果但继续向左查找可能更小的符合条件节点
            y = x;
            x = GetNode(x->m_left);
        }
        else // x.key < k
        {
            // x的key小于k，需要向右查找
            x = GetNode(x->m_right);
        }
    }

    return iterator(this, y);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::const_iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::lower_bound(const key_type& k) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());

    const NodeBase* y = GetHeader();
    const NodeBase* x = GetRoot();

    while (x != nullptr)
    {
        if (!m_keyCompare(GetKey(x), k))
        {
            y = x;
            x = GetNode(x->m_left);
        }
        else
            x = GetNode(x->m_right);
    }

    return const_iterator(this, y);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::upper_bound(const key_type& k)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    // 查找第一个大于k的节点
    NodeBase* y = GetHeader(); // 结果初始化为header节点
    NodeBase* x = GetRoot(); // 从根节点开始搜索

    // 二分查找过程
    while (x != nullptr)
    {
        if (m_keyCompare(k, GetKey(x))) // k < x.key
        {
            // x的key大于k，更新结果但继续向左查找可能更小的符合条件节点
            y = x;
            x = GetNode(x->m_left);
        }
        else // k >= x.key
        {
            // x的key小于等于k，需要向右查找
            x = GetNode(x->m_right);
        }
    }

    return iterator(this, y);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::const_iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::upper_bound(const key_type& k) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());

    const NodeBase* y = GetHeader();
    const NodeBase* x = GetRoot();

    while (x != nullptr)
    {
        if (m_keyCompare(k, GetKey(x)))
        {
            y = x;
            x = GetNode(x->m_left);
        }
        else
            x = GetNode(x->m_right);
    }

    return const_iterator(this, y);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
std::pair<typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator, typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator> NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::equal_range(const key_type& k)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::make_pair(iterator(this, MAX_SIZE), iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
    // 返回一个范围，表示树中所有等于k的元素
    // 第一个迭代器指向第一个不小于k的元素，第二个迭代器指向第一个大于k的元素
    return std::pair<iterator, iterator>(lower_bound(k), upper_bound(k));
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
std::pair<typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::const_iterator, typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::const_iterator> NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::equal_range(const key_type& k) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::make_pair(const_iterator(this, MAX_SIZE), const_iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
    return std::pair<const_iterator, const_iterator>(lower_bound(k), upper_bound(k));
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::find(const key_type& k)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());

    // 先找到不小于k的第一个节点
    iterator j = lower_bound(k);
    // 然后检查该节点是否等于k (即不是k < j 且不是 j < k)
    return (j == end() || m_keyCompare(k, KeyOfValue()(*j))) ? end() : j;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::const_iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::find(const key_type& k) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());

    const_iterator j = lower_bound(k);
    return (j == end() || m_keyCompare(k, KeyOfValue()(*j))) ? end() : j;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::size_type NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::count(const key_type& k) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());

    // 获取键k的范围（从first到last的所有等于k的元素）
    std::pair<const_iterator, const_iterator> p = equal_range(k);
    // 计算范围内的元素数量
    return std::distance(p.first, p.second);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::swap(NFShmRBTree& x) noexcept
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(x.m_init == EN_NF_SHM_STL_INIT_OK, "x not init, TRACE_STACK:%s", TRACE_STACK());

    if (this != &x)
    {
        std::swap(m_size, x.m_size);
        std::swap(m_freeStart, x.m_freeStart);

        for (size_t i = 0; i <= MAX_SIZE; ++i)
        {
            AlignedStorage temp = m_mem[i];
            m_mem[i] = x.m_mem[i];
            x.m_mem[i] = temp;
        }
    }
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_unique(const_iterator position, const value_type& v)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), end(), "tree is full, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_pContainer == this, end(), "invalid iterator container, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_node != nullptr, end(), "invalid iterator node is null, TRACE_STACK:%s", TRACE_STACK());

    if (position.m_node == GetNode(GetHeader()->m_left))
    {
        // begin()
        if (size() > 0 &&
            m_keyCompare(KeyOfValue()(v), GetKey(position.m_node)))
            return InsertNode(position.m_node, position.m_node, v);
            // first argument just needs to be non-null
        else
            return insert_unique(v).first;
    }
    else if (position.m_node == GetHeader())
    {
        // end()
        CHECK_EXPR(GetNode(GetHeader()->m_right) != nullptr, end(), "GetNode(GetHeader()->m_right) == nullptr, TRACE_STACK:%s", TRACE_STACK());
        if (m_keyCompare(GetKey(GetNode(GetHeader()->m_right)), KeyOfValue()(v)))
            return InsertNode(nullptr, GetNode(GetHeader()->m_right), v);
        else
            return insert_unique(v).first;
    }
    else
    {
        const_iterator before = position;
        --before;
        CHECK_EXPR(before.m_node != nullptr, end(), "before.m_node == nullptr, TRACE_STACK:%s", TRACE_STACK());
        if (m_keyCompare(GetKey(before.m_node), KeyOfValue()(v)) &&
            m_keyCompare(KeyOfValue()(v), GetKey(position.m_node)))
        {
            if (GetNode(before.m_node->m_right) == nullptr)
                return InsertNode(nullptr, before.m_node, v);
            else
                return InsertNode(position.m_node, position.m_node, v);
            // first argument just needs to be non-null
        }
        else
            return insert_unique(v).first;
    }
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_equal(const_iterator position, const value_type& v)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(!full(), end(), "tree is full, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_pContainer == this, end(), "invalid iterator container, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_node != nullptr, end(), "invalid iterator node is null, TRACE_STACK:%s", TRACE_STACK());

    // 使用位置提示的插入，尝试优化插入路径
    if (position.m_node == GetNode(GetHeader()->m_left))
    {
        // position指向最小节点（begin()）
        if (size() > 0 &&
            !m_keyCompare(GetKey(position.m_node), KeyOfValue()(v)))
            // 如果新值小于等于最小节点，可以快速插入到其左侧
            return InsertNode(position.m_node, position.m_node, v);
        else
            // 否则退化为普通插入
            return insert_equal(v);
    }
    else if (position.m_node == GetHeader())
    {
        // position指向end()
        CHECK_EXPR(GetNode(GetHeader()->m_right) != nullptr, end(), "GetNode(GetHeader()->m_right) == nullptr, TRACE_STACK:%s", TRACE_STACK());
        if (!m_keyCompare(KeyOfValue()(v), GetKey(GetNode(GetHeader()->m_right))))
            // 如果新值大于等于最大节点，可以快速插入到其右侧
            return InsertNode(nullptr, GetNode(GetHeader()->m_right), v);
        else
            // 否则退化为普通插入
            return insert_equal(v);
    }
    else
    {
        // position指向内部节点
        const_iterator before = position;
        --before;
        CHECK_EXPR(before.m_node != nullptr, end(), "before.m_node == nullptr, TRACE_STACK:%s", TRACE_STACK());

        // 检查是否可以插入到before和position之间
        if (!m_keyCompare(KeyOfValue()(v), GetKey(before.m_node)) &&
            !m_keyCompare(GetKey(position.m_node), KeyOfValue()(v)))
        {
            // 在before和position之间插入
            if (GetNode(before.m_node->m_right) == nullptr)
                return InsertNode(nullptr, before.m_node, v);
            else
                return InsertNode(position.m_node, position.m_node, v);
        }
        else
        // 位置提示无效，退化为普通插入
            return insert_equal(v);
    }
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
template <class InputIterator>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_unique(InputIterator first, InputIterator last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    // 预先检查空间是否足够
    size_type insertCount = std::distance(first, last);
    size_type availableSpace = MAX_SIZE - m_size;

    if (insertCount > availableSpace)
    {
        LOG_WARN(0, -1, "insert_unique batch warning: trying to insert %lu elements but only %lu spaces available, will insert partially, TRACE_STACK:%s",
                 insertCount, availableSpace, TRACE_STACK());
    }

    for (; first != last && !full(); ++first)
        insert_unique(*first);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
template <class InputIterator>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_equal(InputIterator first, InputIterator last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    // 预先检查空间是否足够
    size_type insertCount = std::distance(first, last);
    size_type availableSpace = MAX_SIZE - m_size;

    if (insertCount > availableSpace)
    {
        LOG_WARN(0, -1, "insert_equal batch warning: trying to insert %lu elements but only %lu spaces available, will insert partially, TRACE_STACK:%s",
                 insertCount, availableSpace, TRACE_STACK());
    }

    for (; first != last && !full(); ++first)
        insert_equal(*first);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_unique(const value_type* first, const value_type* last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(first != nullptr && last != nullptr, "null pointer argument, TRACE_STACK:%s", TRACE_STACK());

    // 预先检查空间是否足够
    size_type insertCount = last - first;
    size_type availableSpace = MAX_SIZE - m_size;

    if (insertCount > availableSpace)
    {
        LOG_WARN(0, -1, "insert_unique batch warning: trying to insert %lu elements but only %lu spaces available, will insert partially, TRACE_STACK:%s",
                 insertCount, availableSpace, TRACE_STACK());
    }

    for (; first != last && !full(); ++first)
        insert_unique(*first);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_equal(const value_type* first, const value_type* last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(first != nullptr && last != nullptr, "null pointer argument, TRACE_STACK:%s", TRACE_STACK());

    // 预先检查空间是否足够
    size_type insertCount = last - first;
    size_type availableSpace = MAX_SIZE - m_size;

    if (insertCount > availableSpace)
    {
        LOG_WARN(0, -1, "insert_equal batch warning: trying to insert %lu elements but only %lu spaces available, will insert partially, TRACE_STACK:%s",
                 insertCount, availableSpace, TRACE_STACK());
    }

    for (; first != last && !full(); ++first)
        insert_equal(*first);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_unique(const_iterator first, const_iterator last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(first.m_pContainer == last.m_pContainer, "iterators from different containers, TRACE_STACK:%s", TRACE_STACK());

    // 预先检查空间是否足够
    size_type insertCount = std::distance(first, last);
    size_type availableSpace = MAX_SIZE - m_size;

    if (insertCount > availableSpace)
    {
        LOG_WARN(0, -1, "insert_unique batch warning: trying to insert %lu elements but only %lu spaces available, will insert partially, TRACE_STACK:%s",
                 insertCount, availableSpace, TRACE_STACK());
    }

    for (; first != last && !full(); ++first)
        insert_unique(*first);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::insert_equal(const_iterator first, const_iterator last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(first.m_pContainer == last.m_pContainer, "iterators from different containers, TRACE_STACK:%s", TRACE_STACK());

    // 预先检查空间是否足够
    size_type insertCount = std::distance(first, last);
    size_type availableSpace = MAX_SIZE - m_size;

    if (insertCount > availableSpace)
    {
        LOG_WARN(0, -1, "insert_equal batch warning: trying to insert %lu elements but only %lu spaces available, will insert partially, TRACE_STACK:%s",
                 insertCount, availableSpace, TRACE_STACK());
    }

    for (; first != last && !full(); ++first)
        insert_equal(*first);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::erase(iterator position)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position != end(), iterator(this, MAX_SIZE), "cannot erase end iterator, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_pContainer == this, iterator(this, MAX_SIZE), "iterators from different containers, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_node, iterator(this, MAX_SIZE), "invalid iterator node, TRACE_STACK:%s", TRACE_STACK());
    iterator result = position;
    ++result;
    EraseAux(position);
    return result;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::erase(const_iterator position)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position != end(), iterator(this, MAX_SIZE), "cannot erase end iterator, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_pContainer == this, iterator(this, MAX_SIZE), "iterators from different containers, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(position.m_node, iterator(this, MAX_SIZE), "invalid iterator node, TRACE_STACK:%s", TRACE_STACK());
    const_iterator result = position;
    ++result;
    EraseAux(position);
    return iterator(result.m_pContainer, result.m_node);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::size_type NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::erase(const key_type& k)
{
    return EraseAux(k);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::iterator NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::erase(const_iterator first, const_iterator last)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR(first.m_pContainer == this && last.m_pContainer == this, iterator(this, MAX_SIZE), "iterator not from this container, TRACE_STACK:%s", TRACE_STACK());
    EraseAux(first, last);
    return iterator(last.m_pContainer, last.m_node);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::erase(const key_type* first, const key_type* last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    EraseAux(first, last);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::__rb_verify() const
{
    return RbVerify();
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::EraseAux(const key_type* first, const key_type* last)
{
    while (first != last) EraseAux(*first++);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
int NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::BlackCount(const NFShmRBTreeNodeBase* node, const NFShmRBTreeNodeBase* root) const
{
    // 计算从节点node到根节点root的路径上的黑色节点数量
    // 红黑树的一个重要性质是从任一节点到其每个叶子节点的所有路径包含相同数量的黑色节点
    if (node == nullptr)
        return 0;

    // 当前节点如果是黑色，计数+1
    int bc = (node->m_color == RB_BLACK) ? 1 : 0;

    // 如果已经到达根节点，返回计数
    if (node == root)
        return bc;
    else
    {
        // 递归计算父节点到根节点的黑色节点数
        const NFShmRBTreeNodeBase* parent = (node->m_parent != INVALID_ID) ? GetNode(node->m_parent) : nullptr;
        return bc + BlackCount(parent, root);
    }
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::RbVerify() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
    // 验证红黑树是否满足所有红黑树性质

    // 1. 检查空树情况
    if (m_size == 0 || begin() == end())
        return m_size == 0 && begin() == end() &&
            GetNode(GetHeader()->m_left) == GetHeader() && GetNode(GetHeader()->m_right) == GetHeader();

    // 2. 计算从最左节点到根节点的黑色节点数，作为标准值
    int len = BlackCount(GetNode(GetHeader()->m_left), GetRoot());

    // 3. 遍历整个树进行验证
    for (const_iterator it = begin(); it != end(); ++it)
    {
        NodeBase* x = it.m_node;
        CHECK_EXPR(x, false, "null node in tree, TRACE_STACK:%s", TRACE_STACK());

        const NodeBase* l = GetNode(x->m_left); // 左子节点
        const NodeBase* r = GetNode(x->m_right); // 右子节点

        // 验证性质4：红色节点的子节点必须是黑色
        if (x->m_color == RB_RED)
        {
            if ((l && l->m_color == RB_RED) || (r && r->m_color == RB_RED))
                return false;
        }

        // 验证二叉搜索树性质：左子节点键值小于当前节点，右子节点键值大于当前节点
        if (l && m_keyCompare(GetKey(x), GetKey(l)))
            return false;
        if (r && m_keyCompare(GetKey(r), GetKey(x)))
            return false;

        // 验证性质5：从任一节点到其每个叶子节点的所有路径包含相同数量的黑色节点
        if (!l && !r && BlackCount(x, GetRoot()) != len)
            return false;
    }

    // 验证最小节点和最大节点指针的正确性
    if (GetNode(GetHeader()->m_left) != Minimum(GetRoot()))
        return false;
    if (GetNode(GetHeader()->m_right) != Maximum(GetRoot()))
        return false;

    return true; // 所有验证通过
}

template <class Key, class Value, class KeyOfValue, size_t MAX_SIZE, class Compare>
void swap(NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& x,
          NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& y) noexcept
{
    // 全局交换函数，调用成员swap方法
    x.swap(y);
}

template <class Key, class Value, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool operator==(const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& x,
                const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& y)
{
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}

template <class Key, class Value, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool operator<(const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& x,
               const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& y)
{
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

template <class Key, class Value, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool operator!=(const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& x,
                const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& y)
{
    return !(x == y);
}

template <class Key, class Value, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool operator>(const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& x,
               const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& y)
{
    return y < x;
}

template <class Key, class Value, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool operator<=(const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& x,
                const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& y)
{
    return !(y < x);
}

template <class Key, class Value, class KeyOfValue, size_t MAX_SIZE, class Compare>
bool operator>=(const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& x,
                const NFShmRBTree<Key, Value, KeyOfValue, MAX_SIZE, Compare>& y)
{
    return !(x < y);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
typename NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::key_compare NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::key_comp() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, Compare(), "not init, TRACE_STACK:%s", TRACE_STACK());
    return m_keyCompare;
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::~NFShmRBTree()
{
    clear();
}

// ==================== 打印函数实现 ====================

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::print_structure() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== NFShmRBTree Structure ===\n");
    printf("Size: %zu, Max Size: %zu, Free start: %d\n", m_size, MAX_SIZE, (int)m_freeStart);

    if (empty())
    {
        printf("(Empty tree)\n");
        printf("=============================\n\n");
        return;
    }

    const NodeBase* root = GetRoot();
    if (root)
    {
        // Display tree statistics
        int height = calculate_height(root);
        int redCount = 0, blackCount = 0;
        count_colors(root, redCount, blackCount);

        printf("Height: %d, Red nodes: %d, Black nodes: %d\n", height, redCount, blackCount);
        printf("Tree structure (Left=smaller, Right=larger):\n");
        printf("Format: Key(Color)[Index]\n");
        printf("\n");

        print_subtree(root, "", true, true);
    }
    else
    {
        printf("(Root is null)\n");
    }

    printf("=============================\n\n");
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::print_detailed() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== NFShmRBTree Detailed View ===\n");
    printf("Size: %zu, Max Size: %zu\n", m_size, MAX_SIZE);
    printf("Free Start: %d\n", (int)m_freeStart);
    printf("==================================\n");

    auto pNode = node();

    // Print all node states
    printf("Node Status Table:\n");
    printf("Index Valid Color Parent  Left  Right  SelfRef  Key Info\n");
    printf("----- ----- ----- ------  ----  -----  -------  --------\n");

    for (size_t i = 0; i <= MAX_SIZE; ++i)
    {
        const char* colorStr = (pNode[i].m_color == RB_RED) ? "RED" : "BLK";

        printf("%5zu %5s %5s %6d %5d %6d %7d  ",
               i,
               pNode[i].m_valid ? "Yes" : "No",
               colorStr,
               (int)pNode[i].m_parent,
               (int)pNode[i].m_left,
               (int)pNode[i].m_right,
               (int)pNode[i].m_self);

        if (i < MAX_SIZE && pNode[i].m_valid)
        {
            try
            {
                auto key = KeyOfValue()(pNode[i].m_data);
                printf("Key: ");

                // Print based on key type
                if (std::is_arithmetic<Key>::value)
                {
                    if (std::is_integral<Key>::value)
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
            }
            catch (...)
            {
                printf("(Cannot get key info)");
            }
        }
        else if (i == MAX_SIZE)
        {
            printf("HEADER NODE");
        }
        else
        {
            printf("(Invalid/Free)");
        }

        printf("\n");
    }

    // Print free list
    printf("\nFree list: ");
    if (m_freeStart < 0 || m_freeStart >= MAX_SIZE)
    {
        printf("Empty\n");
    }
    else
    {
        ptrdiff_t freeIdx = m_freeStart;
        size_t freeCount = 0;
        size_t maxFreeCount = MAX_SIZE; // Prevent infinite loop

        while (freeIdx >= 0 && freeIdx < MAX_SIZE && freeCount < maxFreeCount)
        {
            printf("[%d]", (int)freeIdx);
            freeCount++;

            ptrdiff_t nextIdx = pNode[freeIdx].m_right;
            if (nextIdx >= 0 && nextIdx < MAX_SIZE)
            {
                freeIdx = nextIdx;
                if (freeIdx >= 0)
                {
                    printf(" -> ");
                }
            }
            else
            {
                break;
            }
        }

        if (freeCount >= maxFreeCount)
        {
            printf(" ... (Loop detected!)");
        }

        printf(" (Free nodes: %zu)\n", freeCount);

        size_t expectedFreeCount = MAX_SIZE - m_size;
        if (freeCount != expectedFreeCount)
        {
            printf("  Warning: Free nodes count %zu doesn't match expected %zu!\n", freeCount, expectedFreeCount);
        }
    }

    // Tree statistics
    printf("\nTree Statistics:\n");
    if (!empty())
    {
        int height = calculate_height(GetRoot());
        int redCount = 0, blackCount = 0;
        count_colors(GetRoot(), redCount, blackCount);

        printf("  Height: %d\n", height);
        printf("  Red nodes: %d\n", redCount);
        printf("  Black nodes: %d\n", blackCount);

        // Verify red-black tree properties
        bool isValid = RbVerify();
        printf("  RB-Tree valid: %s\n", isValid ? "Yes" : "No");
    }

    printf("==================================\n\n");
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::print_simple() const
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());

    printf("\n=== RB-Tree Simple View ===\n");
    printf("Size: %zu/%zu, Free head: %d\n", m_size, MAX_SIZE, (int)m_freeStart);

    if (empty())
    {
        printf("(Empty tree)\n");
    }
    else
    {
        int height = calculate_height(GetRoot());
        int redCount = 0, blackCount = 0;
        count_colors(GetRoot(), redCount, blackCount);

        printf("Height: %d, Nodes: R%d/B%d\n", height, redCount, blackCount);

        // Print in-order traversal (simplified)
        printf("In-order: ");
        const_iterator it = begin();
        const_iterator endIt = end();
        int count = 0;
        while (it != endIt && count < 10) // Show at most 10 elements
        {
            try
            {
                auto key = KeyOfValue()(*it);
                if (std::is_arithmetic<Key>::value)
                {
                    if (std::is_integral<Key>::value)
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
            }
            catch (...)
            {
                printf("?");
            }

            ++it;
            count++;
            if (it != endIt && count < 10)
            {
                printf(", ");
            }
        }

        if (count >= 10 && it != endIt)
        {
            printf("...");
        }
        printf("\n");

        bool isValid = RbVerify();
        printf("Valid: %s\n", isValid ? "Yes" : "No");
    }

    printf("============================\n\n");
}

// ==================== 打印辅助函数实现 ====================

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::print_subtree(const NodeBase* node, const std::string& prefix, bool isLast, bool isRoot) const
{
    if (!node) return;

    // Print current node with proper tree graphics
    printf("%s", prefix.c_str());

    if (!isRoot)
    {
        printf("%s", isLast ? "└── " : "├── ");
    }

    printf("%s\n", get_node_info(node).c_str());

    // Get children
    const NodeBase* leftChild = GetNode(node->m_left);
    const NodeBase* rightChild = GetNode(node->m_right);

    // Only print children if they exist
    if (leftChild || rightChild)
    {
        // Prepare prefix for children
        std::string childPrefix = prefix;
        if (!isRoot)
        {
            childPrefix += isLast ? "    " : "│   ";
        }

        // Print left child first (smaller values), then right child (larger values)
        // This maintains the BST property in visual representation
        if (leftChild && rightChild)
        {
            // Both children exist: left is not last, right is last
            print_subtree(leftChild, childPrefix, false, false);
            print_subtree(rightChild, childPrefix, true, false);
        }
        else if (leftChild)
        {
            // Only left child exists: it's the last one
            print_subtree(leftChild, childPrefix, true, false);
        }
        else if (rightChild)
        {
            // Only right child exists: it's the last one
            print_subtree(rightChild, childPrefix, true, false);
        }
    }
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
std::string NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::get_node_info(const NodeBase* node) const
{
    if (!node) return "null";

    char buffer[256];
    const char* colorStr = (node->m_color == RB_RED) ? "R" : "B";

    if (node == GetHeader())
    {
        snprintf(buffer, sizeof(buffer), "HEADER(%s)", colorStr);
    }
    else
    {
        try
        {
            auto key = GetKey(node);
            if (std::is_arithmetic<Key>::value)
            {
                if (std::is_integral<Key>::value)
                {
                    snprintf(buffer, sizeof(buffer), "%lld(%s)[%d]",
                             (long long)key, colorStr, (int)node->m_self);
                }
                else
                {
                    snprintf(buffer, sizeof(buffer), "%.2f(%s)[%d]",
                             (double)key, colorStr, (int)node->m_self);
                }
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "?(%s)[%d]", colorStr, (int)node->m_self);
            }
        }
        catch (...)
        {
            snprintf(buffer, sizeof(buffer), "?(%s)[%d]", colorStr, (int)node->m_self);
        }
    }

    return std::string(buffer);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
int NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::calculate_height(const NodeBase* node) const
{
    if (!node) return 0;

    const NodeBase* left = GetNode(node->m_left);
    const NodeBase* right = GetNode(node->m_right);

    int leftHeight = calculate_height(left);
    int rightHeight = calculate_height(right);

    return 1 + std::max(leftHeight, rightHeight);
}

template <class Key, class KeyValue, class KeyOfValue, size_t MAX_SIZE, class Compare>
void NFShmRBTree<Key, KeyValue, KeyOfValue, MAX_SIZE, Compare>::count_colors(const NodeBase* node, int& redCount, int& blackCount) const
{
    if (!node) return;

    if (node->m_color == RB_RED)
    {
        redCount++;
    }
    else
    {
        blackCount++;
    }

    const NodeBase* left = GetNode(node->m_left);
    const NodeBase* right = GetNode(node->m_right);

    count_colors(left, redCount, blackCount);
    count_colors(right, redCount, blackCount);
}
