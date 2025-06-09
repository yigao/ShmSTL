// -------------------------------------------------------------------------
//    @FileName         :    NFShmSet.h
//    @Author           :    Craft
//    @Date             :    2025/1/16
//    @Email            :    445267987@qq.com
//    @Module           :    NFShmSet
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFShmStl/NFShmRBTree.h"
#include <functional>

/**
 * @file NFShmSet.h
 * @brief 基于共享内存的有序集合容器实现，与STL std::set兼容
 * 
 * 本文件实现了一个基于共享内存的红黑树集合容器，在API设计上尽量兼容STL标准，
 * 但在内存管理、容量限制等方面有显著差异。
 */

/****************************************************************************
 * STL std::set 对比分析
 ****************************************************************************
 * 
 * 1. 内存管理策略对比：
 *    - std::set: 动态内存分配，使用allocator管理堆内存，节点按需分配释放
 *    - NFShmSet: 固定大小共享内存，预分配所有节点，支持进程间共享持久化
 * 
 * 2. 容量管理对比：
 *    - std::set: 动态扩容，理论上只受系统内存限制
 *    - NFShmSet: 固定容量MAX_SIZE，编译时确定，无法动态扩容
 * 
 * 3. 元素唯一性对比：
 *    - 都保证元素唯一性，不允许重复元素
 *    - 都基于红黑树实现，元素自动排序
 *    - 都使用insert_unique策略确保唯一性
 * 
 * 4. 元素存储对比：
 *    - std::set: 直接存储元素值，键即值
 *    - NFShmSet: 同样直接存储元素值，使用std::stl__Identity函数提取键
 * 
 * 5. 性能特征对比：
 *    - std::set: O(log n)查找/插入/删除，动态内存分配开销
 *    - NFShmSet: O(log n)查找/插入/删除，无内存分配开销，但受固定容量限制
 * 
 * 6. 线程安全对比：
 *    - std::set: 非线程安全，需要外部同步
 *    - NFShmSet: 非线程安全，但支持进程间共享，需要进程间同步
 * 
 * 7. API兼容性：
 *    - 兼容接口：insert, find, erase, begin, end, size, empty, count, equal_range等
 *    - 特有接口：full(), left_size(), CreateInit(), ResumeInit()等共享内存特有功能
 *    - 迭代器特性：都是const_iterator，元素不可修改（保证有序性）
 * 
 * 8. 构造和初始化对比：
 *    - std::set: 标准构造函数，支持多种初始化方式
 *    - NFShmSet: 支持从STL容器构造，增加共享内存特有的CreateInit/ResumeInit模式
 *****************************************************************************/

// ==================== 前向声明 ====================

template <class Key, size_t MAX_SIZE, class Compare>
class NFShmSet;

// ==================== 主要容器类 ====================

/**
 * @brief 基于共享内存的有序集合容器
 * @tparam Key 元素类型（键即值）
 * @tparam MAX_SIZE 最大容量（固定）
 * @tparam Compare 元素比较函数类型，默认std::less<Key>
 * 
 * 设计特点：
 * 1. 基于红黑树实现，保证元素有序性
 * 2. 保证元素唯一性，不允许重复元素（与NFShmMultiSet的主要区别）
 * 3. 固定容量，不支持动态扩容（与STL主要区别）
 * 4. 基于共享内存，支持进程间共享和持久化
 * 5. API设计尽量兼容STL std::set
 * 6. 迭代器为const_iterator，元素不可修改
 * 
 * 与std::set的主要差异：
 * - 容量限制：固定大小 vs 动态扩容
 * - 内存管理：共享内存 vs 堆内存
 * - 进程支持：进程间共享 vs 单进程内使用
 * - 初始化：CreateInit/ResumeInit vs 标准构造
 * - 插入返回：pair<iterator, bool> vs pair<iterator, bool>（兼容）
 */
template <class Key, size_t MAX_SIZE, class Compare = std::less<Key>>
class NFShmSet
{
public:
    // ==================== STL兼容类型定义 ====================

    typedef Key key_type;                           ///< 键类型（与值类型相同）
    typedef Key value_type;                         ///< 值类型（与键类型相同）
    typedef Compare key_compare;                    ///< 键比较函数类型
    typedef Compare value_compare;                  ///< 值比较函数类型（与键比较函数相同）
    typedef const value_type* pointer;              ///< 指针类型（注意：const，元素不可修改）
    typedef const value_type* const_pointer;       ///< 常量指针类型
    typedef const value_type& reference;           ///< 引用类型（注意：const，元素不可修改）
    typedef const value_type& const_reference;     ///< 常量引用类型
    typedef size_t size_type;                       ///< 大小类型
    typedef ptrdiff_t difference_type;              ///< 差值类型

private:
    /// @brief 底层红黑树类型，使用std::stl__Identity提取键，保证唯一性
    typedef NFShmRBTree<Key, value_type, std::stl__Identity<value_type>, MAX_SIZE, key_compare> rep_type;
    rep_type m_tree;                                ///< 底层红黑树实例

public:
    // ==================== STL兼容迭代器类型 ====================
    // 注意：set的所有迭代器都是const_iterator，确保元素不可修改以维护有序性

    typedef typename rep_type::const_iterator iterator;                    ///< 迭代器类型（实际为const_iterator）
    typedef typename rep_type::const_iterator const_iterator;              ///< 常量迭代器类型
    typedef std::reverse_iterator<iterator> reverse_iterator;              ///< 反向迭代器类型
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;  ///< 常量反向迭代器类型

public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 默认构造函数
     * @note 与std::set()行为类似，但增加共享内存初始化逻辑
     * @note 根据SHM_CREATE_MODE决定创建或恢复模式
     */
    NFShmSet()
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
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @note 与std::set(first, last)兼容
     */
    template <class InputIterator>
    NFShmSet(InputIterator first, InputIterator last)
    {
        insert(first, last);
    }

    /**
     * @brief 拷贝构造函数
     * @param x 源NFShmSet对象
     * @note 与std::set拷贝构造函数兼容
     */
    NFShmSet(const NFShmSet& x)
    {
        insert(x.begin(), x.end());
    }

    /**
     * @brief 指针范围构造函数
     * @param first 起始指针
     * @param last 结束指针
     * @note STL容器没有此接口，为方便使用而提供
     */
    NFShmSet(const value_type* first, const value_type* last)
    {
        insert(first, last);
    }

    /**
     * @brief 迭代器范围构造函数（const_iterator版本）
     * @param first 起始常量迭代器
     * @param last 结束常量迭代器
     */
    NFShmSet(const_iterator first, const_iterator last)
    {
        insert(first, last);
    }

    /**
     * @brief 初始化列表构造函数
     * @param list 初始化列表
     * @note 与std::set(std::initializer_list)兼容
     */
    NFShmSet(const std::initializer_list<Key>& list)
    {
        insert(list.begin(), list.end());
    }

    /**
     * @brief 从std::unordered_set构造
     * @param set 源std::unordered_set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmSet(const std::unordered_set<Key>& set) { insert(set.begin(), set.end()); }

    /**
     * @brief 从std::set构造
     * @param set 源std::set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmSet(const std::set<Key>& set) { insert(set.begin(), set.end()); }

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
        new(this) NFShmSet();
    }

    // ==================== 赋值操作符（STL兼容） ====================

    /**
     * @brief 拷贝赋值操作符
     * @param x 源NFShmSet对象
     * @return 自身引用
     * @note 与std::set::operator=兼容
     */
    NFShmSet& operator=(const NFShmSet& x)
    {
        if (this != &x)
        {
            clear();
            insert(x.begin(), x.end());
        }
        return *this;
    }

    /**
     * @brief 从std::unordered_set赋值
     * @param x 源std::unordered_set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmSet& operator=(const std::unordered_set<Key>& x)
    {
        clear();
        insert(x.begin(), x.end());
        return *this;
    }

    /**
     * @brief 从std::set赋值
     * @param x 源std::set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmSet& operator=(const std::set<Key>& x)
    {
        clear();
        insert(x.begin(), x.end());
        return *this;
    }

    /**
     * @brief 从初始化列表赋值
     * @param x 初始化列表
     * @return 自身引用
     * @note 与std::set::operator=(std::initializer_list)兼容
     */
    NFShmSet& operator=(const std::initializer_list<Key>& x)
    {
        clear();
        insert(x.begin(), x.end());
        return *this;
    }

    // ==================== 迭代器接口（STL兼容） ====================

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器（实际为const_iterator）
     * @note 与std::set::begin()兼容，返回const_iterator以防止修改元素
     */
    iterator begin() { return m_tree.begin(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::set::begin() const兼容
     */
    const_iterator begin() const { return m_tree.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器（实际为const_iterator）
     * @note 与std::set::end()兼容
     */
    iterator end() { return m_tree.end(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::set::end() const兼容
     */
    const_iterator end() const { return m_tree.end(); }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向最后一个元素的反向迭代器
     * @note 与std::set::rbegin()兼容
     */
    reverse_iterator rbegin() { return reverse_iterator(end()); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个元素的常量反向迭代器
     * @note 与std::set::rbegin() const兼容
     */
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个元素前面的反向迭代器
     * @note 与std::set::rend()兼容
     */
    reverse_iterator rend() { return reverse_iterator(begin()); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素前面的常量反向迭代器
     * @note 与std::set::rend() const兼容
     */
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    // ==================== C++11 新增迭代器（STL兼容） ====================

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::set::cbegin()兼容
     */
    const_iterator cbegin() const { return m_tree.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::set::cend()兼容
     */
    const_iterator cend() const { return m_tree.end(); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个元素的常量反向迭代器
     * @note 与std::set::crbegin()兼容
     */
    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素前面的常量反向迭代器
     * @note 与std::set::crend()兼容
     */
    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

    // ==================== 容量相关接口（STL兼容） ====================

    /**
     * @brief 判断是否为空
     * @return true表示空
     * @note 与std::set::empty()兼容
     */
    bool empty() const { return m_tree.empty(); }

    /**
     * @brief 获取当前元素数量
     * @return 元素数量
     * @note 与std::set::size()兼容
     */
    size_type size() const { return m_tree.size(); }

    /**
     * @brief 获取最大容量
     * @return 最大容量MAX_SIZE
     * @note 与std::set::max_size()不同，返回固定值而非理论最大值
     */
    size_type max_size() const { return m_tree.max_size(); }

    /**
     * @brief 判断是否已满
     * @return true表示已满
     * @note STL容器没有此接口，固定容量特有
     */
    bool full() const { return m_tree.full(); }

    // ==================== 修改器接口（STL兼容） ====================

    /**
     * @brief 插入元素（保证唯一性）
     * @param x 要插入的元素
     * @return pair<iterator, bool>，iterator指向插入位置，bool表示是否成功插入
     * @note 与std::set::insert()兼容，保证元素唯一性
     */
    std::pair<iterator, bool> insert(const value_type& x) { return m_tree.insert_unique(x); }

    /**
     * @brief 带提示插入元素
     * @param position 位置提示常量迭代器
     * @param x 要插入的元素
     * @return 指向插入位置的迭代器
     * @note 与std::set::insert(const_iterator, const value_type&)兼容
     */
    iterator insert(const_iterator position, const value_type& x) { return m_tree.insert_unique(position, x); }

    /**
     * @brief 范围插入
     * @tparam InputIterator 输入迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @note 与std::set::insert(first, last)兼容
     */
    template <class InputIterator>
    void insert(InputIterator first, InputIterator last) { m_tree.insert_unique(first, last); }

    /**
     * @brief 指针范围插入
     * @param first 起始指针
     * @param last 结束指针
     */
    void insert(const value_type* first, const value_type* last) { m_tree.insert_unique(first, last); }

    /**
     * @brief 迭代器范围插入
     * @param first 起始常量迭代器
     * @param last 结束常量迭代器
     */
    void insert(const_iterator first, const_iterator last) { m_tree.insert_unique(first, last); }

    /**
     * @brief 就地构造元素
     * @tparam Args 构造参数类型包
     * @param args 构造参数
     * @return pair<iterator, bool>，iterator指向构造位置，bool表示是否成功构造
     * @note 与std::set::emplace()兼容
     */
    template <class... Args>
    std::pair<iterator, bool> emplace(const Args&... args) { return m_tree.emplace_unique(args...); }

    /**
     * @brief 带提示就地构造元素
     * @tparam Args 构造参数类型包
     * @param position 位置提示常量迭代器
     * @param args 构造参数
     * @return 指向构造位置的迭代器
     * @note 与std::set::emplace_hint()兼容
     */
    template <class... Args>
    iterator emplace_hint(const_iterator position, const Args&... args) { return m_tree.emplace_hint_unique(position, args...); }

    /**
     * @brief 根据常量迭代器删除元素
     * @param position 指向要删除元素的常量迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::set::erase(const_iterator)兼容
     */
    iterator erase(const_iterator position) { return m_tree.erase(position); }

    /**
     * @brief 根据键删除元素
     * @param k 要删除的键（元素值）
     * @return 删除的元素数量（0或1）
     * @note 与std::set::erase(const key_type&)兼容
     */
    size_type erase(const key_type& k) { return m_tree.erase(k); }

    /**
     * @brief 删除指定范围的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @note 与std::set::erase(iterator, iterator)兼容
     */
    iterator erase(const_iterator first, const_iterator last) { return m_tree.erase(first, last); }

    /**
     * @brief 删除指定范围的元素（指针版本）
     * @param first 起始指针
     * @param last 结束指针
     */
    void erase(const key_type* first, const key_type* last) { m_tree.erase(first, last); }

    /**
     * @brief 清空所有元素
     * @note 与std::set::clear()兼容
     */
    void clear() { m_tree.clear(); }

    // ==================== 观察器接口（STL兼容） ====================

    /**
     * @brief 获取键比较器
     * @return 键比较器对象
     * @note 与std::set::key_comp()兼容
     */
    key_compare key_comp() const { return m_tree.key_comp(); }

    /**
     * @brief 获取值比较器
     * @return 值比较器对象（与键比较器相同）
     * @note 与std::set::value_comp()兼容
     */
    value_compare value_comp() const { return m_tree.key_comp(); }

    // ==================== 操作接口（STL兼容） ====================

    /**
     * @brief 查找指定键的元素
     * @param k 要查找的键（元素值）
     * @return 指向找到元素的迭代器，未找到返回end()
     * @note 与std::set::find()兼容
     */
    iterator find(const key_type& k) { return m_tree.find(k); }

    /**
     * @brief 查找指定键的元素（常量版本）
     * @param k 要查找的键（元素值）
     * @return 指向找到元素的常量迭代器，未找到返回end()
     * @note 与std::set::find() const兼容
     */
    const_iterator find(const key_type& k) const { return m_tree.find(k); }

    /**
     * @brief 统计指定键的元素数量
     * @param k 要统计的键
     * @return 元素数量（0或1，因为set保证元素唯一）
     * @note 与std::set::count()兼容
     */
    size_type count(const key_type& k) const { return find(k) == end() ? 0 : 1; } // set中每个键最多只有一个元素

    /**
     * @brief 查找第一个不小于指定键的元素
     * @param k 要查找的键
     * @return 指向第一个不小于k的元素的迭代器
     * @note 与std::set::lower_bound()兼容
     */
    iterator lower_bound(const key_type& k) { return m_tree.lower_bound(k); }

    /**
     * @brief 查找第一个不小于指定键的元素（常量版本）
     * @param k 要查找的键
     * @return 指向第一个不小于k的元素的常量迭代器
     * @note 与std::set::lower_bound() const兼容
     */
    const_iterator lower_bound(const key_type& k) const { return m_tree.lower_bound(k); }

    /**
     * @brief 查找第一个大于指定键的元素
     * @param k 要查找的键
     * @return 指向第一个大于k的元素的迭代器
     * @note 与std::set::upper_bound()兼容
     */
    iterator upper_bound(const key_type& k) { return m_tree.upper_bound(k); }

    /**
     * @brief 查找第一个大于指定键的元素（常量版本）
     * @param k 要查找的键
     * @return 指向第一个大于k的元素的常量迭代器
     * @note 与std::set::upper_bound() const兼容
     */
    const_iterator upper_bound(const key_type& k) const { return m_tree.upper_bound(k); }

    /**
     * @brief 获取指定键的元素范围
     * @param k 要查找的键
     * @return pair<iterator, iterator>表示范围[lower_bound, upper_bound)
     * @note 与std::set::equal_range()兼容
     */
    std::pair<iterator, iterator> equal_range(const key_type& k) { return m_tree.equal_range(k); }

    /**
     * @brief 获取指定键的元素范围（常量版本）
     * @param k 要查找的键
     * @return pair<const_iterator, const_iterator>表示范围[lower_bound, upper_bound)
     * @note 与std::set::equal_range() const兼容
     */
    std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const { return m_tree.equal_range(k); }

    // ==================== 交换操作（STL兼容） ====================

    /**
     * @brief 交换两个集合的内容
     * @param x 另一个NFShmSet对象
     * @note 与std::set::swap()兼容
     */
    void swap(NFShmSet& x)
    {
        if (this != &x)
        {
            m_tree.swap(x.m_tree);
        }
    }

    // ==================== 调试和诊断接口 ====================

    /**
     * @brief 打印红黑树结构（图形化显示）
     * @note 调试用，显示底层红黑树的层次结构和节点颜色
     */
    void print_structure() const { m_tree.print_structure(); }

    /**
     * @brief 打印详细信息
     * @note 调试用，显示所有节点的详细状态和属性
     */
    void print_detailed() const { m_tree.print_detailed(); }

    /**
     * @brief 打印简化信息
     * @note 调试用，仅显示树的基本统计信息
     */
    void print_simple() const { m_tree.print_simple(); }
};

// ==================== 全局比较操作符（STL兼容） ====================

/**
 * @brief 相等比较操作符
 * @param x 第一个NFShmSet对象
 * @param y 第二个NFShmSet对象
 * @return true表示相等
 * @note 与std::set的operator==兼容
 */
template <class Key, size_t MAX_SIZE, class Compare>
bool operator==(const NFShmSet<Key, MAX_SIZE, Compare>& x,
                const NFShmSet<Key, MAX_SIZE, Compare>& y)
{
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}

/**
 * @brief 小于比较操作符
 * @param x 第一个NFShmSet对象
 * @param y 第二个NFShmSet对象
 * @return true表示x < y
 * @note 与std::set的operator<兼容
 */
template <class Key, size_t MAX_SIZE, class Compare>
bool operator<(const NFShmSet<Key, MAX_SIZE, Compare>& x,
               const NFShmSet<Key, MAX_SIZE, Compare>& y)
{
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

/**
 * @brief 不等比较操作符
 * @param x 第一个NFShmSet对象
 * @param y 第二个NFShmSet对象
 * @return true表示不等
 * @note 与std::set的operator!=兼容
 */
template <class Key, size_t MAX_SIZE, class Compare>
bool operator!=(const NFShmSet<Key, MAX_SIZE, Compare>& x,
                const NFShmSet<Key, MAX_SIZE, Compare>& y)
{
    return !(x == y);
}

/**
 * @brief 大于比较操作符
 * @param x 第一个NFShmSet对象
 * @param y 第二个NFShmSet对象
 * @return true表示x > y
 * @note 与std::set的operator>兼容
 */
template <class Key, size_t MAX_SIZE, class Compare>
bool operator>(const NFShmSet<Key, MAX_SIZE, Compare>& x,
               const NFShmSet<Key, MAX_SIZE, Compare>& y)
{
    return y < x;
}

/**
 * @brief 小于等于比较操作符
 * @param x 第一个NFShmSet对象
 * @param y 第二个NFShmSet对象
 * @return true表示x <= y
 * @note 与std::set的operator<=兼容
 */
template <class Key, size_t MAX_SIZE, class Compare>
bool operator<=(const NFShmSet<Key, MAX_SIZE, Compare>& x,
                const NFShmSet<Key, MAX_SIZE, Compare>& y)
{
    return !(y < x);
}

/**
 * @brief 大于等于比较操作符
 * @param x 第一个NFShmSet对象
 * @param y 第二个NFShmSet对象
 * @return true表示x >= y
 * @note 与std::set的operator>=兼容
 */
template <class Key, size_t MAX_SIZE, class Compare>
bool operator>=(const NFShmSet<Key, MAX_SIZE, Compare>& x,
                const NFShmSet<Key, MAX_SIZE, Compare>& y)
{
    return !(x < y);
}

// ==================== 特化的swap函数（STL兼容） ====================

/**
 * @brief 特化的swap函数
 * @param x 第一个NFShmSet对象
 * @param y 第二个NFShmSet对象
 * @note 与std::swap(std::set&, std::set&)兼容
 */
template <class Key, size_t MAX_SIZE, class Compare>
inline void swap(NFShmSet<Key, MAX_SIZE, Compare>& x,
                 NFShmSet<Key, MAX_SIZE, Compare>& y)
{
    x.swap(y);
}
