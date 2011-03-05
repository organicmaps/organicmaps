#ifndef __TIM_SORT_H__
#define __TIM_SORT_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void timsort(void *aArray, size_t aElementCnt, size_t aWidth, int (*aCmpCb)(const void *, const void *));

#endif
