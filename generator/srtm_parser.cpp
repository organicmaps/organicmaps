#include "generator/srtm_parser.hpp"

#include "coding/endianness.hpp"
#include "coding/zip_reader.hpp"

#include "base/logging.hpp"

#include <iomanip>
#include <sstream>

namespace generator
{
namespace
{
size_t constexpr kArcSecondsInDegree = 60 * 60;
size_t constexpr kSrtmTileSize = (kArcSecondsInDegree + 1) * (kArcSecondsInDegree + 1) * 2;

struct UnzipMemDelegate : public ZipFileReader::Delegate
{
  UnzipMemDelegate(std::string & buffer) : m_buffer(buffer), m_completed(false) {}

  // ZipFileReader::Delegate overrides:
  void OnBlockUnzipped(size_t size, char const * data) override { m_buffer.append(data, size); }

  void OnStarted() override
  {
    m_buffer.clear();
    m_completed = false;
  }

  void OnCompleted() override { m_completed = true; }

  std::string & m_buffer;
  bool m_completed;
};
}  // namespace

// SrtmTile ----------------------------------------------------------------------------------------
SrtmTile::SrtmTile()
{
  Invalidate();
}

SrtmTile::SrtmTile(SrtmTile && rhs) : m_data(move(rhs.m_data)), m_valid(rhs.m_valid)
{
  rhs.Invalidate();
}

void SrtmTile::Init(std::string const & dir, ms::LatLon const & coord)
{
  Invalidate();

  std::string const base = GetBase(coord);
  std::string const cont = dir + base + ".SRTMGL1.hgt.zip";
  std::string file = base + ".hgt";

  UnzipMemDelegate delegate(m_data);
  try
  {
    ZipFileReader::UnzipFile(cont, file, delegate);
  }
  catch (ZipFileReader::LocateZipException const & e)
  {
    // Sometimes packed file has different name. See N39E051 measure.
    file = base + ".SRTMGL1.hgt";

    ZipFileReader::UnzipFile(cont, file, delegate);
  }

  if (!delegate.m_completed)
  {
    LOG(LWARNING, ("Can't decompress SRTM file:", cont));
    Invalidate();
    return;
  }

  if (m_data.size() != kSrtmTileSize)
  {
    LOG(LWARNING, ("Bad decompressed SRTM file size:", cont, m_data.size()));
    Invalidate();
    return;
  }

  m_valid = true;
}

feature::TAltitude SrtmTile::GetHeight(ms::LatLon const & coord)
{
  if (!IsValid())
    return feature::kInvalidAltitude;

  double ln = coord.lon - static_cast<int>(coord.lon);
  if (ln < 0)
    ln += 1;
  double lt = coord.lat - static_cast<int>(coord.lat);
  if (lt < 0)
    lt += 1;
  lt = 1 - lt;  // from North to South

  size_t const row = kArcSecondsInDegree * lt;
  size_t const col = kArcSecondsInDegree * ln;

  size_t const ix = row * (kArcSecondsInDegree + 1) + col;

  if (ix >= Size())
    return feature::kInvalidAltitude;
  return ReverseByteOrder(Data()[ix]);
}

std::string SrtmTile::GetBase(ms::LatLon coord)
{
  std::ostringstream ss;
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
  ss << std::setw(2) << std::setfill('0') << static_cast<int>(coord.lat);

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
  ss << std::setw(3) << static_cast<int>(coord.lon);
  return ss.str();
}

void SrtmTile::Invalidate()
{
  m_data.clear();
  m_data.shrink_to_fit();
  m_valid = false;
}

// SrtmTileManager ---------------------------------------------------------------------------------
SrtmTileManager::SrtmTileManager(std::string const & dir) : m_dir(dir) {}
feature::TAltitude SrtmTileManager::GetHeight(ms::LatLon const & coord)
{
  std::string const base = SrtmTile::GetBase(coord);
  auto it = m_tiles.find(base);
  if (it == m_tiles.end())
  {
    SrtmTile tile;
    try
    {
      tile.Init(m_dir, coord);
    }
    catch (RootException const & e)
    {
      LOG(LINFO, ("Can't init SRTM tile:", base, "reason:", e.Msg()));
    }

    // It's OK to store even invalid tiles and return invalid height
    // for them later.
    it = m_tiles.emplace(base, std::move(tile)).first;
  }

  return it->second.GetHeight(coord);
}
}  // namespace generator
