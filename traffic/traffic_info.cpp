#include "traffic/traffic_info.hpp"

#include "platform/http_client.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"

#include "defines.hpp"

#include "private.h"

namespace traffic
{
namespace
{
bool ReadRemoteFile(string const & url, string & result, int & errorCode)
{
  platform::HttpClient request(url);
  if (!request.RunHttpRequest())
  {
    LOG(LINFO, ("Couldn't run traffic request", url));
    errorCode = -1;
    return false;
  }

  errorCode = request.ErrorCode();
  result = request.ServerResponse();

  if (errorCode != 200)
  {
    LOG(LINFO, ("Traffic request", url, "failed. HTTP Error:", errorCode));
    return false;
  }

  return true;
}

string MakeRemoteURL(string const & name, uint64_t version)
{
  if (string(TRAFFIC_DATA_BASE_URL).empty())
    return {};

  stringstream ss;
  ss << TRAFFIC_DATA_BASE_URL;
  if (version != 0)
    ss << version << "/";
  ss << name << TRAFFIC_FILE_EXTENSION;
  return ss.str();
}
}  // namespace

// TrafficInfo::RoadSegmentId -----------------------------------------------------------------
TrafficInfo::RoadSegmentId::RoadSegmentId() : m_fid(0), m_idx(0), m_dir(0) {}

TrafficInfo::RoadSegmentId::RoadSegmentId(uint32_t fid, uint16_t idx, uint8_t dir)
  : m_fid(fid), m_idx(idx), m_dir(dir)
{
}

// TrafficInfo --------------------------------------------------------------------------------
TrafficInfo::TrafficInfo(MwmSet::MwmId const & mwmId, int64_t const & currentDataVersion)
  : m_mwmId(mwmId)
  , m_currentDataVersion(currentDataVersion)
{}

bool TrafficInfo::ReceiveTrafficData()
{
  auto const & info = m_mwmId.GetInfo();
  if (!info)
    return false;

  string const url = MakeRemoteURL(info->GetCountryName(), info->GetVersion());

  if (url.empty())
    return false;

  string result;
  int errorCode;
  if (!ReadRemoteFile(url, result, errorCode))
  {
    if (errorCode == 404)
    {
      int64_t version = atoi(result.c_str());

      if (version > info->GetVersion() && version <= m_currentDataVersion)
        m_availabilityStatus = Availability::ExpiredMwm;
      else if (version > m_currentDataVersion)
        m_availabilityStatus = Availability::ExpiredApp;
      else
        m_availabilityStatus = Availability::NoData;
    }
    else
    {
      m_availabilityStatus = Availability::Unknown;
    }
    return false;
  }

  vector<uint8_t> contents;
  contents.resize(result.size());
  for (size_t i = 0; i < result.size(); ++i)
    contents[i] = static_cast<uint8_t>(result[i]);

  Coloring coloring;
  try
  {
    DeserializeTrafficData(contents, coloring);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LINFO, ("Could not read traffic data received from server. MWM:", info->GetCountryName(),
                "Version:", info->GetVersion()));
    return false;
  }
  m_coloring.swap(coloring);
  m_availabilityStatus = Availability::IsAvailable;
  return true;
}

SpeedGroup TrafficInfo::GetSpeedGroup(RoadSegmentId const & id) const
{
  auto const it = m_coloring.find(id);
  if (it == m_coloring.cend())
    return SpeedGroup::Unknown;
  return it->second;
}

// static
void TrafficInfo::SerializeTrafficData(Coloring const & coloring, vector<uint8_t> & result)
{
  MemWriter<vector<uint8_t>> memWriter(result);
  WriteToSink(memWriter, static_cast<uint32_t>(coloring.size()));
  for (auto const & p : coloring)
  {
    WriteVarUint(memWriter, p.first.m_fid);
    uint16_t const x = (p.first.m_idx << 1) | p.first.m_dir;
    WriteVarUint(memWriter, x);
  }
  {
    BitWriter<decltype(memWriter)> bitWriter(memWriter);
    for (auto const & p : coloring)
    {
      // SpeedGroup's values fit into 3 bits.
      bitWriter.Write(static_cast<uint8_t>(p.second), 3);
    }
  }
}

// static
void TrafficInfo::DeserializeTrafficData(vector<uint8_t> const & data, Coloring & coloring)
{
  MemReader memReader(data.data(), data.size());
  ReaderSource<decltype(memReader)> src(memReader);
  auto const n = ReadPrimitiveFromSource<uint32_t>(src);

  vector<RoadSegmentId> keys(n);
  for (size_t i = 0; i < static_cast<size_t>(n); ++i)
  {
    keys[i].m_fid = ReadVarUint<uint32_t>(src);
    auto const x = ReadVarUint<uint32_t>(src);
    keys[i].m_idx = static_cast<uint16_t>(x >> 1);
    keys[i].m_dir = static_cast<uint8_t>(x & 1);
  }

  vector<SpeedGroup> values(n);
  BitReader<decltype(src)> bitReader(src);
  for (size_t i = 0; i < static_cast<size_t>(n); ++i)
  {
    // SpeedGroup's values fit into 3 bits.
    values[i] = static_cast<SpeedGroup>(bitReader.Read(3));
  }

  coloring.clear();
  for (size_t i = 0; i < static_cast<size_t>(n); ++i)
    coloring[keys[i]] = values[i];

  ASSERT_EQUAL(src.Size(), 0, ());
}
}  // namespace traffic
