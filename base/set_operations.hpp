#pragma once

// Similar to set_difference(), but if element is present n times in the first sequence and once in
// the second sequence, all n copies are filtered, insted of one.
template <typename Iter1T, typename Iter2T, typename OutIterT, typename LessT>
OutIterT SetDifferenceUnlimited(Iter1T beg1, Iter1T end1, Iter2T beg2, Iter2T end2, OutIterT out, LessT lessCompare)
{
  while (beg1 != end1 && beg2 != end2)
  {
    if (lessCompare(*beg1, *beg2))
    {
      *out = *beg1;
      ++beg1;
      ++out;
    }
    else if (lessCompare(*beg2, *beg1))
    {
      ++beg2;
    }
    else
    {
      ++beg1;
      // This is the difference between set_difference and this function:
      // In set_difference the commented line should be present.
      // ++beg2;
    }
  }
  return std::copy(beg1, end1, out);
}
