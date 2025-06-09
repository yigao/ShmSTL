// -------------------------------------------------------------------------
//    @FileName         :    NFShmVector.h
//    @Author           :    gaoyi
//    @Date             :    23-1-30
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmVector
//
// -------------------------------------------------------------------------

/**
 * @file NFShmVector.h
 * @brief 基于共享内存的动态数组实现，提供类似std::vector的功能
 *
 * @section overview 概述
 *
 * NFShmVector 是一个专为共享内存环境设计的动态数组容器，为std::vector
 * 提供共享内存兼容的替代方案。在API设计上高度兼容STL vector，但在内存管理、
 * 容量限制等方面针对共享内存环境进行了优化。
 *
 * @section features 核心特性
 *
 * 1. **动态数组结构**：
 *    - 连续内存存储，支持随机访问O(1)
 *    - 动态调整大小，但受MAX_SIZE限制
 *    - 支持双向迭代器和随机访问迭代器
 *
 * 2. **共享内存优化**：
 *    - 固定大小内存布局，适合进程间共享
 *    - 内存对齐优化，提高访问效率
 *    - 支持CREATE/RESUME模式初始化
 *    - 避免动态内存分配
 *
 * 3. **STL高度兼容**：
 *    - 完整的迭代器支持（正向、反向、常量迭代器）
 *    - 标准的容器操作（push_back、pop_back、insert、erase等）
 *    - C++11风格的emplace操作
 *    - initializer_list支持
 *
 * 4. **扩展功能**：
 *    - 二分查找相关操作（binary_search、binary_insert等）
 *    - 排序和去重功能
 *    - 与标准容器的转换操作
 *    - 随机打乱功能
 *
 * 5. **双模式实现**：
 *    - USE_SHM_STL模式：真正的共享内存实现
 *    - 标准模式：基于std::vector的包装器
 *
 * @section stl_comparison STL容器对比
 *
 * | 特性 | STL vector | NFShmVector |
 * |------|------------|-------------|
 * | **数据结构** | 动态数组 | 动态数组 |
 * | **容量管理** | 动态扩容，无限制 | 固定容量MAX_SIZE，编译时确定 |
 * | **内存管理** | 堆内存，自动扩容 | 共享内存，固定内存池 |
 * | **随机访问** | O(1) | O(1) |
 * | **插入删除** | 尾部O(1)，中间O(n) | 尾部O(1)，中间O(n) |
 * | **迭代器类型** | 随机访问迭代器 | 随机访问迭代器 |
 * | **内存布局** | 连续存储 | 连续存储 |
 * | **进程共享** | 不支持 | **原生支持** |
 * | **异常安全** | 强异常安全保证 | 无异常，错误码返回 |
 * | **内存碎片** | 可能产生 | **无碎片**（固定内存池） |
 * | **扩展功能** | 基础操作 | **二分查找、排序等** |
 *
 * @section api_compatibility API兼容性
 *
 * **完全兼容的接口**：
 * - size(), empty(), max_size(), capacity()
 * - begin(), end(), rbegin(), rend(), cbegin(), cend()
 * - front(), back(), at(), operator[](), data()
 * - push_back(), pop_back(), insert(), erase(), clear()
 * - assign(), resize(), reserve(), shrink_to_fit()
 * - emplace(), emplace_back()
 * - swap()
 *
 * **扩展的接口（新增）**：
 * - full() - 检查容器是否已满
 * - CreateInit(), ResumeInit() - 共享内存初始化
 * - binary_insert(), binary_search(), binary_delete() - 二分查找操作
 * - binary_lower_bound(), binary_upper_bound() - 二分边界查找
 * - sort(), is_sorted(), random_shuffle() - 排序相关
 * - remove(), remove_if(), unique() - 元素移除操作
 * - to_vector() - 转换为标准vector
 *
 * **行为差异的接口**：
 * - max_size()：返回MAX_SIZE而非理论最大值
 * - push_back()：容量满时失败而非自动扩容
 * - insert()：容量满时截断而非抛出异常
 * - 构造函数：需要显式调用CreateInit()或ResumeInit()
 *
 * @section usage_examples 使用示例
 *
 * @subsection basic_usage 基础用法（类似std::vector）
 *
 * ```cpp
 * // 定义容量为1000的整数向量
 * NFShmVector<int, 1000> vec;
 * vec.CreateInit();  // 创建模式初始化
 *
 * // STL兼容的基础操作
 * vec.push_back(1);
 * vec.push_back(2);
 * vec.push_back(3);
 *
 * // 随机访问
 * std::cout << "First element: " << vec[0] << std::endl;
 * std::cout << "Size: " << vec.size() << std::endl;
 *
 * // 迭代器遍历
 * for (auto it = vec.begin(); it != vec.end(); ++it) {
 *     std::cout << *it << " ";
 * }
 *
 * // 范围for循环（C++11）
 * for (const auto& item : vec) {
 *     std::cout << item << " ";
 * }
 * ```
 *
 * @subsection construction_modes 构造和初始化模式
 *
 * ```cpp
 * // 1. 默认构造（空容器）
 * NFShmVector<int, 1000> vec1;
 * vec1.CreateInit();
 *
 * // 2. 指定大小构造
 * NFShmVector<int, 1000> vec2(10);  // 10个默认值元素
 *
 * // 3. 指定大小和初值构造
 * NFShmVector<int, 1000> vec3(10, 42);  // 10个值为42的元素
 *
 * // 4. 从迭代器范围构造
 * std::vector<int> stdVec = {1, 2, 3, 4, 5};
 * NFShmVector<int, 1000> vec4(stdVec.begin(), stdVec.end());
 *
 * // 5. 从initializer_list构造（C++11）
 * NFShmVector<int, 1000> vec5 = {1, 2, 3, 4, 5};
 *
 * // 6. 共享内存恢复模式
 * NFShmVector<int, 1000> vec6;
 * vec6.ResumeInit();  // 恢复已存在的共享内存数据
 * ```
 *
 * @subsection binary_operations 二分查找操作
 *
 * ```cpp
 * NFShmVector<int, 1000> vec = {10, 20, 30, 40, 50};
 * vec.sort();  // 确保有序
 *
 * // 二分查找
 * auto it = vec.binary_search(30);
 * if (it != vec.end()) {
 *     std::cout << "Found: " << *it << std::endl;
 * }
 *
 * // 二分插入（保持有序）
 * vec.binary_insert(25);  // 插入到正确位置
 *
 * // 自定义比较器的二分操作
 * auto greater_comp = [](int a, int b) { return a > b; };
 * vec.sort(greater_comp);  // 降序排列
 * vec.binary_insert(35, greater_comp);  // 按降序插入
 *
 * // 范围查找
 * auto lower = vec.binary_lower_bound(20, std::less<int>());
 * auto upper = vec.binary_upper_bound(40, std::less<int>());
 *
 * // 查找所有匹配元素
 * std::vector<decltype(vec)::iterator> results = vec.binary_search_array(30);
 * ```
 *
 * @subsection container_operations 容器操作
 *
 * ```cpp
 * NFShmVector<std::string, 100> vec;
 *
 * // 添加元素
 * vec.push_back("hello");
 * vec.emplace_back("world");  // 原地构造
 *
 * // 插入操作
 * vec.insert(vec.begin() + 1, "beautiful");  // 在位置1插入
 * vec.insert(vec.end(), {"foo", "bar"});     // 批量插入
 *
 * // 删除操作
 * vec.erase(vec.begin());           // 删除第一个元素
 * vec.erase(vec.begin(), vec.begin() + 2);  // 删除范围
 *
 * // 容量检查
 * if (vec.full()) {
 *     std::cout << "Vector is full!" << std::endl;
 * }
 *
 * // 调整大小
 * vec.resize(50, "default");  // 调整到50个元素，新元素为"default"
 * ```
 *
 * @subsection algorithm_operations 算法操作
 *
 * ```cpp
 * NFShmVector<int, 1000> vec = {3, 1, 4, 1, 5, 9, 2, 6, 5};
 *
 * // 排序
 * vec.sort();  // 升序排序
 * vec.sort(std::greater<int>());  // 降序排序
 *
 * // 检查是否有序
 * if (vec.is_sorted()) {
 *     std::cout << "Vector is sorted" << std::endl;
 * }
 *
 * // 去重（需要先排序）
 * vec.sort();
 * vec.unique();  // 移除连续的重复元素
 *
 * // 移除特定值
 * vec.remove(5);  // 移除所有值为5的元素
 *
 * // 条件移除
 * vec.remove_if([](int x) { return x % 2 == 0; });  // 移除偶数
 *
 * // 随机打乱
 * vec.random_shuffle();
 * ```
 *
 * @subsection conversion_operations 转换操作
 *
 * ```cpp
 * // 从标准容器转换
 * std::vector<int> stdVec = {1, 2, 3, 4, 5};
 * std::list<int> stdList = {6, 7, 8, 9, 10};
 * std::set<int> stdSet = {11, 12, 13, 14, 15};
 *
 * NFShmVector<int, 1000> vec;
 * vec = stdVec;   // 从vector赋值
 * vec = stdList;  // 从list赋值
 * vec = stdSet;   // 从set赋值
 * vec = {16, 17, 18, 19, 20};  // 从initializer_list赋值
 *
 * // 转换为标准vector
 * std::vector<int> result = vec.to_vector();
 * ```
 *
 * @section performance_characteristics 性能特征
 *
 * | 操作 | STL vector | NFShmVector |
 * |------|------------|-------------|
 * | **随机访问 [i]** | O(1) | O(1) |
 * | **尾部插入 push_back** | 摊销O(1) | O(1) |
 * | **中间插入 insert** | O(n) | O(n) |
 * | **尾部删除 pop_back** | O(1) | O(1) |
 * | **中间删除 erase** | O(n) | O(n) |
 * | **查找 find** | O(n) | O(n) |
 * | **二分查找 binary_search** | - | **O(log n)** |
 * | **排序 sort** | O(n log n) | O(n log n) |
 * | **内存分配** | 动态扩容 | 预分配固定内存 |
 * | **缓存友好性** | 好（连续内存） | **好**（连续内存） |
 * | **内存开销** | 动态 | 固定MAX_SIZE * sizeof(T) |
 *
 * @section memory_layout 内存布局
 *
 * ```
 * NFShmVector 内存结构：
 * ┌─────────────────┐
 * │   管理数据       │ <- m_size, m_init等
 * ├─────────────────┤
 * │   元素存储区     │ <- m_mem[MAX_SIZE] (AlignedStorage)
 * │   [0]           │    ├─ 元素0
 * │   [1]           │    ├─ 元素1
 * │   ...           │    ├─ ...
 * │   [size-1]      │    ├─ 最后一个有效元素
 * │   [size..MAX-1] │    └─ 未使用空间
 * └─────────────────┘
 *
 * 内存对齐：
 * - 使用 std::aligned_storage 确保正确对齐
 * - 每个元素按照 alignof(T) 对齐
 * - 连续存储，支持指针算术运算
 *
 * 共享内存兼容性：
 * - 固定大小布局，适合mmap映射
 * - 无指针引用，仅使用索引和偏移
 * - 支持不同进程地址空间共享
 * ```
 *
 * @section thread_safety 线程安全
 *
 * - **非线程安全**：需要外部同步机制
 * - **共享内存兼容**：多进程可安全访问（需进程间锁）
 * - **无内部锁**：避免性能开销，由用户控制并发
 * - **读写分离**：多读者可并发，读写需要互斥
 *
 * @section migration_guide 从STL迁移指南
 *
 * 1. **包含头文件**：
 *    ```cpp
 *    // 替换
 *    #include <vector>
 *    // 为
 *    #include "NFComm/NFShmStl/NFShmVector.h"
 *    ```
 *
 * 2. **类型定义**：
 *    ```cpp
 *    // STL
 *    std::vector<int> vec;
 *
 *    // NFShmVector（需要指定最大容量）
 *    NFShmVector<int, 1000> vec;  // 最大1000个元素
 *    ```
 *
 * 3. **初始化**：
 *    ```cpp
 *    // 添加初始化调用
 *    vec.CreateInit();  // 或 ResumeInit() 用于共享内存恢复
 *    ```
 *
 * 4. **容量管理**：
 *    ```cpp
 *    // 添加容量检查
 *    if (vec.full()) {
 *        // 处理容量已满的情况
 *    }
 *
 *    // 移除无限容量假设
 *    // while (condition) vec.push_back(item);  // 可能失败
 *    while (condition && !vec.full()) vec.push_back(item);  // 安全
 *    ```
 *
 * 5. **错误处理**：
 *    ```cpp
 *    // STL异常处理
 *    try {
 *        vec.at(index);
 *    } catch (const std::out_of_range&) {
 *        // 处理越界
 *    }
 *
 *    // NFShmVector边界检查
 *    if (index < vec.size()) {
 *        auto value = vec[index];  // 安全访问
 *    }
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
 * 4. **错误检查**：始终检查full()状态再插入元素
 * 5. **性能优化**：
 *    - 批量操作时使用范围版本的insert/assign
 *    - 利用emplace操作减少拷贝构造
 *    - 对有序数据使用binary_*系列函数
 *
 * @section debugging_support 调试支持
 *
 * NFShmVector提供了一些调试和诊断功能：
 *
 * - **边界检查**：operator[]和at()包含边界检查
 * - **状态验证**：所有操作都会检查初始化状态
 * - **容量监控**：full()函数监控容量使用情况
 * - **转换功能**：to_vector()转换为标准容器便于调试
 *
 * @warning 注意事项
 * - 固定容量限制，超出MAX_SIZE的操作会失败或截断
 * - 共享内存环境下需要考虑进程崩溃的数据恢复
 * - 模板实例化会产生较大的代码量，建议控制不同参数组合的数量
 * - 非线程安全，多线程访问需要外部同步
 *
 * @see NFShmRBTree - 基于共享内存的红黑树实现
 * @see NFShmHashTable - 基于共享内存的哈希表实现
 * @see std::vector - 标准动态数组容器
 *
 * @author gaoyi
 * @date 2023-01-30
 */

#pragma once

#include "NFShmStl.h"
#include <iterator>
#include <algorithm>
#include <vector>
#include <type_traits>

#ifdef USE_SHM_STL

template <class Tp, size_t MAX_SIZE>
class NFShmVectorBase
{
public:
    explicit NFShmVectorBase()
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

    virtual ~NFShmVectorBase()
    {
        memset(m_mem, 0, sizeof(m_mem));
        m_size = 0;
    }

    int CreateInit()
    {
        m_size = 0;
        memset(m_mem, 0, sizeof(m_mem));
        m_init = EN_NF_SHM_STL_INIT_OK;
#ifdef NF_DEBUG_MODE
        m_ptr = (Tp*)m_mem;
#endif
        return 0;
    }

    int ResumeInit()
    {
#ifdef NF_DEBUG_MODE
        m_ptr = (Tp*)m_mem;
#endif
        return 0;
    }

    Tp* base_data() { return reinterpret_cast<Tp*>(m_mem); }
    const Tp* base_data() const { return reinterpret_cast<const Tp*>(m_mem); }

protected:
    // 内存对齐优化（C++11 alignas）
    typedef typename std::aligned_storage<sizeof(Tp), alignof(Tp)>::type AlignedStorage;
    AlignedStorage m_mem[MAX_SIZE];
    size_t m_size;
    int m_init;
#ifdef NF_DEBUG_MODE
    Tp* m_ptr; //用来debug的时候看内存
#endif
};


template <class Tp, size_t MAX_SIZE>
class NFShmVector : protected NFShmVectorBase<Tp, MAX_SIZE>
{
private:
    typedef NFShmVectorBase<Tp, MAX_SIZE> Base;

protected:
    using Base::m_size;
    using Base::m_init;
    using Base::base_data;

public:
    static Tp m_staticError;

public:
    typedef Tp value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

public:
    explicit NFShmVector()
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
     *       - NFShmVector: 支持共享内存CREATE/RESUME两阶段初始化
     *       - 用于共享内存首次创建时的初始化
     */
    int CreateInit()
    {
        return 0;
    }

    /**
     * @brief 恢复模式初始化（共享内存特有）
     * @return 0表示成功，-1表示失败
     * @note 与STL对比：
     *       - STL: 无对应概念
     *       - NFShmVector: 用于从已存在的共享内存恢复容器状态
     *       - 对非平凡构造类型执行placement构造
     *       - 进程重启后恢复共享内存数据使用
     */
    int ResumeInit()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init");
        if (!std::stl_is_trivially_default_constructible<Tp>::value)
        {
            Tp* pData = base_data();
            for (size_t i = 0; i < m_size; i++)
            {
                new(pData + i) Tp(); // 标准placement构造
            }
        }
        return 0;
    }

    //init data in union
    void Init()
    {
        new(this) NFShmVector();
    }

    NFShmVector(size_type n)
    {
        if (n > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor:__n:%lu > MAX_SIZE:%lu, Vector Space Not Enough! n change to MAX_SIZE, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
            n = MAX_SIZE;
        }

        std::__uninitialized_default_n(base_data(), n);
        m_size = n;
    }

    NFShmVector(size_type n, const Tp& value)
    {
        if (n > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor:__n:%lu > MAX_SIZE:%lu, Vector Space Not Enough! n change to MAX_SIZE, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
            n = MAX_SIZE;
        }

        std::uninitialized_fill_n(base_data(), n, value);

        m_size = n;
    }

    template <size_t X_MAX_SIZE>
    NFShmVector(const NFShmVector<Tp, X_MAX_SIZE>& x)
    {
        int maxSize = std::min(MAX_SIZE, x.size());
        auto finish = std::uninitialized_copy_n(x.begin(), maxSize, base_data());
        m_size = finish - begin();
    }

    NFShmVector(const NFShmVector& x)
    {
        int maxSize = std::min(MAX_SIZE, x.size());
        auto finish = std::uninitialized_copy_n(x.begin(), maxSize, base_data());
        m_size = finish - begin();
    }

    NFShmVector(const std::initializer_list<Tp>& list)
    {
        for (auto it = list.begin(); it != list.end(); ++it)
        {
            if (size() >= max_size()) break;

            push_back(*it);
        }
    }

    template <class InputIterator>
    NFShmVector(InputIterator first, InputIterator last)
    {
        typedef typename std::is_integral<InputIterator>::type Integral;

        initialize_aux(first, last, Integral());
    }


    explicit NFShmVector(const std::vector<Tp>& x)
    {
        typedef typename std::is_integral<typename std::vector<Tp>::const_iterator>::type Integral;
        initialize_aux(x.begin(), x.end(), Integral());
    }

    ~NFShmVector()
    {
        if (m_init == EN_NF_SHM_STL_INIT_OK)
        {
            auto pData = base_data();
            std::_Destroy(pData, pData + m_size);
            m_size = 0;
        }
    }

    NFShmVector& operator=(const NFShmVector& x);

    NFShmVector& operator=(const std::vector<Tp>& x);

    NFShmVector& operator=(const std::list<Tp>& x);

    NFShmVector& operator=(const std::set<Tp>& x);

    NFShmVector& operator=(const std::initializer_list<Tp>& list);

public:
    void assign(size_type n, const Tp& val)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        fill_assign(n, val);
    }

    template <class InputIterator>
    void assign(InputIterator first, InputIterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        typedef typename std::is_integral<InputIterator>::type Integral;
        assign_dispatch(first, last, Integral());
    }

    void assign(const std::initializer_list<Tp>& list)
    {
        assign(list.begin(), list.end());
    }

    iterator begin()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, base_data(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return base_data();
    }

    const_iterator begin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, base_data(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return base_data();
    }

    iterator end()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, base_data(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return base_data() + m_size;
    }

    const_iterator end() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, base_data(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return base_data() + m_size;
    }

    reverse_iterator rbegin()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, reverse_iterator(end()), "not init, TRACE_STACK:%s", TRACE_STACK());
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(end()), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_reverse_iterator(end());
    }

    reverse_iterator rend()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, reverse_iterator(begin()), "not init, TRACE_STACK:%s", TRACE_STACK());
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(begin()), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_reverse_iterator(begin());
    }

    const_iterator cbegin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, base_data(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return base_data();
    }

    const_iterator cend() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, base_data(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return base_data() + m_size;
    }

    const_reverse_iterator crbegin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(end()), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crend() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(begin()), "not init, TRACE_STACK:%s", TRACE_STACK());
        return const_reverse_iterator(begin());
    }

    size_type size() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_size;
    }

    size_type max_size() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, MAX_SIZE, "not init, TRACE_STACK:%s", TRACE_STACK());
        return MAX_SIZE;
    }

    void resize(size_type newSize, const Tp& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        if (newSize < size())
            erase(begin() + newSize, end());
        else
            insert(end(), newSize - size(), x);
    }

    void resize(size_type newSize)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        resize(newSize, Tp());
    }

    void shrink_to_fit()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    }

    size_type capacity() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, MAX_SIZE, "not init, TRACE_STACK:%s", TRACE_STACK());
        return MAX_SIZE;
    }

    bool empty() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, true, "not init, TRACE_STACK:%s", TRACE_STACK());
        return begin() == end();
    }

    void reserve(size_type)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    }

    reference operator[](size_type n)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(n < MAX_SIZE, m_staticError, "index n:%lu >= MAX_SIZE:%lu, the server dump, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR(n < m_size, m_staticError, "index n:%lu >= m_size:%lu, you can't use it, TRACE_STACK:%s", n, m_size, TRACE_STACK());

        return *(begin() + n);
    }

    const_reference operator[](size_type n) const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(n < MAX_SIZE, m_staticError, "index n:%lu >= MAX_SIZE:%lu, the server dump, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR(n < m_size, m_staticError, "index n:%lu >= m_size:%lu, you can't use it, TRACE_STACK:%s", n, m_size, TRACE_STACK());

        return *(begin() + n);
    }

    reference at(size_type n)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(n < MAX_SIZE, m_staticError, "index n:%lu >= MAX_SIZE:%lu, the server dump, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR(n < m_size, m_staticError, "index n:%lu >= m_size:%lu, you can't use it, TRACE_STACK:%s", n, m_size, TRACE_STACK());
        return *(begin() + n);
    }

    const_reference at(size_type n) const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(n < MAX_SIZE, m_staticError, "index n:%lu >= MAX_SIZE:%lu, the server dump, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR(n < m_size, m_staticError, "index n:%lu >= m_size:%lu, you can't use it, TRACE_STACK:%s", n, m_size, TRACE_STACK());
        return *(begin() + n);
    }

    reference front()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_size > 0, m_staticError, "vector is empty, size:%lu <= 0, you can't use front(), TRACE_STACK:%s", m_size, TRACE_STACK());

        return *begin();
    }

    const_reference front() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_size > 0, m_staticError, "vector is empty, size:%lu <= 0, you can't use front(), TRACE_STACK:%s", m_size, TRACE_STACK());

        return *begin();
    }

    reference back()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_size > 0, m_staticError, "vector is empty, size:%lu <= 0, you can't use back(), TRACE_STACK:%s", m_size, TRACE_STACK());

        return *(end() - 1);
    }

    const_reference back() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_size > 0, m_staticError, "vector is empty, size:%lu <= 0, you can't use back(), TRACE_STACK:%s", m_size, TRACE_STACK());

        return *(end() - 1);
    }

    Tp* data()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, &m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        return std::addressof(front());
    }

    const Tp* data() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, &m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        return std::addressof(front());
    }

    void push_back(const Tp& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_size < MAX_SIZE, "NFShmVector push_back Failed, Vector Not Enough Space, TRACE_STACK:%s", TRACE_STACK());
        std::_Construct(base_data() + m_size, x);
        ++m_size;
    }

    void pop_back()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        --m_size;
        std::_Destroy(base_data() + m_size);
    }

    template <typename... Args>
    void emplace_back(const Args&... args)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        if (m_size < MAX_SIZE)
        {
            std::_Construct(base_data() + m_size, args...);
            ++m_size;
        }
        else
        {
            LOG_ERR(0, -1, "NFShmVector emplace_back Failed, Vector Not Enough Space, TRACE_STACK:%s", TRACE_STACK());
        }
    }

    template <typename... Args>
    iterator emplace(iterator position, const Args&... args)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        return insert(position, Tp(args...));
    }

    iterator insert(iterator position, const Tp& x)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        size_type n = position - begin();
        CHECK_EXPR(n <= m_size && n <= MAX_SIZE, nullptr, "position not right, TRACE_STACK:%s", TRACE_STACK());
        if (m_size < MAX_SIZE && position == end())
        {
            std::_Construct(base_data() + m_size, x);
            ++m_size;
        }
        else
        {
            insert_aux(position, x);
        }
        return begin() + n;
    }

    void insert(iterator position, const initializer_list<value_type>& l)
    {
        insert(position, l.begin(), l.end());
    }

    void insert(iterator position, const_iterator first, const_iterator last);

    template <class InputIterator>
    void insert(iterator pos, InputIterator first, InputIterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        typedef typename std::is_integral<InputIterator>::type Integral;
        insert_dispatch(pos, first, last, Integral());
    }

    void insert(iterator pos, size_type n, const Tp& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        fill_insert(pos, n, x);
    }

    iterator erase(iterator position)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(position != end(), end(), "pos not right, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR((size_t)(position - begin()) < m_size, end(), "position out of range, TRACE_STACK:%s", TRACE_STACK());
        auto pData = base_data();

        if (position + 1 != end())
            std::copy(position + 1, pData + m_size, position);
        --m_size;
        std::_Destroy(pData + m_size);
        return position;
    }

    iterator erase(iterator first, iterator last)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, end(), "not init, TRACE_STACK:%s", TRACE_STACK());
        auto pData = base_data();
        iterator it = std::copy(last, pData + m_size, first);
        std::_Destroy(it, pData + m_size);

        m_size = m_size - (last - first);
        return first;
    }

    void swap(NFShmVector& x) noexcept
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        NFShmVector temp = x;
        x.assign(begin(), end());
        assign(temp.begin(), temp.end());
    }


    void clear()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        erase(begin(), end());
    }

    // ==================== 扩展功能接口（STL扩展） ====================

    /**
     * @brief 二分插入元素（保持有序）
     * @param val 要插入的值
     * @return 指向插入元素的迭代器，失败返回nullptr
     * @note 与STL对比：
     *       - STL: 无对应接口，需要组合std::lower_bound + insert
     *       - NFShmVector: 提供一体化接口，更方便使用
     *       - 要求容器已排序，使用默认比较器std::less<Tp>
     *       - 自动找到正确位置插入，保持有序性
     *       - 时间复杂度：O(n)（查找O(log n) + 插入O(n)）
     *       - 容量满时插入失败，而非自动扩容
     * 
     * 与STL等价操作对比：
     * ```cpp
     * // STL实现方式
     * std::vector<int> stdVec = {10, 30, 50};
     * auto pos = std::lower_bound(stdVec.begin(), stdVec.end(), 20);
     * stdVec.insert(pos, 20);
     * 
     * // NFShmVector方式
     * NFShmVector<int, 100> vec = {10, 30, 50};
     * auto it = vec.binary_insert(20);
     * if (it != end()) {
     *     // 插入成功，结果：{10, 20, 30, 50}
     * } else {
     *     // 插入失败（容器已满或其他错误）
     * }
     * ```
     */
    iterator binary_insert(const Tp& val)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        return binary_insert(val, std::less<Tp>());
    }

    template <typename Compare>
    iterator binary_insert(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_size < MAX_SIZE, end(), "The Vector No Enough Space! binary_insert Fail!, TRACE_STACK:%s", TRACE_STACK());

        auto iter = std::lower_bound(begin(), end(), val, comp);
        auto newIter = insert(iter, Tp());
        if (newIter != end())
        {
            *newIter = val;
        }
        return newIter;
    }

    template <typename Compare>
    iterator binary_lower_bound(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        return std::lower_bound(begin(), end(), val, comp);
    }

    template <typename Compare>
    iterator binary_upper_bound(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        return std::upper_bound(begin(), end(), val, comp);
    }

    /**
     * @brief 二分查找元素
     * @param val 要查找的值
     * @return 指向找到元素的迭代器，未找到返回end()
     * @note 与STL对比：
     *       - STL: std::binary_search() 只返回bool，需要配合std::equal_range获取迭代器
     *       - NFShmVector: 直接返回迭代器，使用更方便
     *       - 要求容器已排序，使用默认比较器std::less<Tp>
     *       - 时间复杂度：O(log n)，与STL相同
     *       - 未找到时返回end()，而非抛出异常
     * 
     * 与STL等价操作对比：
     * ```cpp
     * // STL实现方式
     * std::vector<int> stdVec = {10, 20, 30, 40, 50};
     * if (std::binary_search(stdVec.begin(), stdVec.end(), 30)) {
     *     auto range = std::equal_range(stdVec.begin(), stdVec.end(), 30);
     *     auto it = range.first;  // 找到的迭代器
     * }
     * 
     * // NFShmVector方式
     * NFShmVector<int, 100> vec = {10, 20, 30, 40, 50};
     * auto it = vec.binary_search(30);
     * if (it != vec.end()) {
     *     // 找到了，it指向值为30的元素
     *     std::cout << "Found: " << *it << std::endl;
     * } else {
     *     // 未找到
     *     std::cout << "Not found" << std::endl;
     * }
     * ```
     */
    iterator binary_search(const Tp& val)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        return binary_search(val, std::less<Tp>());
    }

    template <typename Compare>
    iterator binary_search(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, nullptr, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto pairIter = std::equal_range(begin(), end(), val, comp);
        if (pairIter.first != pairIter.second)
        {
            return pairIter.first;
        }
        return end();
    }

    std::vector<iterator> binary_search_array(const Tp& val)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::vector<iterator>(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return binary_search_array(val, std::less<Tp>());
    }

    template <typename Compare>
    std::vector<iterator> binary_search_array(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::vector<iterator>(), "not init, TRACE_STACK:%s", TRACE_STACK());
        std::vector<iterator> vec;
        auto pairIter = std::equal_range(begin(), end(), val, comp);
        for (auto iter = pairIter.first; iter != pairIter.second; ++iter)
        {
            vec.push_back(iter);
        }
        return vec;
    }

    template <typename Compare>
    int binary_delete(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto pairIter = std::equal_range(begin(), end(), val, comp);
        erase(pairIter.first, pairIter.second);
        return 0;
    }

    int binary_delete(const Tp& val)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        return binary_delete(val, std::less<Tp>());
    }

    template <typename Compare>
    bool is_sorted(Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
        return std::is_sorted(begin(), end(), comp);
    }

    bool is_sorted()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
        return is_sorted(std::less<Tp>());
    }

    /**
     * @brief 容器排序（使用默认比较器）
     * @return 0表示成功，-1表示失败
     * @note 与STL对比：
     *       - STL: std::sort() 是算法，不是容器方法，需要传递迭代器
     *       - NFShmVector: 直接在容器上调用，无需传递迭代器
     *       - 使用默认比较器std::less<Tp>，与STL保持一致
     *       - 时间复杂度：O(n log n)，与STL std::sort相同
     *       - 返回错误码而非抛出异常，适合共享内存环境
     * 
     * 使用示例：
     * ```cpp
     * // STL用法
     * std::vector<int> stdVec = {3, 1, 4, 1, 5};
     * std::sort(stdVec.begin(), stdVec.end());
     * 
     * // NFShmVector用法
     * NFShmVector<int, 100> vec = {3, 1, 4, 1, 5};
     * if (vec.sort() == 0) {
     *     // 排序成功，结果：{1, 1, 3, 4, 5}
     * }
     * ```
     */
    int sort()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        sort(std::less<Tp>());
        return 0;
    }

    template <typename Compare>
    int sort(Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        std::sort(begin(), end(), comp);
        return 0;
    }

    int random_shuffle()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        std::random_shuffle(begin(), end());
        return 0;
    }

    /**
     * @brief 移除所有等于指定值的元素
     * @param value 要移除的值
     * @return 0表示成功，-1表示失败
     * @note 与STL对比：
     *       - STL: std::remove() 是算法，需要配合erase()使用（remove-erase idiom）
     *       - NFShmVector: 一体化接口，自动完成移除和删除
     *       - 移除所有匹配的元素，而非仅第一个
     *       - 时间复杂度：O(n)，与STL相同
     *       - 返回错误码而非抛出异常
     * 
     * 与STL等价操作对比：
     * ```cpp
     * // STL实现方式（remove-erase idiom）
     * std::vector<int> stdVec = {1, 2, 3, 2, 4, 2, 5};
     * stdVec.erase(std::remove(stdVec.begin(), stdVec.end(), 2), 
     *              stdVec.end());
     * // 结果：{1, 3, 4, 5}
     * 
     * // NFShmVector方式
     * NFShmVector<int, 100> vec = {1, 2, 3, 2, 4, 2, 5};
     * if (vec.remove(2) == 0) {
     *     // 移除成功，结果：{1, 3, 4, 5}
     * }
     * ```
     */
    int remove(const Tp& value)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto iter = std::remove(begin(), end(), value);
        if (iter != end())
        {
            erase(iter, end());
        }
        return 0;
    }

    template <class Predicate>
    int remove_if(Predicate pre)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto iter = std::remove_if(begin(), end(), pre);
        if (iter != end())
        {
            erase(iter, end());
        }
        return 0;
    }

    int unique()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto iter = std::unique(begin(), end());
        if (iter != end())
        {
            erase(iter, end());
        }
        return 0;
    }

    /**
     * @brief 转换为标准vector
     * @return std::vector<Tp> 包含所有元素的标准vector
     * @note 与STL对比：
     *       - STL: 无需转换，本身就是std::vector
     *       - NFShmVector: 提供转换接口，便于与STL代码集成
     *       - 创建新的std::vector副本，不影响原容器
     *       - 可用于调试、序列化或与只接受std::vector的接口交互
     *       - 时间复杂度：O(n)，空间复杂度：O(n)
     * 
     * 使用示例：
     * ```cpp
     * NFShmVector<int, 100> vec = {1, 2, 3, 4, 5};
     * 
     * // 转换为标准vector
     * std::vector<int> stdVec = vec.to_vector();
     * 
     * // 与STL算法配合使用
     * std::reverse(stdVec.begin(), stdVec.end());
     * 
     * // 序列化或网络传输
     * serialize(stdVec);
     * 
     * // 调试输出
     * for (const auto& item : stdVec) {
     *     std::cout << item << " ";
     * }
     * ```
     */
    std::vector<Tp> to_vector() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::vector<Tp>(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return std::vector<Tp>(begin(), end());
    }

    /**
     * @brief 检查容器是否已满
     * @return true表示已满，false表示还有空间
     * @note 与STL对比：
     *       - STL: 无对应接口，std::vector可无限扩容
     *       - NFShmVector: 特有功能，体现固定容量限制
     *       - 用于在插入前检查容量，避免溢出
     *       - 等价于 size() >= max_size()
     *       - 与empty()形成对比，提供完整的容量状态检查
     * 
     * 使用示例：
     * ```cpp
     * NFShmVector<int, 100> vec;
     * 
     * // 安全插入模式
     * while (!vec.full() && hasMoreData()) {
     *     vec.push_back(getData());
     * }
     * 
     * // 容量检查
     * if (vec.full()) {
     *     std::cout << "Vector已满，无法继续添加元素" << std::endl;
     * } else {
     *     std::cout << "还可以添加 " << (vec.max_size() - vec.size()) 
     *               << " 个元素" << std::endl;
     * }
     * ```
     */
    bool full()
    {
        return size() >= MAX_SIZE;
    }

protected:
    void insert_aux(iterator position, const Tp& x);

    void insert_aux(iterator position);

    void fill_insert(iterator pos, size_type n, const Tp& x);

    void fill_assign(size_type n, const Tp& val);

    template <class Integer>
    void initialize_aux(Integer n, Integer value, std::true_type)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        if (n > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor _M_initialize_aux, __n:%d > MAX_SIZE:%lu, Vector Space Not Enough! __n change to MAX_SIZE, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
            n = MAX_SIZE;
        }
        std::uninitialized_fill_n(base_data(), n, value);
        m_size = n;
    }

    template <class InputIterator>
    void initialize_aux(InputIterator first, InputIterator last, std::false_type)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        range_initialize(first, last, typename std::iterator_traits<InputIterator>::iterator_category());
    }

    template <class InputIterator>
    void range_initialize(InputIterator first, InputIterator last, std::input_iterator_tag)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        for (; first != last; ++first)
        {
            if (size() >= max_size()) break;
            push_back(*first);
        }
    }

    // This function is only called by the constructor.
    template <class ForwardIterator>
    void range_initialize(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        size_type n = std::distance(first, last);
        if (n > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor range_initialize, __n:%lu > MAX_SIZE:%lu, Vector Space Not Enough! __n change to MAX_SIZE, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
            n = MAX_SIZE;
        }
        auto finish = std::uninitialized_copy_n(first, n, base_data());
        m_size = finish - begin();
    }

    template <class InputIterator>
    void range_insert(iterator pos, InputIterator first, InputIterator last, std::input_iterator_tag);

    template <class ForwardIterator>
    void range_insert(iterator pos, ForwardIterator first, ForwardIterator last, std::forward_iterator_tag);

    template <class Integer>
    void assign_dispatch(Integer n, Integer val, std::true_type)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        fill_assign((size_type)n, (Tp)val);
    }

    template <class InputIter>
    void assign_dispatch(InputIter first, InputIter last, std::false_type)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        assign_aux(first, last, typename std::iterator_traits<InputIter>::iterator_category());
    }

    template <class InputIterator>
    void assign_aux(InputIterator first, InputIterator last, std::input_iterator_tag);

    template <class ForwardIterator>
    void assign_aux(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag);

    template <class Integer>
    void insert_dispatch(iterator pos, Integer n, Integer val, std::true_type)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        fill_insert(pos, (size_type)n, (Tp)val);
    }

    template <class InputIterator>
    void insert_dispatch(iterator pos, InputIterator first, InputIterator last, std::false_type)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        range_insert(pos, first, last, typename std::iterator_traits<InputIterator>::iterator_category());
    }
};

template <class Tp, size_t MAX_SIZE>
Tp NFShmVector<Tp, MAX_SIZE>::m_staticError = Tp();

template <class Tp, int MAX_SIZE>
bool operator==(const NFShmVector<Tp, MAX_SIZE>& x, const NFShmVector<Tp, MAX_SIZE>& y)
{
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}

template <class Tp, int MAX_SIZE>
bool operator<(const NFShmVector<Tp, MAX_SIZE>& x, const NFShmVector<Tp, MAX_SIZE>& y)
{
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const NFShmVector& x)
{
    CHECK_EXPR(x.m_init == EN_NF_SHM_STL_INIT_OK, *this, "__x not init", TRACE_STACK());
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init", TRACE_STACK());
    auto pData = base_data();
    if (&x != this)
    {
        const size_type xLen = x.size();
        if (size() >= xLen)
        {
            iterator i = std::copy(x.begin(), x.end(), begin());
            std::_Destroy(i, pData + m_size);
        }
        else
        {
            std::copy(x.begin(), x.begin() + size(), pData);
            std::uninitialized_copy(x.begin() + size(), x.end(), pData + m_size);
        }
        m_size = xLen;
    }
    return *this;
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const std::vector<Tp>& x)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init, TRACE_STACK:%s", TRACE_STACK());
    assign(x.begin(), x.end());
    return *this;
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const std::list<Tp>& x)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (auto it = x.begin(); it != x.end(); ++it)
    {
        if (size() >= max_size()) break;

        push_back(*it);
    }
    return *this;
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const std::set<Tp>& x)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (auto it = x.begin(); it != x.end(); ++it)
    {
        if (size() >= max_size()) break;

        push_back(*it);
    }
    return *this;
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const std::initializer_list<Tp>& list)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        if (size() >= max_size()) break;

        push_back(*it);
    }
    return *this;
}

template <class Tp, size_t MAX_SIZE>
void NFShmVector<Tp, MAX_SIZE>::fill_assign(size_type n, const Tp& val)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init", TRACE_STACK());
    if (n > capacity())
    {
        LOG_WARN(0, -1, "The Vector Left Space:%lu Not Enough! Can't Assign %lu Element, Only %lu, TRACE_STACK:%s", MAX_SIZE, n, MAX_SIZE, TRACE_STACK());
        n = capacity();
    }

    if (n > size())
    {
        std::fill(begin(), end(), val);
        std::uninitialized_fill_n(base_data() + m_size, n - size(), val);
        m_size = n;
    }
    else
        erase(std::fill_n(begin(), n, val), end());
}

template <class Tp, size_t MAX_SIZE>
void NFShmVector<Tp, MAX_SIZE>::insert_aux(iterator position, const Tp& x)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(m_size < MAX_SIZE, "The Vector No Enough Space!, TRACE_STACK:%s", TRACE_STACK());
    auto pData = base_data();
    std::_Construct(pData + m_size, *(pData + m_size - 1));

    ++m_size;
    std::copy_backward(position, pData + m_size - 2, pData + m_size - 1);
    *position = x;
}

template <class Tp, size_t MAX_SIZE>
void NFShmVector<Tp, MAX_SIZE>::insert_aux(iterator position)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(m_size < MAX_SIZE, "The Vector No Enough Space!, TRACE_STACK:%s", TRACE_STACK());
    auto pData = base_data();
    std::_Construct(pData + m_size, *(pData + m_size - 1));

    ++m_size;
    std::copy_backward(position, pData + m_size - 2, pData + m_size - 1);
    *position = Tp();
}

template <class Tp, size_t MAX_SIZE>
void NFShmVector<Tp, MAX_SIZE>::fill_insert(iterator pos, size_type n, const Tp& x)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    auto pData = base_data();
    if (n != 0)
    {
        CHECK_EXPR_RE_VOID(m_size < MAX_SIZE, "The Vector No Enough Space! Insert Fail! size:%lu max_size:%lu, TRACE_STACK:%s", m_size, MAX_SIZE, TRACE_STACK());
        if (static_cast<size_type>(MAX_SIZE - m_size) < n)
        {
            LOG_WARN(0, -1, "The Vector Left Space:%lu Not Enough! Can't Insert %lu Element, Only %lu, TRACE_STACK:%s", MAX_SIZE - m_size, n, MAX_SIZE - m_size, TRACE_STACK());
            n = static_cast<size_type>(MAX_SIZE - m_size);
        }

        const size_type elemsAfter = pData + m_size - pos;
        iterator oldFinish = pData + m_size;
        if (elemsAfter > n)
        {
            std::uninitialized_copy(pData + m_size - n, pData + m_size, pData + m_size);
            m_size += n;
            std::copy_backward(pos, oldFinish - n, oldFinish);
            std::fill(pos, pos + n, x);
        }
        else
        {
            std::uninitialized_fill_n(pData + m_size, n - elemsAfter, x);
            m_size += n - elemsAfter;
            std::uninitialized_copy(pos, oldFinish, pData + m_size);
            m_size += elemsAfter;
            std::fill(pos, oldFinish, x);
        }
    }
}

template <class Tp, size_t MAX_SIZE>
void NFShmVector<Tp, MAX_SIZE>::insert(iterator position, const_iterator first, const_iterator last)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init", TRACE_STACK());
    size_t posN = position - begin();
    CHECK_EXPR_RE_VOID(posN <= m_size && posN <= MAX_SIZE, "position not right, TRACE_STACK:%s", TRACE_STACK());
    auto pData = base_data();
    if (first != last)
    {
        CHECK_EXPR_RE_VOID(m_size < MAX_SIZE, "The Vector No Enough Space! Insert Fail!, TRACE_STACK:%s", TRACE_STACK());
        size_type n = std::distance(first, last);

        if (static_cast<size_type>(MAX_SIZE - m_size) < n)
        {
            LOG_WARN(0, -1, "The Vector Left Space:%lu Not Enough! Can't Insert %lu Element, Only %lu, TRACE_STACK:%s", MAX_SIZE - m_size, n, MAX_SIZE - m_size, TRACE_STACK());
            n = static_cast<size_type>(MAX_SIZE - m_size);
            auto temp = first;
            std::advance(temp, n);
            last = temp;
            CHECK_EXPR_RE_VOID(std::distance(first, last) == n, "error, TRACE_STACK:%s", TRACE_STACK());
        }

        const size_type elemsAfter = pData + m_size - position;
        iterator oldFinish = pData + m_size;
        if (elemsAfter > n)
        {
            std::uninitialized_copy(pData + m_size - n, pData + m_size, pData + m_size);
            m_size += n;
            std::copy_backward(position, oldFinish - n, oldFinish);
            std::copy(first, last, position);
        }
        else
        {
            std::uninitialized_copy(first + elemsAfter, last, pData + m_size);
            m_size += n - elemsAfter;
            std::uninitialized_copy(position, oldFinish, pData + m_size);
            m_size += elemsAfter;
            std::copy(first, first + elemsAfter, position);
        }
    }
}

template <class Tp, size_t MAX_SIZE>
template <class InputIter>
void NFShmVector<Tp, MAX_SIZE>::assign_aux(InputIter first, InputIter last, std::input_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    iterator cur = begin();
    for (; first != last && cur != end(); ++cur, ++first)
        *cur = *first;
    if (first == last)
        erase(cur, end());
    else
        insert(end(), first, last);
}

template <class Tp, size_t MAX_SIZE>
template <class ForwardIter>
void NFShmVector<Tp, MAX_SIZE>::assign_aux(ForwardIter first, ForwardIter last, std::forward_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    auto pData = base_data();
    size_type len = std::distance(first, last);

    if (len > capacity())
    {
        LOG_ERR(0, -1, "__len > capacity(), some copy not success, TRACE_STACK:%s", TRACE_STACK());
        std::_Destroy(pData, pData + m_size);

        auto finish = std::uninitialized_copy_n(first, MAX_SIZE, pData);
        m_size = finish - begin();
    }
    else if (size() >= len)
    {
        iterator newFinish = std::copy(first, last, pData);
        std::_Destroy(newFinish, pData + m_size);

        m_size = newFinish - begin();
    }
    else
    {
        ForwardIter mid = first;
        std::advance(mid, size());
        std::copy(first, mid, pData);
        auto finish = std::uninitialized_copy(mid, last, pData + m_size);
        m_size = finish - begin();
    }
}

template <class Tp, size_t MAX_SIZE>
template <class InputIterator>
void NFShmVector<Tp, MAX_SIZE>::range_insert(iterator pos, InputIterator first, InputIterator last, std::input_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_t posN = pos - begin();
    CHECK_EXPR_RE_VOID(posN <= m_size && posN <= MAX_SIZE, "position not right, TRACE_STACK:%s", TRACE_STACK());
    for (; first != last; ++first)
    {
        pos = insert(pos, *first);
        if (pos == end())
        {
            break;
        }
        ++pos;
    }
}

template <class Tp, size_t MAX_SIZE>
template <class ForwardIterator>
void NFShmVector<Tp, MAX_SIZE>::range_insert(iterator pos, ForwardIterator first, ForwardIterator last, std::forward_iterator_tag)
{
    CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
    size_t posN = pos - begin();
    CHECK_EXPR_RE_VOID(posN <= m_size && posN <= MAX_SIZE, "position not right, TRACE_STACK:%s", TRACE_STACK());
    CHECK_EXPR_RE_VOID(m_size < MAX_SIZE, "The Vector No Enough Space! Insert Fail!, TRACE_STACK:%s", TRACE_STACK());
    auto pData = base_data();
    if (first != last)
    {
        size_type n = std::distance(first, last);

        if (static_cast<size_type>(MAX_SIZE - m_size) < n)
        {
            LOG_WARN(0, -1, "The Vector Left Space:%lu Not Enough! Can't Insert %lu Element, Only %lu, TRACE_STACK:%s", MAX_SIZE - m_size, n, MAX_SIZE - m_size, TRACE_STACK());
            n = static_cast<size_type>(MAX_SIZE - m_size);
            auto temp = first;
            std::advance(temp, n);
            last = temp;
            CHECK_EXPR_RE_VOID(std::distance(first, last) == n, "TRACE_STACK:%s", TRACE_STACK());
        }

        const size_type elemsAfter = pData + m_size - pos;
        iterator oldFinish = pData + m_size;
        if (elemsAfter > n)
        {
            std::uninitialized_copy(pData + m_size - n, pData + m_size, pData + m_size);
            m_size += n;
            std::copy_backward(pos, oldFinish - n, oldFinish);
            std::copy(first, last, pos);
        }
        else
        {
            ForwardIterator mid = first;
            std::advance(mid, elemsAfter);
            std::uninitialized_copy(mid, last, pData + m_size);
            m_size += n - elemsAfter;
            std::uninitialized_copy(pos, oldFinish, pData + m_size);
            m_size += elemsAfter;
            std::copy(first, mid, pos);
        }
    }
}
#else
template <class Tp, size_t MAX_SIZE>
class NFShmVectorBase
{
public:
    explicit NFShmVectorBase(): m_init(EN_NF_SHM_STL_INIT_OK)
    {
    }

    virtual ~NFShmVectorBase()
    {
    }

protected:
    int8_t m_init;
};

template <class Tp, size_t MAX_SIZE = UINT64_MAX>
class NFShmVector : public NFShmVectorBase<Tp, MAX_SIZE>
{
private:
    std::vector<Tp> m_data;

public:
    static Tp m_staticError;

private:
    typedef NFShmVectorBase<Tp, MAX_SIZE> Base;

protected:
    using Base::m_init;

public:
    typedef typename std::vector<Tp>::value_type value_type;
    typedef typename std::vector<Tp>::pointer pointer;
    typedef typename std::vector<Tp>::const_pointer const_pointer;
    typedef typename std::vector<Tp>::iterator iterator;
    typedef typename std::vector<Tp>::const_iterator const_iterator;
    typedef typename std::vector<Tp>::reference reference;
    typedef typename std::vector<Tp>::const_reference const_reference;
    typedef typename std::vector<Tp>::size_type size_type;
    typedef typename std::vector<Tp>::difference_type difference_type;

    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

public:
    explicit NFShmVector()
    {
    }

    //init data in union
    void Init()
    {
        new(this) NFShmVector();
    }

    NFShmVector(size_type n)
    {
        if (n > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor:__n:%lu > MAX_SIZE:%lu, Vector Space Not Enough! n change to MAX_SIZE, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
            n = MAX_SIZE;
        }

        m_data.assign(n, Tp());
    }

    NFShmVector(size_type n, const Tp& value)
    {
        if (n > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor:__n:%lu > MAX_SIZE:%lu, Vector Space Not Enough! n change to MAX_SIZE, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
            n = MAX_SIZE;
        }

        m_data.assign(n, value);
    }

    template <size_t X_MAX_SIZE>
    NFShmVector(const NFShmVector<Tp, X_MAX_SIZE>& x)
    {
        int maxSize = std::min(MAX_SIZE, x.size());
        for (int i = 0; i < maxSize; i++)
        {
            push_back(x[i]);
        }
    }

    NFShmVector(const NFShmVector& x)
    {
        int maxSize = std::min(MAX_SIZE, x.size());
        for (int i = 0; i < maxSize; i++)
        {
            push_back(x[i]);
        }
    }

    NFShmVector(const std::initializer_list<Tp>& list)
    {
        for (auto it = list.begin(); it != list.end(); ++it)
        {
            if (size() >= max_size()) break;

            push_back(*it);
        }
    }

    template <class InputIterator>
    NFShmVector(InputIterator first, InputIterator last)
    {
        typedef typename std::is_integral<InputIterator>::type Integral;

        initialize_aux(first, last, Integral());
    }


    explicit NFShmVector(const std::vector<Tp>& x)
    {
        for (auto iter = x.begin(); iter != x.end(); ++iter)
        {
            if (size() >= max_size()) break;

            push_back(*iter);
        }
    }

    virtual ~NFShmVector()
    {
    }

    NFShmVector& operator=(const NFShmVector& x);

    NFShmVector& operator=(const std::vector<Tp>& x);

    NFShmVector& operator=(const std::list<Tp>& x);

    NFShmVector& operator=(const std::set<Tp>& x);

    NFShmVector& operator=(const std::initializer_list<Tp>& list);

public:
    void assign(size_type n, const Tp& val)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        if (n > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor:__n:%lu > MAX_SIZE:%lu, Vector Space Not Enough! n change to MAX_SIZE, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
            n = MAX_SIZE;
        }
        m_data.assign(n, val);
    }

    template <class InputIterator>
    void assign(InputIterator first, InputIterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        size_t n = last - first;
        if (n > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor:__n:%lu > MAX_SIZE:%lu, Vector Space Not Enough! n change to MAX_SIZE, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
            n = MAX_SIZE;
        }
        m_data.assign(first, first + n);
    }

    void assign(const std::initializer_list<Tp>& list)
    {
        m_data.assign(list.begin(), list.end());
    }

    iterator begin()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.begin();
    }

    const_iterator begin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.begin();
    }

    iterator end()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.end();
    }

    const_iterator end() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.end();
    }

    reverse_iterator rbegin()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, reverse_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.rbegin();
    }

    const_reverse_iterator rbegin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.rbegin();
    }

    reverse_iterator rend()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, reverse_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.rend();
    }

    const_reverse_iterator rend() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.rend();
    }

    const_iterator cbegin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.cbegin();
    }

    const_iterator cend() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.cend();
    }

    const_reverse_iterator crbegin() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.crbegin();
    }

    const_reverse_iterator crend() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, const_reverse_iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.crend();
    }

    size_type size() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.size();
    }

    size_type max_size() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, MAX_SIZE, "not init, TRACE_STACK:%s", TRACE_STACK());
        return MAX_SIZE;
    }

    void resize(size_type newSize, const Tp& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        if (newSize > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor:newSize:%lu > MAX_SIZE:%lu, Vector Space Not Enough! n change to MAX_SIZE, TRACE_STACK:%s", newSize, MAX_SIZE, TRACE_STACK());
            newSize = MAX_SIZE;
        }
        m_data.resize(newSize, x);
    }

    void resize(size_type newSize)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        resize(newSize, Tp());
    }

    void shrink_to_fit()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        m_data.shrink_to_fit();
    }

    size_type capacity() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
        return MAX_SIZE;
    }

    bool empty() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, true, "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.empty();
    }

    int reserve(size_t n)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        m_data.reserve(n);
        return 0;
    }

    reference operator[](size_type n)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(n < MAX_SIZE, m_staticError, "index n:%lu >= MAX_SIZE:%lu, the server dump, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR(n < m_data.size(), m_staticError, "index n:%lu >= m_size:%lu, you can't use it, TRACE_STACK:%s", n, m_data.size(), TRACE_STACK());

        return m_data[n];
    }

    const_reference operator[](size_type n) const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(n < MAX_SIZE, m_staticError, "index n:%lu >= MAX_SIZE:%lu, the server dump, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR(n < m_data.size(), m_staticError, "index n:%lu >= m_size:%lu, you can't use it, TRACE_STACK:%s", n, m_data.size(), TRACE_STACK());

        return m_data[n];
    }

    reference at(size_type n)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(n < MAX_SIZE, m_staticError, "index n:%lu >= MAX_SIZE:%lu, the server dump, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR(n < m_data.size(), m_staticError, "index n:%lu >= m_size:%lu, you can't use it, TRACE_STACK:%s", n, m_data.size(), TRACE_STACK());
        return m_data.at(n);
    }

    const_reference at(size_type n) const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(n < MAX_SIZE, m_staticError, "index n:%lu >= MAX_SIZE:%lu, the server dump, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
        CHECK_EXPR(n < m_data.size(), m_staticError, "index n:%lu >= m_size:%lu, you can't use it, TRACE_STACK:%s", n, m_data.size(), TRACE_STACK());
        return m_data.at(n);
    }

    reference front()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_data.size() > 0, m_staticError, "vector is empty, size:%lu <= 0, you can't use front(), TRACE_STACK:%s", m_data.size(), TRACE_STACK());

        return m_data.front();
    }

    const_reference front() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_data.size() > 0, m_staticError, "vector is empty, size:%lu <= 0, you can't use front(), TRACE_STACK:%s", m_data.size(), TRACE_STACK());

        return m_data.front();
    }

    reference back()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_data.size() > 0, m_staticError, "vector is empty, size:%lu <= 0, you can't use back(), TRACE_STACK:%s", m_data.size(), TRACE_STACK());

        return m_data.back();
    }

    const_reference back() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_data.size() > 0, m_staticError, "vector is empty, size:%lu <= 0, you can't use back(), TRACE_STACK:%s", m_data.size(), TRACE_STACK());

        return m_data.back();
    }

    Tp* data()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, &m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.data();
    }

    const Tp* data() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, &m_staticError, "not init, TRACE_STACK:%s", TRACE_STACK());
        return m_data.data();
    }

    void push_back(const Tp& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_data.size() < MAX_SIZE, "m_data.size():%lu >= MAX_SIZE:%lu, Vector Space Not Enough, TRACE_STACK:%s", m_data.size(), MAX_SIZE, TRACE_STACK());
        m_data.push_back(x);
    }

    void pop_back()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_data.size() > 0, "m_data.size():%lu <= 0, you can't use it, TRACE_STACK:%s", m_data.size(), TRACE_STACK());
        m_data.pop_back();
    }

    template <typename... Args>
    void emplace_back(Args&&... args)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR_RE_VOID(m_data.size() < MAX_SIZE, "m_data.size():%lu >= MAX_SIZE:%lu, Vector Space Not Enough, TRACE_STACK:%s", m_data.size(), MAX_SIZE, TRACE_STACK());
        m_data.emplace_back(std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator emplace(iterator position, Args&&... args)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_data.size() < MAX_SIZE, end(), "m_data.size():%lu >= MAX_SIZE:%lu, Vector Space Not Enough, TRACE_STACK:%s", m_data.size(), MAX_SIZE, TRACE_STACK());
        size_type n = position - begin();
        CHECK_EXPR(n <= m_data.size(), end(), "position not right, TRACE_STACK:%s", TRACE_STACK());
        return m_data.emplace(position, std::forward<Args>(args)...);
    }

    iterator insert(iterator position, Tp&& x)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_data.size() < MAX_SIZE, end(), "m_data.size():%lu >= MAX_SIZE:%lu, Vector Space Not Enough, TRACE_STACK:%s", m_data.size(), MAX_SIZE, TRACE_STACK());
        size_type n = position - begin();
        CHECK_EXPR(n <= m_data.size(), end(), "position not right, TRACE_STACK:%s", TRACE_STACK());
        return m_data.emplace(position, std::move(x));
    }

    iterator insert(iterator position, const Tp& x)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(m_data.size() < MAX_SIZE, end(), "m_data.size():%lu >= MAX_SIZE:%lu, Vector Space Not Enough, TRACE_STACK:%s", m_data.size(), MAX_SIZE, TRACE_STACK());
        size_type n = position - begin();
        CHECK_EXPR(n <= m_data.size(), end(), "position not right, TRACE_STACK:%s", TRACE_STACK());
        return m_data.insert(position, x);
    }

    void insert(iterator position, const initializer_list<value_type>& l)
    {
        insert(position, l.begin(), l.end());
    }

    void insert(iterator position, const_iterator first, const_iterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        size_type posN = position - begin();
        CHECK_EXPR_RE_VOID(posN <= m_data.size(), "position not right, TRACE_STACK:%s", TRACE_STACK());
        size_type n = last - first;
        if (n > m_data.max_size() - m_data.size())
        {
            n = m_data.max_size() - m_data.size();
        }
        m_data.insert(position, first, first + n);
    }

    template <class InputIterator>
    void insert(iterator pos, InputIterator first, InputIterator last)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        typedef typename std::is_integral<InputIterator>::type Integral;
        insert_dispatch(pos, first, last, Integral());
    }

    void insert(iterator pos, size_type n, const Tp& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        if (n > m_data.max_size() - m_data.size())
        {
            n = m_data.max_size() - m_data.size();
        }
        m_data.insert(pos, n, x);
    }

    iterator erase(iterator position)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR(position != end(), end(), "pos not right, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR((size_t)(position - begin()) < m_data.size(), end(), "position out of range, TRACE_STACK:%s", TRACE_STACK());
        return m_data.erase(position);
    }

    iterator erase(iterator first, iterator last)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR((size_t)(first - begin()) < m_data.size(), end(), "pos not right, TRACE_STACK:%s", TRACE_STACK());
        CHECK_EXPR((size_t)(last - begin()) <= m_data.size(), end(), "pos not right, TRACE_STACK:%s", TRACE_STACK());
        return m_data.erase(first, last);
    }

    void swap(NFShmVector& x)
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        m_data.swap(x.m_data);
    }


    void clear()
    {
        CHECK_EXPR_RE_VOID(m_init == EN_NF_SHM_STL_INIT_OK, "not init, TRACE_STACK:%s", TRACE_STACK());
        m_data.clear();
    }

    iterator binary_insert(const Tp& val)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return binary_insert(val, std::less<Tp>());
    }

    template <typename Compare>
    iterator binary_insert(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());

        auto iter = std::lower_bound(begin(), end(), val, comp);
        auto newIter = insert(iter, Tp());
        if (newIter != end())
        {
            *newIter = val;
        }
        return newIter;
    }

    template <typename Compare>
    iterator binary_lower_bound(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());

        return std::lower_bound(begin(), end(), val, comp);
    }

    template <typename Compare>
    iterator binary_upper_bound(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());

        return std::upper_bound(begin(), end(), val, comp);
    }

    iterator binary_search(const Tp& val)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return binary_search(val, std::less<Tp>());
    }

    template <typename Compare>
    iterator binary_search(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, iterator(), "not init, TRACE_STACK:%s", TRACE_STACK());
        auto pairIter = std::equal_range(begin(), end(), val, comp);
        if (pairIter.first != pairIter.second)
        {
            return pairIter.first;
        }
        return end();
    }

    std::vector<iterator> binary_search_array(const Tp& val)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::vector<iterator>(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return binary_search_array(val, std::less<Tp>());
    }

    template <typename Compare>
    std::vector<iterator> binary_search_array(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::vector<iterator>(), "not init, TRACE_STACK:%s", TRACE_STACK());
        std::vector<iterator> vec;
        auto pairIter = std::equal_range(begin(), end(), val, comp);
        for (auto iter = pairIter.first; iter != pairIter.second; ++iter)
        {
            vec.push_back(iter);
        }
        return vec;
    }

    template <typename Compare>
    int binary_delete(const Tp& val, Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto pairIter = std::equal_range(begin(), end(), val, comp);
        erase(pairIter.first, pairIter.second);
        return 0;
    }

    int binary_delete(const Tp& val)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        return binary_delete(val, std::less<Tp>());
    }

    template <typename Compare>
    bool is_sorted(Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
        return std::is_sorted(begin(), end(), comp);
    }

    bool is_sorted()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
        return is_sorted(std::less<Tp>());
    }

    int sort()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        sort(std::less<Tp>());
        return 0;
    }

    template <typename Compare>
    int sort(Compare comp)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        std::sort(begin(), end(), comp);
        return 0;
    }

    int random_shuffle()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        std::random_shuffle(begin(), end());
        return 0;
    }

    int remove(const Tp& value)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto iter = std::remove(begin(), end(), value);
        if (iter != end())
        {
            erase(iter, end());
        }
        return 0;
    }

    template <class Predicate>
    int remove_if(Predicate pre)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto iter = std::remove_if(begin(), end(), pre);
        if (iter != end())
        {
            erase(iter, end());
        }
        return 0;
    }

    int unique()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        auto iter = std::unique(begin(), end());
        if (iter != end())
        {
            erase(iter, end());
        }
        return 0;
    }

    bool full()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
        return size() >= MAX_SIZE;
    }

    std::vector<Tp> to_vector() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, std::vector<Tp>(), "not init, TRACE_STACK:%s", TRACE_STACK());
        return std::vector<Tp>(begin(), end());
    }

private:
    template <class Integer>
    int initialize_aux(Integer n, Integer value, std::true_type)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        if (n > MAX_SIZE)
        {
            LOG_WARN(0, -1, "NFShmVector Constructor _M_initialize_aux, __n:%d > MAX_SIZE:%lu, Vector Space Not Enough! __n change to MAX_SIZE, TRACE_STACK:%s", n, MAX_SIZE, TRACE_STACK());
            n = MAX_SIZE;
        }
        m_data.assign(n, value);
        return 0;
    }

    template <class InputIterator>
    int initialize_aux(InputIterator first, InputIterator last, std::false_type)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        for (auto iter = first; iter != last; ++iter)
        {
            if (size() >= max_size()) break;
            push_back(*iter);
        }
        return 0;
    }

    template <class Integer>
    int insert_dispatch(iterator pos, Integer n, Integer val, std::true_type)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        if (n > max_size() - size())
        {
            n = max_size() - size();
        }
        m_data.insert(pos, (size_type)n, (Tp)val);
        return 0;
    }

    template <class InputIterator>
    int insert_dispatch(iterator pos, InputIterator first, InputIterator last, std::false_type)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        range_insert(pos, first, last, typename std::iterator_traits<InputIterator>::iterator_category());
        return 0;
    }

    template <class InputIterator>
    int range_insert(iterator pos, InputIterator first, InputIterator last, std::input_iterator_tag)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        size_t posN = pos - begin();
        CHECK_EXPR(posN <= size() && posN <= MAX_SIZE, -1, "position not right, TRACE_STACK:%s", TRACE_STACK());
        for (; first != last; ++first)
        {
            pos = insert(pos, *first);
            if (pos == end())
            {
                break;
            }
            ++pos;
        }
        return 0;
    }

    template <class ForwardIterator>
    int range_insert(iterator pos, ForwardIterator first, ForwardIterator last, std::forward_iterator_tag)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, -1, "not init, TRACE_STACK:%s", TRACE_STACK());
        size_t posN = pos - begin();
        CHECK_EXPR(posN <= size() && posN <= max_size(), -1, "position not right, TRACE_STACK:%s", TRACE_STACK());
        if (first != last)
        {
            size_type n = std::distance(first, last);
            size_type leftSpace = static_cast<size_type>(max_size() - size());

            if (leftSpace < n)
            {
                LOG_WARN(0, -1, "The Vector Left Space:%lu Not Enough! Can't Insert %lu Element, Only %lu, TRACE_STACK:%s", leftSpace, n, leftSpace, TRACE_STACK());
                n = leftSpace;
            }

            auto temp = first;
            std::advance(temp, n);
            last = temp;
            m_data.insert(pos, first, last);
        }
        return 0;
    }
};

template <class Tp, size_t MAX_SIZE>
Tp NFShmVector<Tp, MAX_SIZE>::m_staticError = Tp();

template <class Tp, int MAX_SIZE>
bool operator==(const NFShmVector<Tp, MAX_SIZE>& x, const NFShmVector<Tp, MAX_SIZE>& y)
{
    return x.m_data == y.m_data;
}

template <class Tp, int MAX_SIZE>
bool operator<(const NFShmVector<Tp, MAX_SIZE>& x, const NFShmVector<Tp, MAX_SIZE>& y)
{
    return x.m_data < y.m_data;
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const NFShmVector& x)
{
    CHECK_EXPR(x.m_init == EN_NF_SHM_STL_INIT_OK, *this, "__x not init", TRACE_STACK());
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init", TRACE_STACK());
    for (int i = 0; i < x.size(); ++i)
    {
        if (size() >= max_size()) break;
        push_back(x[i]);
    }
    return *this;
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const std::vector<Tp>& x)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (int i = 0; i < x.size(); ++i)
    {
        if (size() >= max_size()) break;
        push_back(x[i]);
    }
    return *this;
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const std::list<Tp>& x)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (auto it = x.begin(); it != x.end(); ++it)
    {
        if (size() >= max_size()) break;
        push_back(*it);
    }
    return *this;
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const std::set<Tp>& x)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (auto it = x.begin(); it != x.end(); ++it)
    {
        if (size() >= max_size()) break;
        push_back(*it);
    }
    return *this;
}

template <class Tp, size_t MAX_SIZE>
NFShmVector<Tp, MAX_SIZE>& NFShmVector<Tp, MAX_SIZE>::operator=(const std::initializer_list<Tp>& list)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, *this, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        if (size() >= max_size()) break;
        push_back(*it);
    }
    return *this;
}
#endif
