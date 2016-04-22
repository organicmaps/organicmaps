#include "srtm_parser.hpp"

#include "coding/zip_reader.hpp"

#include "std/iomanip.hpp"

string GetSRTMBase(ms::LatLon coord)
{
  stringstream ss;
  if (coord.lat < 0)
  {
    ss << "S";
    coord.lat *= -1;
    coord.lat += 1;
  }
  else
  {
    ss << "N";
  }
  ss << setw(2) << setfill('0') << int(coord.lat);

  if (coord.lon < 0)
  {
    ss << "W";
    coord.lon *= -1;
    coord.lon += 1;
  }
  else
  {
    ss << "E";
  }
  ss << setw(3) << int(coord.lon);
  return ss.str();
}

void SrtmFile::Init(string dir, ms::LatLon coord)
{
  string base = GetSRTMBase(coord);
  string path = dir + base + ".SRTMGL1.hgt.zip";
  string file = base + ".hgt";
  try
  {
    m_data = reinterpret_cast<TSRTMHeighType *>(ZipFileReader::UnzipFileToMemory(path, file));
  }
  catch (ZipFileReader::OpenZipException e)
  {
    m_data = nullptr;
  }
  catch (ZipFileReader::LocateZipException e)
  {
    try
    {
      // Sometimes packed file has different name. See N39E051 measure.
      file = base + ".SRTMGL1.hgt";
      m_data = reinterpret_cast<TSRTMHeighType *>(ZipFileReader::UnzipFileToMemory(path, file));
    }
    catch (int)
    {
      m_data = nullptr;
    }
  }
}
