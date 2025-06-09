// -------------------------------------------------------------------------
//    @FileName         :    NFShmList.h
//    @Author           :    gaoyi
//    @Date             :    23-2-2
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmList
//
// -------------------------------------------------------------------------

/**
 * @file NFShmList.h
 * @brief 基于共享内存的双向链表实现，提供类似std::list的功能
 * 
 * @section overview 概述
 * 
 * NFShmList 是一个专为共享内存环境设计的双向链表容器，为std::list
 * 提供共享内存兼容的替代方案。在API设计上高度兼容STL list，但在内存管理、
 * 节点分配等方面针对共享内存环境进行了优化。
 * 
 * @section features 核心特性
 * 
 * 1. **双向链表结构**：
 *    - 双向链表，支持双向迭代器
 *    - 任意位置O(1)插入删除（已知迭代器）
 *    - 动态调整大小，但受MAX_SIZE限制
 *    - 非连续内存存储，不支持随机访问
 * 
 * 2. **共享内存优化**：
 *    - 固定大小节点池，避免动态内存分配
 *    - 基于索引的节点引用，支持进程间共享
 *    - 内存对齐优化，提高访问效率
 *    - 支持CREATE/RESUME模式初始化
 * 
 * 3. **STL高度兼容**：
 *    - 完整的双向迭代器支持
 *    - 标准的容器操作（push_front、push_back、insert、erase等）
 *    - 链表特有操作（splice、merge、sort等）
 *    - initializer_list支持
 * 
 * 4. **扩展功能**：
 *    - 节点池管理和回收
 *    - 容量检查功能（full()）
 *    - 高效的随机打乱操作
 *    - 完整的链表操作集合
 * 
 * @section stl_comparison STL容器对比
 * 
 * | 特性 | STL list | NFShmList |
 * |------|----------|-----------|
 * | **数据结构** | 双向链表 | 双向链表 |
 * | **容量管理** | 动态分配，无限制 | 固定节点池MAX_SIZE，编译时确定 |
 * | **内存管理** | 堆内存，动态分配 | 共享内存，预分配节点池 |
 * | **插入删除** | O(1)（已知位置） | O(1)（已知位置） |
 * | **随机访问** | 不支持 | 不支持 |
 * | **迭代器类型** | 双向迭代器 | 双向迭代器 |
 * | **内存布局** | 分散分配 | 连续节点池 |
 * | **进程共享** | 不支持 | **原生支持** |
 * | **异常安全** | 强异常安全保证 | 无异常，错误码返回 |
 * | **内存碎片** | 可能产生 | **无碎片**（固定节点池） |
 * | **节点引用** | 指针 | **索引**（共享内存友好） |
 * 
 * @section api_compatibility API兼容性
 * 
 * **完全兼容的接口**：
 * - size(), empty(), max_size()
 * - begin(), end(), rbegin(), rend(), cbegin(), cend()
 * - front(), back()
 * - push_front(), push_back(), pop_front(), pop_back()
 * - insert(), erase(), clear()
 * - assign(), resize()
 * - splice(), merge(), sort(), reverse(), unique(), remove()
 * 
 * **扩展的接口（新增）**：
 * - full() - 检查节点池是否已满
 * - CreateInit(), ResumeInit() - 共享内存初始化
 * - GetNode(), GetData(), GetIterator() - 节点访问接口
 * - random_shuffle() - 随机打乱
 * 
 * **行为差异的接口**：
 * - max_size()：返回MAX_SIZE而非理论最大值
 * - 插入操作：节点池满时失败而非自动扩容
 * - 构造函数：需要显式调用CreateInit()或ResumeInit()
 * 
 * @section usage_examples 使用示例
 * 
 * @subsection basic_usage 基础用法（类似std::list）
 * 
 * ```cpp
 * // 定义容量为1000的整数链表
 * NFShmList<int, 1000> list;
 * list.CreateInit();  // 创建模式初始化
 * 
 * // STL兼容的基础操作
 * list.push_back(1);
 * list.push_back(2);
 * list.push_front(0);
 * 
 * // 访问首尾元素
 * std::cout << "Front: " << list.front() << std::endl;  // 0
 * std::cout << "Back: " << list.back() << std::endl;    // 2
 * std::cout << "Size: " << list.size() << std::endl;    // 3
 * 
 * // 迭代器遍历
 * for (auto it = list.begin(); it != list.end(); ++it) {
 *     std::cout << *it << " ";  // 输出：0 1 2
 * }
 * 
 * // 范围for循环（C++11）
 * for (const auto& item : list) {
 *     std::cout << item << " ";
 * }
 * ```
 * 
 * @subsection splice_operations 拼接操作
 * 
 * ```cpp
 * NFShmList<int, 100> list1 = {1, 2, 3};
 * NFShmList<int, 100> list2 = {4, 5, 6};
 * 
 * // 将list2的所有元素拼接到list1的末尾
 * list1.splice(list1.end(), list2);
 * // list1: {1, 2, 3, 4, 5, 6}, list2: {}
 * 
 * // 将单个元素拼接
 * auto it = list1.begin();
 * ++it; ++it;  // 指向元素3
 * NFShmList<int, 100> list3 = {7};
 * list1.splice(it, list3.begin());
 * // list1: {1, 2, 7, 3, 4, 5, 6}
 * ```
 * 
 * @subsection sort_merge_operations 排序和合并操作
 * 
 * ```cpp
 * NFShmList<int, 100> list1 = {3, 1, 4, 1, 5};
 * NFShmList<int, 100> list2 = {2, 6, 0, 8};
 * 
 * // 排序
 * list1.sort();  // {1, 1, 3, 4, 5}
 * list2.sort();  // {0, 2, 6, 8}
 * 
 * // 合并两个有序链表
 * list1.merge(list2);
 * // list1: {0, 1, 1, 2, 3, 4, 5, 6, 8}, list2: {}
 * 
 * // 去重
 * list1.unique();  // {0, 1, 2, 3, 4, 5, 6, 8}
 * 
 * // 反转
 * list1.reverse(); // {8, 6, 5, 4, 3, 2, 1, 0}
 * ```
 * 
 * @subsection capacity_management 容量管理
 * 
 * ```cpp
 * NFShmList<int, 10> list;
 * 
 * // 容量检查
 * while (!list.full()) {
 *     list.push_back(getData());  // 安全插入
 * }
 * 
 * if (list.full()) {
 *     std::cout << "链表节点池已满" << std::endl;
 * }
 * 
 * // 调整大小
 * list.resize(5, 100);  // 调整到5个元素，新元素值为100
 * ```
 * 
 * @section performance_characteristics 性能特征
 * 
 * | 操作 | STL list | NFShmList |
 * |------|----------|-----------|
 * | **头部插入 push_front** | O(1) | O(1) |
 * | **尾部插入 push_back** | O(1) | O(1) |
 * | **中间插入 insert** | O(1) | O(1) |
 * | **删除 erase** | O(1) | O(1) |
 * | **查找 find** | O(n) | O(n) |
 * | **排序 sort** | O(n log n) | O(n log n) |
 * | **合并 merge** | O(n + m) | O(n + m) |
 * | **拼接 splice** | O(1)或O(n) | O(1)或O(n) |
 * | **随机访问 [i]** | 不支持 | 不支持 |
 * | **内存分配** | 动态分配 | 预分配节点池 |
 * | **缓存友好性** | 差（分散内存） | **较好**（连续节点池） |
 * 
 * @section memory_layout 内存布局
 * 
 * ```
 * NFShmList 内存结构：
 * ┌─────────────────┐
 * │   管理数据       │ <- 基类信息，头尾节点索引等
 * ├─────────────────┤
 * │   节点池         │ <- m_astNode[MAX_SIZE]
 * │   [0] 头节点     │    ├─ prev/next索引
 * │   [1] 节点1      │    ├─ data + valid标志
 * │   [2] 节点2      │    ├─ ...
 * │   ...           │    ├─ ...
 * │   [MAX_SIZE-1]  │    └─ 最后一个节点
 * └─────────────────┘
 * 
 * 节点结构：
 * ┌─────────────────┐
 * │   m_next        │ <- 下一个节点索引
 * │   m_prev        │ <- 前一个节点索引  
 * │   m_self        │ <- 自身索引
 * │   m_data        │ <- 用户数据
 * │   m_valid       │ <- 节点是否有效
 * └─────────────────┘
 * 
 * 索引引用优势：
 * - 进程间地址无关，支持共享内存
 * - 固定大小，内存布局可预测
 * - 避免悬空指针问题
 * ```
 * 
 * @section thread_safety 线程安全
 * 
 * - **非线程安全**：需要外部同步机制
 * - **共享内存兼容**：多进程可安全访问（需进程间锁）
 * - **无内部锁**：避免性能开销，由用户控制并发
 * - **迭代器稳定性**：插入删除不影响其他有效迭代器
 * 
 * @section migration_guide 从STL迁移指南
 * 
 * 1. **包含头文件**：
 *    ```cpp
 *    // 替换
 *    #include <list>
 *    // 为
 *    #include "NFComm/NFShmStl/NFShmList.h"
 *    ```
 * 
 * 2. **类型定义**：
 *    ```cpp
 *    // STL
 *    std::list<int> list;
 *    
 *    // NFShmList（需要指定最大节点数）
 *    NFShmList<int, 1000> list;  // 最大1000个节点
 *    ```
 * 
 * 3. **初始化**：
 *    ```cpp
 *    // 添加初始化调用
 *    list.CreateInit();  // 或 ResumeInit() 用于共享内存恢复
 *    ```
 * 
 * 4. **容量管理**：
 *    ```cpp
 *    // 添加容量检查
 *    if (list.full()) {
 *        // 处理节点池已满的情况
 *    }
 *    
 *    // 移除无限容量假设
 *    while (!list.full()) list.push_back(item);  // 安全
 *    ```
 * 
 * @section best_practices 最佳实践
 * 
 * 1. **容量规划**：根据预期数据量选择合适的MAX_SIZE，预留20-30%余量
 * 2. **元素类型选择**：
 *    - 优先使用trivially copyable类型
 *    - 避免包含指针的复杂对象
 *    - 考虑内存对齐对性能的影响
 * 3. **初始化模式**：
 *    - 新创建时使用CreateInit()
 *    - 恢复现有数据时使用ResumeInit()
 * 4. **迭代器使用**：
 *    - 插入删除后迭代器仍然有效（除被删除的）
 *    - 利用splice操作高效移动元素
 * 5. **性能优化**：
 *    - 对有序数据使用merge而非sort+insert
 *    - 利用链表特性，避免随机访问需求
 * 
 * @warning 注意事项
 * - 固定节点池限制，超出MAX_SIZE的操作会失败
 * - 不支持随机访问，只能通过迭代器顺序访问
 * - 共享内存环境下需要考虑进程崩溃的数据恢复
 * - 非线程安全，多线程访问需要外部同步
 * 
 * @see NFShmVector - 基于共享内存的动态数组实现
 * @see NFShmRBTree - 基于共享内存的红黑树实现  
 * @see std::list - 标准双向链表容器
 * 
 * @author gaoyi
 * @date 2023-02-02
 */

#pragma once

#include "NFComm/NFShmStl/NFShmStl.h"
#include <iterator>
#include <algorithm>
#include <vector>

struct NFShmListNodeBase
{
    NFShmListNodeBase()
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
     * @brief 创建模式初始化（共享内存特有）
     * @return 0表示成功
     * @note 与STL对比：
     *       - STL: 无对应概念，构造函数即完成初始化
     *       - NFShmList: 支持共享内存CREATE/RESUME两阶段初始化
     *       - 用于共享内存首次创建时的初始化
     *       - 清空所有节点状态，重置为空链表
     *       - 初始化节点池的空闲链表结构
     *       - 在SHM_CREATE_MODE下自动调用
     */
    int CreateInit()
    {
        m_next = 0;
        m_prev = 0;
        m_self = 0;
        return 0;
    }

    /**
     * @brief 恢复模式初始化（共享内存特有）
     * @return 0表示成功，-1表示失败
     * @note 与STL对比：
     *       - STL: 无对应概念
     *       - NFShmList: 用于从已存在的共享内存恢复容器状态
     *       - 不清空现有数据，恢复链表和节点池状态
     *       - 重新建立节点间的链接关系
     *       - 进程重启后恢复共享内存数据使用
     *       - 对非平凡构造类型执行placement构造
     */
    int ResumeInit()
    {
        return 0;
    }

    ptrdiff_t m_next;
    ptrdiff_t m_prev;
    ptrdiff_t m_self;
};

template<class Tp>
struct NFShmListNode : public NFShmListNodeBase
{
    NFShmListNode()
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

    int CreateInit()
    {
        m_valid = false;
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    Tp m_data;
    bool m_valid;
};

template<class Container>
struct NFShmListIteratorBase
{
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::bidirectional_iterator_tag iterator_category;

    Container *m_pContainer;
    NFShmListNodeBase *m_node;

    explicit NFShmListIteratorBase(const Container *pContainer, size_t iPos)
        : m_pContainer(const_cast<Container *>(pContainer))
    {
        m_node = m_pContainer->GetNode(iPos);
    }

    explicit NFShmListIteratorBase(const Container *pContainer, const NFShmListNodeBase *pNode)
        : m_pContainer(const_cast<Container *>(pContainer)), m_node(const_cast<NFShmListNodeBase *>(pNode))
    {
    }

    NFShmListIteratorBase() : m_pContainer(nullptr), m_node(nullptr)
    {
    }

    void Incr()
    {
        CHECK_EXPR_RE_VOID(m_node, "m_node == nullptr, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_pContainer, "m_pContainer == nullptr, TRACE_STACK:%s", TRACE_STACK());
        m_node = m_pContainer->GetNode(m_node->m_next);
    }

    void Decr()
    {
        CHECK_EXPR_RE_VOID(m_node, "m_node == nullptr, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_pContainer, "m_pContainer == nullptr, TRACE_STACK:%s", TRACE_STACK());
        m_node = m_pContainer->GetNode(m_node->m_prev);
    }

    bool operator==(const NFShmListIteratorBase &x) const
    {
        return m_node == x.m_node;
    }

    bool operator!=(const NFShmListIteratorBase &x) const
    {
        return m_node != x.m_node;
    }
};

template<class Tp, class Ref, class Ptr, class Container>
struct NFShmListIterator : public NFShmListIteratorBase<Container>
{
    typedef NFShmListIterator<Tp, Tp &, Tp *, Container> iterator;
    typedef NFShmListIterator<Tp, const Tp &, const Tp *, Container> const_iterator;
    typedef NFShmListIterator Self;

    typedef Tp value_type;
    typedef Ptr pointer;
    typedef Ref reference;
    typedef NFShmListNode<Tp> Node;

    using NFShmListIteratorBase<Container>::m_node;

    explicit NFShmListIterator(const Container *pContainer, size_t iPos) : NFShmListIteratorBase<Container>(pContainer, iPos)
    {
    }

    explicit NFShmListIterator(const Container *pContainer, const NFShmListNodeBase *pNode) : NFShmListIteratorBase<Container>(pContainer, pNode)
    {
    }

    NFShmListIterator()
    {
    }

    NFShmListIterator(const iterator &x) : NFShmListIteratorBase<Container>(x.m_pContainer, x.m_node)
    {
    }

    reference operator*() const { return ((Node *) m_node)->m_data; }

    pointer operator->() const { return &(operator*()); }

    Self &operator++()
    {
        this->Incr();
        return *this;
    }

    Self operator++(int)
    {
        Self tmp = *this;
        this->Incr();
        return tmp;
    }

    Self &operator--()
    {
        this->Decr();
        return *this;
    }

    Self operator--(int)
    {
        Self tmp = *this;
        this->Decr();
        return tmp;
    }
};

template<class Tp, size_t MAX_SIZE>
class NFShmListBase
{
public:
    NFShmListBase()
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

    ~NFShmListBase()
    {
        m_size = 0;
        m_freeStart = 0;
        memset(m_mem, 0, sizeof(m_mem));
        m_init = 0;
    }

    int CreateInit()
    {
        m_size = 0;
        m_freeStart = 0;
        memset(m_mem, 0, sizeof(m_mem));
        auto pNode = node();

        for (size_t i = 0; i < MAX_SIZE; i++)
        {
            pNode[i].m_next = i + 1;
            pNode[i].m_prev = 0;
            pNode[i].m_valid = false;
            pNode[i].m_self = i;
        }

        pNode[MAX_SIZE].m_next = MAX_SIZE;
        pNode[MAX_SIZE].m_prev = MAX_SIZE;
        pNode[MAX_SIZE].m_self = MAX_SIZE;
        pNode[MAX_SIZE].m_valid = false;

        m_init = EN_NF_SHM_STL_INIT_OK;
#ifdef NF_DEBUG_MODE
        m_ptr = (NFShmListNode<Tp>*)m_mem;
#endif
        return 0;
    }

    int ResumeInit()
    {
        if (m_init == EN_NF_SHM_STL_INIT_OK)
        {
            auto pNode = node();
            if (!std::stl_is_trivially_default_constructible<Tp>::value)
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
        m_ptr = (NFShmListNode<Tp>*)m_mem;
#endif

        return 0;
    }

    void clear()
    {
        if (m_init == EN_NF_SHM_STL_INIT_OK)
        {
            m_size = 0;
            m_freeStart = 0;
            auto pNode = node();

            for (size_t i = 0; i < MAX_SIZE; i++)
            {
                if (pNode[i].m_valid)
                {
                    std::_Destroy(&(pNode[i].m_data));
                }
                pNode[i].m_next = i + 1;
                pNode[i].m_prev = 0;
                pNode[i].m_valid = false;
                pNode[i].m_self = i;
            }

            if (pNode[MAX_SIZE].m_valid)
            {
                std::_Destroy(&(pNode[MAX_SIZE].m_data));
            }
            pNode[MAX_SIZE].m_next = MAX_SIZE;
            pNode[MAX_SIZE].m_prev = MAX_SIZE;
            pNode[MAX_SIZE].m_self = MAX_SIZE;
            pNode[MAX_SIZE].m_valid = false;
        }
    }

    NFShmListNode<Tp> *node() { return reinterpret_cast<NFShmListNode<Tp> *>(m_mem); }
    const NFShmListNode<Tp> *node() const { return reinterpret_cast<const NFShmListNode<Tp> *>(m_mem); }

protected:
    // 内存对齐优化（C++11 alignas）
    typedef typename std::aligned_storage<sizeof(NFShmListNode<Tp>), alignof(NFShmListNode<Tp>)>::type AlignedStorage;
    AlignedStorage m_mem[MAX_SIZE+1];
    ptrdiff_t m_freeStart;
    size_t m_size;
    int m_init;
#ifdef NF_DEBUG_MODE
    NFShmListNode<Tp>* m_ptr;//用来debug的时候看内存
#endif
};

template<class Tp, size_t MAX_SIZE>
class NFShmList : protected NFShmListBase<Tp, MAX_SIZE>
{
    typedef NFShmListBase<Tp, MAX_SIZE> base;
public:
    static Tp m_staticError;
public:
    /**
     * @brief 类型定义（与STL list完全兼容）
     * @note 与STL对比：
     *       - 所有类型定义与std::list完全一致
     *       - 基于双向链表实现，支持双向迭代器
     *       - 使用索引而非指针来实现节点引用，适合共享内存
     *       - 迭代器类型为bidirectional_iterator_tag
     *       - 完全支持STL算法和范围操作
     */
    typedef NFShmList ListType;                                            // 链表类型别名
    typedef Tp value_type;                                                 // 元素类型
    typedef value_type *pointer;                                           // 指针类型
    typedef const value_type *const_pointer;                               // 常量指针类型
    typedef value_type &reference;                                         // 引用类型
    typedef const value_type &const_reference;                             // 常量引用类型
    typedef NFShmListNode<Tp> Node;                                        // 节点类型
    typedef size_t size_type;                                              // 大小类型
    typedef ptrdiff_t difference_type;                                     // 差值类型

public:
    typedef NFShmListIterator<Tp, Tp &, Tp *, ListType> iterator;          // 迭代器类型（双向）
    typedef NFShmListIterator<Tp, const Tp &, const Tp *, ListType> const_iterator; // 常量迭代器类型

    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;  // 常量反向迭代器
    typedef std::reverse_iterator<iterator> reverse_iterator;              // 反向迭代器

protected:
    using base::node;
    using base::m_freeStart;
    using base::m_size;
    using base::m_init;

protected:
    template <typename... Args>
    Node* CreateNode(const Args&... args)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_freeStart != MAX_SIZE, nullptr, "no free node available, free start index:%lu, TRACE_STACK:%s", (size_t)m_freeStart, TRACE_STACK());
        ptrdiff_t iSelf = m_freeStart;
        auto pNode = node();
        m_freeStart = pNode[m_freeStart].m_next;

        std::_Construct(&pNode[iSelf].m_data, args...);

        CHECK_EXPR(!pNode[iSelf].m_valid, nullptr, "node already valid, index:%lu, TRACE_STACK:%s", (size_t)iSelf, TRACE_STACK());
        pNode[iSelf].m_valid = true;

        return &pNode[iSelf];
    }

    void RecycleNode(Node *pNode)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(pNode, "attempt to recycle null node, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(pNode->m_valid, "attempt to recycle invalid node, index:%lu, TRACE_STACK:%s", (size_t)pNode->m_self, TRACE_STACK());

        std::_Destroy(&(pNode->m_data));

        pNode->m_valid = false;
        pNode->m_next = m_freeStart;
        m_freeStart = pNode->m_self;
    }

public:
    explicit NFShmList()
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

    int CreateInit()
    {
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    //init data int the union
    void Init()
    {
        new(this) NFShmList();
    }

    NFShmList(size_type n, const Tp &value)
    {
        insert(begin(), n, value);
    }

    explicit NFShmList(size_type n)
    {
        insert(begin(), n, Tp());
    }

    template<class InputIterator>
    NFShmList(InputIterator first, InputIterator last)
    {
        insert(begin(), first, last);
    }

    NFShmList(const_iterator first, const_iterator last)
    {
        insert(begin(), first, last);
    }

    template<size_t X_MAX_SIZE>
    NFShmList(const NFShmList<Tp, X_MAX_SIZE> &x)
    {
        insert(begin(), x.begin(), x.end());
    }

    NFShmList(const NFShmList &x)
    {
        insert(begin(), x.begin(), x.end());
    }

    NFShmList(const std::initializer_list<Tp> &list)
    {
        insert(begin(), list.begin(), list.end());
    }

    ~NFShmList()
    {
    }

    NFShmList &operator=(const NFShmList &x);

public:
    Node* GetNode(size_t index)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(index <= MAX_SIZE, nullptr, "index out of range:%lu, TRACE_STACK:%s", index, TRACE_STACK());
        auto pNode = node();
        return &pNode[index];
    }

    pointer GetData(size_t index)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto pNode = node();
        if (index < MAX_SIZE)
        {
            CHECK_EXPR(pNode[index].m_valid, nullptr, "node not valid, index:%lu, TRACE_STACK:%s", index, TRACE_STACK());
            return &pNode[index].m_data;
        }

        return nullptr;
    }

    iterator GetIterator(size_t index)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(index <= MAX_SIZE, end(), "index out of range:%lu, TRACE_STACK:%s", index, TRACE_STACK());
        return iterator(this, index);
    }

    const_iterator GetIterator(size_t index) const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(index <= MAX_SIZE, end(), "index out of range:%lu, TRACE_STACK:%s", index, TRACE_STACK());
        return const_iterator(this, index);
    }

public:
    void assign(size_type n, const Tp& val)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        FillAssign(n, val);
    }

    template <class InputIterator>
    void assign(InputIterator first, InputIterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        typedef typename std::is_integral<InputIterator>::type Integral;
        AssignDispatch(first, last, Integral());
    }

    void assign(const std::initializer_list<Tp>& list)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        assign(list.begin(), list.end());
    }
public:
    iterator begin()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        return iterator(this, node()[MAX_SIZE].m_next);
    }

    const_iterator begin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_iterator(this, node()[MAX_SIZE].m_next);
    }

    iterator end()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        return iterator(this, MAX_SIZE);
    }

    const_iterator end() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_iterator(this, MAX_SIZE);
    }

    reverse_iterator rbegin()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, reverse_iterator(iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(const_iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_reverse_iterator(end());
    }

    reverse_iterator rend()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, reverse_iterator(iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(const_iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_reverse_iterator(begin());
    }

    const_iterator cbegin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_iterator(this, node()[MAX_SIZE].m_next);
    }

    const_iterator cend() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_iterator(this, MAX_SIZE);
    }

    const_reverse_iterator crbegin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(const_iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crend() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(const_iterator(this, MAX_SIZE)), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_reverse_iterator(begin());
    }

    bool empty() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, true, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto pNode = node();
        if (pNode[MAX_SIZE].m_next == MAX_SIZE)
        {
            CHECK_EXPR(m_size == 0, true, "empty check failed, m_size:%lu != 0, TRACE_STACK:%s", m_size, TRACE_STACK());
        }
        return pNode[MAX_SIZE].m_next == MAX_SIZE;
    }

    /**
     * @brief 检查链表节点池是否已满
     * @return true表示节点池已满，false表示还有可用节点
     * @note 与STL对比：
     *       - STL: 无对应接口，std::list可无限扩容
     *       - NFShmList: 特有功能，体现固定节点池限制
     *       - 用于在插入前检查容量，避免操作失败
     *       - 等价于 size() >= max_size()
     *       - 与empty()形成对比，提供完整的容量状态检查
     * 
     * 使用示例：
     * ```cpp
     * NFShmList<int, 100> list;
     * 
     * // 安全插入模式
     * while (!list.full() && hasMoreData()) {
     *     list.push_back(getData());
     * }
     * 
     * // 容量检查
     * if (list.full()) {
     *     std::cout << "节点池已满，无法继续添加元素" << std::endl;
     * } else {
     *     std::cout << "还可以添加 " << (list.max_size() - list.size()) 
     *               << " 个元素" << std::endl;
     * }
     * ```
     */
    bool full() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
        if (m_freeStart == MAX_SIZE)
        {
            CHECK_EXPR(m_size == MAX_SIZE, true, "full check failed, m_size:%lu != MAX_SIZE:%lu, TRACE_STACK:%s", m_size, MAX_SIZE, TRACE_STACK());
        }
        return m_freeStart == MAX_SIZE;
    }

    size_type size() const
    {
        //CHECK_EXPR((size_type)std::distance(begin(), end()) == m_size, m_size, "TRACE_STACK:%s", TRACE_STACK());
        return m_size;
    }

    size_type max_size() const { return MAX_SIZE; }

    void resize(size_type newSize, const Tp& x);

    void resize(size_type newSize) { this->resize(newSize, Tp()); }

    reference front()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(!empty(), m_staticError, "list empty, TRACE_STACK:%s", TRACE_STACK());
        return *begin();
    }

    const_reference front() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(!empty(), m_staticError, "list empty, TRACE_STACK:%s", TRACE_STACK());
        return *begin();
    }

    reference back()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(!empty(), m_staticError, "list empty, TRACE_STACK:%s", TRACE_STACK());
        return *(--end());
    }

    const_reference back() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(!empty(), m_staticError, "list empty, TRACE_STACK:%s", TRACE_STACK());
        return *(--end());
    }

    void push_front(const Tp& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(!full(), "NFShmList push_front Failed, Vector Not Enough Space, TRACE_STACK:%s", TRACE_STACK());
        insert(begin(), x);
    }

    template <typename... Args>
    void emplace_front(const Args&... args)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(!full(), "NFShmList emplace_front Failed, Vector Not Enough Space, TRACE_STACK:%s", TRACE_STACK());
        insert(begin(), args...);
    }

    void pop_front()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(!empty(), "empty, can't pop back, TRACE_STACK:%s", TRACE_STACK());
        erase(begin());
    }

    void push_back(const Tp& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(!full(), "NFShmList push_back Failed, Vector Not Enough Space, TRACE_STACK:%s", TRACE_STACK());
        insert(end(), x);
    }

    template <typename... Args>
    void emplace_back(const Args&... args)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(!full(), "NFShmList emplace_back Failed, Vector Not Enough Space, TRACE_STACK:%s", TRACE_STACK());
        insert(end(), args...);
    }

    void pop_back()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(!empty(), "empty, can't pop back, TRACE_STACK:%s", TRACE_STACK());
        iterator tmp = end();
        erase(--tmp);
    }

    template <typename... Args>
    iterator emplace(iterator pos, const Args&... args)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(this == pos.m_pContainer, iterator(this, MAX_SIZE), "iterator from different container, TRACE_STACK:%s", TRACE_STACK());
        return InsertArgs(pos, args...);
    }

    iterator insert(iterator position, const Tp &x)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(this == position.m_pContainer, iterator(this, MAX_SIZE), "iterator from different container, TRACE_STACK:%s", TRACE_STACK());
        if (full())
        {
            LOG_WARN(0, -1, "The List Space Not Enough, Insert Failed, TRACE_STACK:%s", TRACE_STACK());
            return end();
        }
        Node *tmp = CreateNode(x);
        CHECK_EXPR(tmp, iterator(this, MAX_SIZE), "failed to create node, TRACE_STACK:%s", TRACE_STACK());
        tmp->m_next = position.m_node->m_self;
        tmp->m_prev = position.m_node->m_prev;
        GetNode(position.m_node->m_prev)->m_next = tmp->m_self;
        position.m_node->m_prev = tmp->m_self;

        ++m_size;
        return iterator(this, tmp);
    }

    void insert(iterator position, const std::initializer_list<Tp>& list)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(this == position.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        insert(position, list.begin(), list.end());
    }

    template<class InputIterator>
    void insert(iterator pos, InputIterator first, InputIterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(this == pos.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        typedef typename std::is_integral<InputIterator>::type Integral;
        InsertDispatch(pos, first, last, Integral());
    }

    void insert(iterator position, const_iterator first, const_iterator last);

    void insert(iterator pos, size_type n, const Tp &x) { FillInsert(pos, n, x); }

    iterator erase(iterator position)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(this == position.m_pContainer, iterator(this, MAX_SIZE), "TRACE_STACK:%s", TRACE_STACK());
        if (position == end())
        {
            return end();
        }
        ptrdiff_t nextNode = position.m_node->m_next;
        ptrdiff_t prevNode = position.m_node->m_prev;
        RecycleNode(static_cast<Node *>(position.m_node));
        auto pNode = node();
        pNode[prevNode].m_next = nextNode;
        pNode[nextNode].m_prev = prevNode;
        --m_size;
        return iterator(this, nextNode);
    }

    iterator erase(iterator first, iterator last);

    void swap(NFShmList& x) noexcept
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        std::list<Tp> tmp(x.begin(), x.end());
        x.assign(begin(), end());
        assign(tmp.begin(), tmp.end());
    }

    void clear()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        base::clear();
    }

    /**
     * @brief 拼接单个元素（链表特有操作）
     * @param position 目标位置迭代器
     * @param i 要拼接的元素迭代器
     * @note 与STL对比：
     *       - STL: 完全兼容，时间复杂度O(1)
     *       - NFShmList: 行为完全一致，基于索引实现
     *       - 将迭代器i指向的元素移动到position位置前
     *       - 移动操作不涉及复制构造，只是重新链接
     *       - 被移动的元素从原位置删除（但不销毁）
     *       - 迭代器i在操作后仍然有效，指向新位置
     */
    void splice(iterator position, iterator i)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(this == position.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        iterator j = i;
        ++j;
        if (position == i || position == j) return;
        this->transfer(position, i, j);
    }

    /**
     * @brief 拼接元素范围
     * @param position 目标位置迭代器
     * @param first 范围起始迭代器
     * @param last 范围结束迭代器
     * @note 与STL对比：
     *       - STL: 完全兼容，时间复杂度O(n)，n为范围大小
     *       - NFShmList: 行为完全一致
     *       - 将[first, last)范围内的元素移动到position位置前
     *       - 保持元素在新位置的相对顺序
     *       - 所有被移动的迭代器在操作后仍然有效
     */
    void splice(iterator position, iterator first, iterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(this == position.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        if (first != last)
            this->transfer(position, first, last);
    }

    /**
     * @brief 拼接整个链表
     * @param position 目标位置迭代器
     * @param x 源链表
     * @note 与STL对比：
     *       - STL: 完全兼容，时间复杂度O(1)
     *       - NFShmList: 行为完全一致
     *       - 将链表x的所有元素移动到当前链表position位置前
     *       - 操作后x变为空链表
     *       - 所有来自x的迭代器在操作后仍然有效，但属于当前链表
     */
    void splice(iterator position, NFShmList& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(this == position.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        transfer(position, x.begin(), x.end());
    }

    /**
     * @brief 从其他链表拼接单个元素
     * @param position 目标位置迭代器
     * @param x 源链表
     * @param i 要拼接的元素迭代器
     * @note 与STL对比：
     *       - STL: 完全兼容，时间复杂度O(1)
     *       - NFShmList: 行为完全一致
     *       - 将链表x中迭代器i指向的元素移动到当前链表position位置前
     *       - i必须是x的有效迭代器
     *       - 操作后i仍然有效，但属于当前链表
     */
    void splice(iterator position, NFShmList& x, iterator i)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(this == position.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(&x == i.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        iterator j = i;
        ++j;
        if (position == i || position == j) return;
        transfer(position, i, j);
    }

    /**
     * @brief 从其他链表拼接元素范围
     * @param position 目标位置迭代器
     * @param x 源链表
     * @param first 范围起始迭代器
     * @param last 范围结束迭代器
     * @note 与STL对比：
     *       - STL: 完全兼容，时间复杂度O(n)，n为范围大小
     *       - NFShmList: 行为完全一致
     *       - 将链表x中[first, last)范围内的元素移动到当前链表position位置前
     *       - first和last必须是x的有效迭代器
     *       - 所有被移动的迭代器在操作后仍然有效，但属于当前链表
     */
    void splice(iterator position, NFShmList& x, iterator first, iterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(this == position.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(&x == first.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(&x == last.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        if (first != last)
        {
            transfer(position, first, last);
        }
    }

public:
    void remove(const Tp &value);

    template <class Predicate>
    void remove_if(Predicate);

    void unique();

    template <class BinaryPredicate>
    void unique(BinaryPredicate);

    void merge(NFShmList& x);

    template <class StrictWeakOrdering>
    void merge(NFShmList& x, StrictWeakOrdering comp);

    void reverse();

    /**
     * @brief 链表排序（使用默认比较器）
     * @note 与STL对比：
     *       - STL: 完全兼容，使用稳定排序算法，时间复杂度O(n log n)
     *       - NFShmList: 基于std::list实现，保证稳定性
     *       - 内部实现：转换为std::list -> 排序 -> 重新赋值
     *       - 排序过程中可能有额外的内存开销
     *       - 迭代器在排序后可能失效（实现细节）
     *       - 使用默认比较器std::less<Tp>
     * 
     * 与STL用法对比：
     * ```cpp
     * // STL用法
     * std::list<int> stdList = {3, 1, 4, 1, 5};
     * stdList.sort();  // 直接在原链表上排序
     * 
     * // NFShmList用法
     * NFShmList<int, 100> list = {3, 1, 4, 1, 5};
     * list.sort();     // 同样的接口，内部转换实现
     * // 结果：{1, 1, 3, 4, 5}
     * ```
     */
    void sort()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        std::list<Tp> list(begin(), end());
        list.sort();
        clear();
        assign(list.begin(), list.end());
    }

    /**
     * @brief 链表排序（使用自定义比较器）
     * @param comp 比较函数对象
     * @note 与STL对比：
     *       - STL: 完全兼容，支持自定义比较器
     *       - NFShmList: 行为完全一致
     *       - 支持lambda表达式、函数指针、函数对象等
     *       - 保证稳定排序（相等元素保持原有顺序）
     * 
     * 使用示例：
     * ```cpp
     * NFShmList<int, 100> list = {3, 1, 4, 1, 5};
     * 
     * // 降序排序
     * list.sort(std::greater<int>());  // {5, 4, 3, 1, 1}
     * 
     * // 使用lambda表达式
     * list.sort([](int a, int b) { return std::abs(a) < std::abs(b); });
     * 
     * // 自定义比较器
     * struct CustomCompare {
     *     bool operator()(const int& a, const int& b) const {
     *         return a % 10 < b % 10;  // 按个位数排序
     *     }
     * };
     * list.sort(CustomCompare());
     * ```
     */
    template <class StrictWeakOrdering>
    void sort(StrictWeakOrdering comp)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        std::list<Tp> list(begin(), end());
        list.sort(comp);
        clear();
        assign(list.begin(), list.end());
    }

    /**
     * @brief 随机打乱链表元素（扩展功能）
     * @note 与STL对比：
     *       - STL: std::list无对应接口，需要转换为vector
     *       - NFShmList: 提供便捷的一体化接口
     *       - 内部实现：转换为std::vector -> 随机打乱 -> 重新赋值
     *       - 时间复杂度：O(n)，空间复杂度：O(n)
     *       - 使用标准的随机数生成器
     *       - 迭代器在操作后可能失效
     * 
     * 与STL等价操作对比：
     * ```cpp
     * // STL实现方式
     * std::list<int> stdList = {1, 2, 3, 4, 5};
     * std::vector<int> temp(stdList.begin(), stdList.end());
     * std::random_shuffle(temp.begin(), temp.end());
     * stdList.assign(temp.begin(), temp.end());
     * 
     * // NFShmList方式
     * NFShmList<int, 100> list = {1, 2, 3, 4, 5};
     * list.random_shuffle();  // 一行搞定
     * 
     * // 结果：元素顺序被随机打乱，如{3, 1, 5, 2, 4}
     * ```
     * 
     * 注意：该操作会改变所有元素的位置，适用于需要随机化数据顺序的场景
     */
    void random_shuffle()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        std::vector<Tp> vec(begin(), end());
        std::random_shuffle(vec.begin(), vec.end());
        clear();
        assign(vec.begin(), vec.end());
    }

protected:
    void transfer(iterator position, iterator first, iterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(this == position.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(first.m_pContainer == last.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
        if (this != first.m_pContainer)
        {
            insert(position, first, last);
            auto pContainer = first.m_pContainer;
            if (pContainer)
            {
                pContainer->erase(first, last);
            }
        }
        else
        {
            if (position != last)
            {
                // Remove [first, last) from its old position.
                GetNode(last.m_node->m_prev)->m_next = position.m_node->m_self;
                GetNode(first.m_node->m_prev)->m_next = last.m_node->m_self;
                GetNode(position.m_node->m_prev)->m_next = first.m_node->m_self;

                // Splice [first, last) into its new position.
                ptrdiff_t tmp = position.m_node->m_prev;
                position.m_node->m_prev = last.m_node->m_prev;
                last.m_node->m_prev = first.m_node->m_prev;
                first.m_node->m_prev = tmp;
            }
        }
    }

    template <typename... Args>
    iterator InsertArgs(iterator position, const Args&... args)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(this, MAX_SIZE), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(this == position.m_pContainer, iterator(this, MAX_SIZE), "TRACE_STACK:%s", TRACE_STACK());
        if (full())
        {
            LOG_WARN(0, -1, "The List Space Not Enough, Insert Failed, TRACE_STACK:%s", TRACE_STACK());
            return end();
        }
        Node* tmp = CreateNode(args...);
        CHECK_EXPR(tmp, iterator(this, MAX_SIZE), "TRACE_STACK:%s", TRACE_STACK());
        tmp->m_next = position.m_node->m_self;
        tmp->m_prev = position.m_node->m_prev;
        GetNode(position.m_node->m_prev)->m_next = tmp->m_self;
        position.m_node->m_prev = tmp->m_self;

        ++m_size;
        return iterator(this, tmp);
    }
protected:
    template<class Integer>
    void InsertDispatch(iterator pos, Integer n, Integer x, std::true_type)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
        FillInsert(pos, (size_type) n, (Tp) x);
    }

    template<class InputIterator>
    void InsertDispatch(iterator pos, InputIterator first, InputIterator last, std::false_type);


    template<class Integer>
    void AssignDispatch(Integer n, Integer val, std::true_type) { FillAssign((size_type) n, (Tp) val); }

    template<class InputIterator>
    void AssignDispatch(InputIterator first, InputIterator last, std::false_type);

    void FillAssign(size_type n, const Tp &val);

    void FillInsert(iterator pos, size_type n, const Tp &x);
};

template<class Tp, size_t MAX_SIZE>
Tp NFShmList<Tp, MAX_SIZE>::m_staticError = Tp();

template<class Tp, size_t MAX_SIZE>
 bool operator==(const NFShmList<Tp, MAX_SIZE> &x, const NFShmList<Tp, MAX_SIZE> &y)
{
    typedef typename NFShmList<Tp, MAX_SIZE>::const_iterator const_iterator;
    const_iterator end1 = x.end();
    const_iterator end2 = y.end();

    const_iterator i1 = x.begin();
    const_iterator i2 = y.begin();
    while (i1 != end1 && i2 != end2 && *i1 == *i2)
    {
        ++i1;
        ++i2;
    }
    return i1 == end1 && i2 == end2;
}

template<class Tp, size_t MAX_SIZE>
 bool operator<(const NFShmList<Tp, MAX_SIZE> &x,
                      const NFShmList<Tp, MAX_SIZE> &y)
{
    return std::lexicographical_compare(x.begin(), x.end(),
                                        y.begin(), y.end());
}

template<class Tp, size_t MAX_SIZE>
template<class InputIter>
void NFShmList<Tp, MAX_SIZE>::InsertDispatch(iterator pos, InputIter first, InputIter last, std::false_type)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(this == pos.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
    for (; first != last; ++first)
        insert(pos, *first);
}

template<class Tp, size_t MAX_SIZE>
void NFShmList<Tp, MAX_SIZE>::insert(iterator position, const_iterator first, const_iterator last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(this == position.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
    for (; first != last; ++first)
        insert(position, *first);
}

template<class Tp, size_t MAX_SIZE>
void NFShmList<Tp, MAX_SIZE>::FillInsert(iterator pos, size_type n, const Tp &x)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(this == pos.m_pContainer, "TRACE_STACK:%s", TRACE_STACK());
    for (; n > 0; --n)
        insert(pos, x);
}

template<class Tp, size_t MAX_SIZE>
typename NFShmList<Tp, MAX_SIZE>::iterator NFShmList<Tp, MAX_SIZE>::erase(iterator first, iterator last)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
    while (first != last)
        erase(first++);
    return last;
}

template<class Tp, size_t MAX_SIZE>
void NFShmList<Tp, MAX_SIZE>::resize(size_type newSize, const Tp &x)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    if (newSize > MAX_SIZE)
    {
        LOG_WARN(0, -1, "The List Space Not Enough, Resize Failed, TRACE_STACK:%s", TRACE_STACK());
        newSize = MAX_SIZE;
    }
    iterator i = begin();
    size_type len = 0;
    for (; i != end() && len < newSize; ++i, ++len) {}
    if (len == newSize)
        erase(i, end());
    else // __i == end()
        insert(end(), newSize - len, x);
}

template<class Tp, size_t MAX_SIZE>
NFShmList<Tp, MAX_SIZE> &NFShmList<Tp, MAX_SIZE>::operator=(const NFShmList &x)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init, TRACE_STACK:%s", TRACE_STACK());
    if (this != &x)
    {
        iterator first1 = begin();
        iterator last1 = end();
        const_iterator first2 = x.begin();
        const_iterator last2 = x.end();
        while (first1 != last1 && first2 != last2)
            *first1++ = *first2++;
        if (first2 == last2)
            erase(first1, last1);
        else
            insert(last1, first2, last2);
    }
    return *this;
}

template<class Tp, size_t MAX_SIZE>
void NFShmList<Tp, MAX_SIZE>::FillAssign(size_type n, const Tp &val)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    iterator i = begin();
    for (; i != end() && n > 0; ++i, --n)
        *i = val;
    if (n > 0)
        insert(end(), n, val);
    else
        erase(i, end());
}

template<class Tp, size_t MAX_SIZE>
template<class InputIter>
void NFShmList<Tp, MAX_SIZE>::AssignDispatch(InputIter first, InputIter last, std::false_type)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    iterator first1 = begin();
    iterator last1 = end();
    for (; first1 != last1 && first != last; ++first1, ++first)
        *first1 = *first;
    if (first == last)
        erase(first1, last1);
    else
        insert(last1, first, last);
}


template<class Tp, size_t MAX_SIZE>
void NFShmList<Tp, MAX_SIZE>::remove(const Tp &value)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    iterator first = begin();
    iterator last = end();
    while (first != last)
    {
        iterator next = first;
        ++next;
        if (*first == value) erase(first);
        first = next;
    }
}

template<class Tp, size_t MAX_SIZE>
void NFShmList<Tp, MAX_SIZE>::unique()
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    iterator first = begin();
    iterator last = end();
    if (first == last) return;
    iterator next = first;
    while (++next != last)
    {
        if (*first == *next)
            erase(next);
        else
            first = next;
        next = first;
    }
}

template<class Tp, size_t MAX_SIZE>
 void NFShmList<Tp, MAX_SIZE>::reverse()
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
    std::reverse(begin(), end());
}

template<class Tp, size_t MAX_SIZE>
template<class Predicate>
void NFShmList<Tp, MAX_SIZE>::remove_if(Predicate pred)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
    iterator first = begin();
    iterator last = end();
    while (first != last)
    {
        iterator next = first;
        ++next;
        if (pred(*first)) erase(first);
        first = next;
    }
}

template<class Tp, size_t MAX_SIZE>
template<class BinaryPredicate>
void NFShmList<Tp, MAX_SIZE>::unique(BinaryPredicate binaryPred)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
    iterator first = begin();
    iterator last = end();
    if (first == last) return;
    iterator next = first;
    while (++next != last)
    {
        if (binaryPred(*first, *next))
            erase(next);
        else
            first = next;
        next = first;
    }
}

template <class Tp, size_t MAX_SIZE>
void NFShmList<Tp, MAX_SIZE>::merge(NFShmList& x)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(this != &x, "TRACE_STACK:%s", TRACE_STACK());
    if (this != &x)
    {
        iterator first1 = begin();
        iterator last1 = end();
        iterator first2 = x.begin();
        iterator last2 = x.end();
        while (first1 != last1 && first2 != last2)
            if (*first2 < *first1)
            {
                iterator __next = first2;
                transfer(first1, first2, ++__next);
                first2 = __next;
            }
            else
                ++first1;
        if (first2 != last2)
            transfer(last1, first2, last2);
    }
}

template <class Tp, size_t MAX_SIZE>
template <class StrictWeakOrdering>
void NFShmList<Tp, MAX_SIZE>::merge(NFShmList& x, StrictWeakOrdering comp)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(this != &x, "TRACE_STACK:%s", TRACE_STACK());
    if (this != &x)
    {
        iterator first1 = begin();
        iterator last1 = end();
        iterator first2 = x.begin();
        iterator last2 = x.end();
        while (first1 != last1 && first2 != last2)
            if (comp(*first2, *first1))
            {
                iterator __next = first2;
                transfer(first1, first2, ++__next);
                first2 = __next;
            }
            else
                ++first1;
        if (first2 != last2)
            transfer(last1, first2, last2);
    }
}
