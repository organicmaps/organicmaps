#pragma once
#include "../base/base.hpp"

namespace sl
{

struct StrFn
{
  struct Str; // Never defined.

  Str const * (* Create)(char const * utf8Data, uint32_t size);
  void (* Destroy)(Str const * s);
  int (* PrimaryCompare)(void * pData, Str const * a, Str const * b);
  int (* SecondaryCompare)(void * pData, Str const * a, Str const * b);

  void * m_pData;
  uint64_t m_PrimaryCompareId;
  uint64_t m_SecondaryCompareId;
};

}
