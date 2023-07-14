/******************************************************************************
* Copyright (c) 2020, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/
/*
这段代码是一个命名空间 `entwine` 中的模板函数实现。该命名空间中定义了两个函数模板 `min_element` 和 `max_element`，这些函数分别用于找到范围内的最小值和最大值。

这些函数的实现基于迭代器（Iterator）的概念，用于表示范围内的元素序列。下面是对这些函数的简要解释：

- `min_element`：该函数用于找到范围内的最小元素，并返回指向该元素的迭代器。有两个重载版本：
  - 第一个版本接受两个迭代器参数 `curr` 和 `last`，表示要查找的范围。函数从 `curr` 开始遍历到 `last`（不包括 `last`）之间的元素，找到最小元素并返回指向它的迭代器。
  - 第二个版本接受一个额外的比较函数对象 `comp`，用于指定元素之间的比较方式。函数根据比较结果找到最小元素并返回指向它的迭代器。

- `max_element`：该函数用于找到范围内的最大元素，并返回指向该元素的迭代器。同样有两个重载版本：
  - 第一个版本接受两个迭代器参数 `curr` 和 `last`，表示要查找的范围。函数从 `curr` 开始遍历到 `last`（不包括 `last`）之间的元素，找到最大元素并返回指向它的迭代器。
  - 第二个版本接受一个额外的比较函数对象 `comp`，用于指定元素之间的比较方式。函数根据比较结果找到最大元素并返回指向它的迭代器。

这些函数的实现基于迭代器的遍历和比较操作，通过不断更新最小或最大元素的迭代器来找到最终结果。这些函数可以用于各种类型的容器，如数组、向量、链表等。

请注意，这只是代码片段的一部分，可能还有其他相关的代码，这段代码所属的具体功能和用途可能需要结合上下文来确定。
*/
namespace entwine
{

template<class ForwardIt>
ForwardIt min_element(ForwardIt curr, ForwardIt last)
{
    if (curr == last) return last;

    ForwardIt smallest = curr;
    ++curr;
    for ( ; curr != last; ++curr)
    {
        if (*curr < *smallest) smallest = curr;
    }
    return smallest;
}

template<class ForwardIt, class Compare>
ForwardIt min_element(ForwardIt curr, ForwardIt last, Compare comp)
{
    if (curr == last) return last;

    ForwardIt smallest = curr;
    ++curr;
    for ( ; curr != last; ++curr)
    {
        if (comp(*curr, *smallest)) smallest = curr;
    }
    return smallest;
}

template<class ForwardIt>
ForwardIt max_element(ForwardIt curr, ForwardIt last)
{
    if (curr == last) return last;

    ForwardIt largest = curr;
    ++curr;
    for ( ; curr != last; ++curr)
    {
        if (*largest < *curr) largest = curr;
    }
    return largest;
}

template<class ForwardIt, class Compare>
ForwardIt max_element(ForwardIt curr, ForwardIt last, Compare comp)
{
    if (curr == last) return last;

    ForwardIt largest = curr;
    ++curr;
    for ( ; curr != last; ++curr)
    {
        if (comp(*largest, *curr)) largest = curr;
    }
    return largest;
}

} // namespace entwine
