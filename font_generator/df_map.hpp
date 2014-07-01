#pragma once

#include "../std/vector.hpp"
#include "../std/stdint.hpp"

#include <boost/gil/typedefs.hpp>

using boost::gil::gray8c_view_t;

class DFMap
{
public:
  DFMap(vector<uint8_t> const & data,
        int32_t width, int32_t height,
        uint8_t inValue,
        uint8_t outValue);

  ~DFMap();

  void Minus(DFMap const & other);
  void Normalize();
  void GenerateImage(vector<uint8_t> & image, int32_t & w, int32_t & h);

private:
  void Do(gray8c_view_t const & view, uint8_t inValue, uint8_t outValue);
  float findRadialDistance(gray8c_view_t const & view,
                           int32_t pointX, int32_t pointY,
                           int32_t radius, uint8_t outValue) const;

  template<typename T>
  T get(vector<T> const & data, int32_t i, int32_t j) const
  {
    int index = CalcIndex(i, j);
    return data[index];
  }

  template <typename T>
  void put(vector<T> & data, T val, int32_t i, int32_t j)
  {
    int index = CalcIndex(i, j);
    data[index] = val;
  }

  template <typename T>
  void put(T * data, T val, int32_t i, int32_t j)
  {
    int index = CalcIndex(i, j);
    data[index] = val;
  }

  float GetDistance(int32_t i, int32_t j) const;
  void SetDistance(float val, int32_t i, int32_t j);

  int32_t CalcIndex(int32_t i, int32_t j) const { return j * m_width + i; }

private:
  int32_t m_width;
  int32_t m_height;
  vector<float> m_distances;
};
