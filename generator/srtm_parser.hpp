#pragma once

#include "geometry/latlon.hpp"

#include "coding/endianness.hpp"

#include "std/map.hpp"
#include "std/string.hpp"

using TSRTMHeighType = int16_t;

int16_t constexpr kInvalidSRTMHeight = -32768;

string GetSRTMBase(ms::LatLon coord);

class SrtmFile
{
public:
  void Init(string dir, ms::LatLon coord);

  TSRTMHeighType getHeight(ms::LatLon coord)
  {
    if (m_data == nullptr)
      return kInvalidSRTMHeight;
    double ln = coord.lon - int(coord.lon);
    if (ln < 0)
      ln += 1;
    double lt = coord.lat - int(coord.lat);
    if (lt < 0)
      lt += 1;
    lt = 1 - lt;  // From North to South

    size_t const row = 3600 * lt;
    size_t const column = 3600 * ln;
    return ReverseByteOrder(m_data[row * 3601 + column]);
    // 3600 and 3601 because 3601 is owerlap column.
  }

  ~SrtmFile()
  {
    if (m_data)
      delete[] m_data;
  }

private:
  TSRTMHeighType * m_data;
};

class SrtmFileManager
{
public:
  SrtmFileManager(string dir) : m_dir(dir) {}
  TSRTMHeighType GetCoordHeight(ms::LatLon coord)
  {
    string base = GetSRTMBase(coord);
    auto it = m_storage.find(base);
    if (it == m_storage.end())
      m_storage[base].Init(m_dir, coord);
    return m_storage[base].getHeight(coord);
  }

private:
  string m_dir;
  map<string, SrtmFile> m_storage;
};
