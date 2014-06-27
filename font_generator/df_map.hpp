#pragma once

#include "../std/vector.hpp"
#include "../std/stdint.hpp"

class DFMap
{
public:
  DFMap(vector<uint8_t> const & data,
        int width, int height,
        unsigned char inValue,
        unsigned char outValue);

  ~DFMap();

  void Minus(DFMap const & other);
  void Normalize();
  unsigned char * GenerateImage(int & w, int & h);

private:
  void Do(vector<uint8_t> const & data, unsigned char inValue, unsigned char outValue);
  float findRadialDistance(vector<uint8_t> const & data,
                           int pointX, int pointY,
                           int radius, unsigned char outValue) const;

  template<typename T>
  T get(vector<T> const & data, int i, int j) const
  {
    int index = CalcIndex(i, j);
    return data[index];
  }

  template <typename T>
  void put(vector<T> & data, T val, int i, int j)
  {
    int index = CalcIndex(i, j);
    data[index] = val;
  }

  template <typename T>
  void put(T * data, T val, int i, int j)
  {
    int index = CalcIndex(i, j);
    data[index] = val;
  }

  float GetDistance(int i, int j) const;
  void SetDistance(float val, int i, int j);

  int CalcIndex(int i, int j) const { return j * m_width + i; }

private:
  int m_width;
  int m_height;
  vector<float> m_distances;
};
