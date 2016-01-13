#pragma once

#ifdef new
#undef new
#endif

#include <algorithm>

using std::all_of;
using std::binary_search;
using std::equal;
using std::fill;
using std::find;
using std::find_if;
using std::find_first_of;
using std::is_sorted;
using std::lexicographical_compare;
using std::lower_bound;
using std::max;
using std::max_element;
using std::min;
using std::next_permutation;
using std::sort;
using std::stable_sort;
using std::partial_sort;
using std::swap;
using std::upper_bound;
using std::unique;
using std::equal_range;
using std::for_each;
using std::copy;
using std::remove_if;
using std::replace;
using std::reverse;
using std::set_union;
using std::set_intersection;
// Bug workaround, see http://connect.microsoft.com/VisualStudio/feedbackdetail/view/840578/algorithm-possible-c-compiler-bug-when-using-std-set-difference-with-custom-comperator
#ifdef _MSC_VER
namespace vs_bug
{
template<class InputIt1, class InputIt2, class OutputIt, class Compare>
OutputIt set_difference( InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, OutputIt d_first, Compare comp)
{
  while (first1 != last1)
  {
    if (first2 == last2)
      return std::copy(first1, last1, d_first);
    if (comp(*first1, *first2))
      *d_first++ = *first1++;
    else
    {
      if (!comp(*first2, *first1))
        ++first1;
      ++first2;
    }
  }
  return d_first;
}

template<class InputIt1, class InputIt2, class OutputIt>
OutputIt set_difference(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, OutputIt d_first)
{
  while (first1 != last1)
  {
    if (first2 == last2)
      return std::copy(first1, last1, d_first);

    if (*first1 < *first2)
      *d_first++ = *first1++;
    else
    {
      if (! (*first2 < *first1))
        ++first1;
      ++first2;
    }
  }
  return d_first;
}

} // namespace vc_bug
#else
using std::set_difference;
#endif
using std::set_symmetric_difference;
using std::transform;
using std::push_heap;
using std::pop_heap;
using std::sort_heap;
using std::distance;
using std::remove_copy_if;
using std::generate;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
