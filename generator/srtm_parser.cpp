#include "generator/srtm_parser.hpp"

#include "platform/platform.hpp"

#include "coding/endianness.hpp"
#include "coding/zip_reader.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <fstream>
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
  explicit UnzipMemDelegate(std::string & buffer) : m_buffer(buffer), m_completed(false) {}

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

std::string GetSrtmContFileName(std::string const & dir, std::string const & base)
{
  return base::JoinPath(dir, base + ".SRTMGL1.hgt.zip");
}
}  // namespace

// SrtmTile ----------------------------------------------------------------------------------------
SrtmTile::SrtmTile()
{
  Invalidate();
}

SrtmTile::SrtmTile(SrtmTile && rhs) : m_data(std::move(rhs.m_data)), m_valid(rhs.m_valid)
{
  rhs.Invalidate();
}

void SrtmTile::Init(std::string const & dir, ms::LatLon const & coord)
{
  Invalidate();

  std::string const base = GetBase(coord);
  std::string const cont = GetSrtmContFileName(dir, base);
  std::string file = base + ".hgt";

  // Original files are stored in zip archives. Alternatively, they can be loaded
  // from uncompressed files like "N34E012.hgt".
  static bool const loadFromZip = static_cast<bool>(std::ifstream(cont));
  if (loadFromZip)
  {
    UnzipMemDelegate delegate(m_data);
    try
    {
      ZipFileReader::UnzipFile(cont, file, delegate);
    }
    catch (ZipFileReader::LocateZipException const &)
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
  }
  else
  {
    GetPlatform().GetReader(file)->ReadAsString(m_data);
  }

  if (m_data.size() != kSrtmTileSize)
  {
    LOG(LWARNING, ("Bad decompressed SRTM file size:", cont, m_data.size()));
    Invalidate();
    return;
  }

  m_valid = true;
}

geometry::Altitude SrtmTile::GetHeight(ms::LatLon const & coord) const
{
  if (!IsValid())
    return geometry::kInvalidAltitude;

  double ln = coord.m_lon - static_cast<int>(coord.m_lon);
  if (ln < 0)
    ln += 1;
  double lt = coord.m_lat - static_cast<int>(coord.m_lat);
  if (lt < 0)
    lt += 1;
  lt = 1 - lt;  // from North to South

  auto const row = static_cast<size_t>(std::round(kArcSecondsInDegree * lt));
  auto const col = static_cast<size_t>(std::round(kArcSecondsInDegree * ln));

  size_t const ix = row * (kArcSecondsInDegree + 1) + col;

  CHECK_LESS(ix, Size(), (coord));
  return ReverseByteOrder(Data()[ix]);
}

// static
std::string SrtmTile::GetPath(std::string const & dir, std::string const & base)
{
  return GetSrtmContFileName(dir, base);
}

// static
ms::LatLon SrtmTile::GetCenter(ms::LatLon const & coord)
{
  return {floor(coord.m_lat) + 0.5, floor(coord.m_lon) + 0.5};
}

// static
std::string SrtmTile::GetBase(ms::LatLon const & coord)
{
  auto center = GetCenter(coord);
  std::ostringstream ss;
  if (center.m_lat < 0)
  {
    ss << "S";
    center.m_lat *= -1;
    center.m_lat += 1;
  }
  else
  {
    ss << "N";
  }
  ss << std::setw(2) << std::setfill('0') << static_cast<int>(center.m_lat);

  if (center.m_lon < 0)
  {
    ss << "W";
    center.m_lon *= -1;
    center.m_lon += 1;
  }
  else
  {
    ss << "E";
  }
  ss << std::setw(3) << static_cast<int>(center.m_lon);
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
geometry::Altitude SrtmTileManager::GetHeight(ms::LatLon const & coord)
{
  auto const key = GetKey(coord);

  auto it = m_tiles.find(key);
  if (it == m_tiles.end())
  {
    SrtmTile tile;
    try
    {
      tile.Init(m_dir, coord);
    }
    catch (RootException const & e)
    {
      std::string const base = SrtmTile::GetBase(coord);
      LOG(LINFO, ("Can't init SRTM tile:", base, "reason:", e.Msg()));
    }

    // It's OK to store even invalid tiles and return invalid height
    // for them later.
    it = m_tiles.emplace(key, std::move(tile)).first;
  }

  return it->second.GetHeight(coord);
}

// static
SrtmTileManager::LatLonKey SrtmTileManager::GetKey(ms::LatLon const & coord)
{
  auto const tileCenter = SrtmTile::GetCenter(coord);
  return {static_cast<int32_t>(tileCenter.m_lat), static_cast<int32_t>(tileCenter.m_lon)};
}

SrtmTile const & SrtmTileManager::GetTile(ms::LatLon const & coord)
{
  // Touch the tile to force its loading.
  GetHeight(coord);
  auto const key = GetKey(coord);
  auto const it = m_tiles.find(key);
  CHECK(it != m_tiles.end(), (coord));
  return it->second;
}
}  // namespace generator
