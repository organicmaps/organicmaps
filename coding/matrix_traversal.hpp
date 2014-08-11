#pragma once

template <typename T> T TraverseMatrixInRowOrder(T n, T i, T j, bool is_back)
{
  return (i * n + j) * 2 + (is_back ? 1 : 0);
}
