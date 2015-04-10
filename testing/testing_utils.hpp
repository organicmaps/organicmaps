#pragma once
#include "std/vector.hpp"

template <typename T> vector<T> Vec(T x0)
{
  vector<T> v;
  v.push_back(x0);
  return v;
}

template <typename T> vector<T> Vec(T x0, T x1)
{
  vector<T> v;
  v.push_back(x0);
  v.push_back(x1);
  return v;
}

template <typename T> vector<T> Vec(T x0, T x1, T x2)
{
  vector<T> v;
  v.push_back(x0);
  v.push_back(x1);
  v.push_back(x2);
  return v;
}

template <typename T> vector<T> Vec(T x0, T x1, T x2, T x3)
{
  vector<T> v;
  v.push_back(x0);
  v.push_back(x1);
  v.push_back(x2);
  v.push_back(x3);
  return v;
}
