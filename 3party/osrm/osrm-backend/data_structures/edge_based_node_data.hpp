#pragma once

#include <stdint.h>
#include <vector>
#include <fstream>


namespace osrm
{

struct NodeData
{
#pragma pack (push, 1)
  struct SegmentInfo
  {
    uint64_t wayId;
    double lat1, lon1, lat2, lon2;

    SegmentInfo()
      : wayId(-1), lat1(-10000), lon1(-10000), lat2(-10000), lon2(-10000)
    {
    }

    SegmentInfo(uint64_t wayId, double lat1, double lon1, double lat2, double lon2)
      : wayId(wayId), lat1(lat1), lon1(lon1), lat2(lat2), lon2(lon2)
    {
    }

    bool operator != (SegmentInfo const & other) const
    {
      return wayId != other.wayId || lat1 != other.lat1 || lon1 != other.lon1 ||
          lat2 != other.lat2 || lon2 != other.lon2;
    }
  };
#pragma pack (pop)

  typedef std::vector<SegmentInfo> SegmentInfoVectorT;
  SegmentInfoVectorT m_segments;

  NodeData()
  {
  }

  NodeData(SegmentInfoVectorT & vec)
  {
    m_segments.swap(vec);
  }

  bool operator == (NodeData const & other) const
  {
    if (m_segments.size() != other.m_segments.size())
      return false;

    for (uint32_t i = 0; i < m_segments.size(); ++i)
      if (m_segments[i] != other.m_segments[i])
        return false;

    return true;
  }

  bool operator != (NodeData const & other) const
  {
    return !(*this == other);
  }

  void AddSegment(uint64_t wayId, double lat1, double lon1, double lat2, double lon2)
  {
    m_segments.emplace_back(wayId, lat1, lon1, lat2, lon2);
  }

  void SetSegments(SegmentInfoVectorT & segments)
  {
    m_segments.swap(segments);
  }
};


typedef std::vector<NodeData> NodeDataVectorT;

inline bool SaveNodeDataToFile(std::string const & filename, NodeDataVectorT const & data)
{
  std::ofstream stream;
  stream.open(filename);
  if (!stream.is_open())
    return false;

  uint32_t count = static_cast<uint32_t>(data.size());
  stream.write((char*)&count, sizeof(count));
  for (auto d : data)
  {
    uint32_t pc = static_cast<uint32_t>(d.m_segments.size());
    stream.write((char*)&pc, sizeof(pc));
    stream.write((char*)d.m_segments.data(), sizeof(NodeData::SegmentInfo) * pc);
  }
  stream.close();
  return true;
}

inline bool LoadNodeDataFromFile(std::string const & filename, NodeDataVectorT & data)
{
  std::ifstream stream;
  stream.open(filename);
  if (!stream.is_open())
    return false;

  uint32_t count = 0;
  stream.read((char*)&count, sizeof(count));
  for (uint32_t i = 0; i < count; ++i)
  {
    uint32_t pc;
    stream.read((char*)&pc, sizeof(pc));
    NodeData::SegmentInfoVectorT segments;
    segments.resize(pc);
    stream.read((char*)segments.data(), sizeof(NodeData::SegmentInfo) * pc);

    data.emplace_back(segments);
  }
  stream.close();

  return true;
}

}
