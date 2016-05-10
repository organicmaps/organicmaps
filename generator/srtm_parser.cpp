#include "generator/srtm_parser.hpp"

#include "coding/endianness.hpp"
#include "coding/zip_reader.hpp"

#include "base/logging.hpp"

#include "std/iomanip.hpp"
#include "std/sstream.hpp"

namespace generator
{
// SrtmFile ----------------------------------------------------------------------------------------
SrtmFile::SrtmFile()
{
  Invalidate();
}

SrtmFile::SrtmFile(SrtmFile && rhs) : m_data(move(rhs.m_data)), m_valid(rhs.m_valid)
{
  rhs.Invalidate();
}

void SrtmFile::Init(string const & dir, ms::LatLon const & coord)
{
  Invalidate();

  string const base = GetBase(coord);
  string const cont = dir + base + ".SRTMGL1.hgt.zip";
  string file = base + ".hgt";

  try
  {
    ZipFileReader::UnzipFileToMemory(cont, file, m_data);
  }
  catch (ZipFileReader::LocateZipException e)
  {
    // Sometimes packed file has different name. See N39E051 measure.
    file = base + ".SRTMGL1.hgt";
    ZipFileReader::UnzipFileToMemory(cont, file, m_data);
  }

  m_valid = true;
}

SrtmFile::THeight SrtmFile::GetHeight(ms::LatLon const & coord)
{
  if (!IsValid())
    return kInvalidHeight;

  double ln = coord.lon - static_cast<int>(coord.lon);
  if (ln < 0)
    ln += 1;
  double lt = coord.lat - static_cast<int>(coord.lat);
  if (lt < 0)
    lt += 1;
  lt = 1 - lt;  // from North to South

  size_t const row = 3600 * lt;
  size_t const col = 3600 * ln;

  size_t const ix = row * 3601 + col;

  if (ix >= Size())
    return kInvalidHeight;
  return ReverseByteOrder(Data()[ix]);
}

string SrtmFile::GetBase(ms::LatLon coord)
{
  ostringstream ss;
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
  ss << setw(2) << setfill('0') << static_cast<int>(coord.lat);

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
  ss << setw(3) << static_cast<int>(coord.lon);
  return ss.str();
}

void SrtmFile::Invalidate()
{
  m_data.clear();
  m_data.shrink_to_fit();
  m_valid = false;
}

// SrtmFileManager ---------------------------------------------------------------------------------
SrtmFileManager::SrtmFileManager(string const & dir) : m_dir(dir) {}

SrtmFile::THeight SrtmFileManager::GetHeight(ms::LatLon const & coord)
{
  string const base = SrtmFile::GetBase(coord);
  auto it = m_storage.find(base);
  if (it == m_storage.end())
  {
    SrtmFile file;
    try
    {
      file.Init(m_dir, coord);
    }
    catch (RootException const & e)
    {
      LOG(LERROR, ("Can't init SRTM file:", base, "reason:", e.Msg()));
    }

    // It's OK to store even invalid files and return invalid height
    // for them later.
    m_storage.emplace(base, move(file));
  }

  return m_storage[base].GetHeight(coord);
}
}  // namespace generator
