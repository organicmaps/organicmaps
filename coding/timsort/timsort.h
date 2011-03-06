#ifndef __TIM_SORT_H__
#define __TIM_SORT_H__

#ifndef _MSC_VER
  #include <stdint.h>
#else
  typedef unsigned char uint8_t;
  typedef __int32 int32_t;
  typedef unsigned __int32 uint32_t;
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void timsort(void *aArray, size_t aElementCnt, size_t aWidth, int (*aCmpCb)(const void *, const void *));

#endif
