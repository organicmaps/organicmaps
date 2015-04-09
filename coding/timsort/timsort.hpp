#pragma once

#include "std/stdint.hpp"

void timsort(void *aArray, size_t aElementCnt, size_t aWidth, int (*aCmpCb)(const void *, const void *));
