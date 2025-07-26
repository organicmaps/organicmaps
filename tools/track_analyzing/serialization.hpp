#pragma once

#include "track_analyzing/track.hpp"

#include "routing/segment.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "platform/country_file.hpp"

#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/traffic.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace track_analyzing
{
class MwmToMatchedTracksSerializer final
{
public:
  MwmToMatchedTracksSerializer(std::shared_ptr<routing::NumMwmIds> numMwmIds) : m_numMwmIds(std::move(numMwmIds)) {}

  template <typename Sink>
  void Serialize(MwmToMatchedTracks const & mwmToMatchedTracks, Sink & sink)
  {
    WriteSize(sink, mwmToMatchedTracks.size());

    for (auto const & mwmIt : mwmToMatchedTracks)
    {
      rw::Write(sink, m_numMwmIds->GetFile(mwmIt.first).GetName());

      UserToMatchedTracks const & userToMatchedTracks = mwmIt.second;
      CHECK(!userToMatchedTracks.empty(), ());
      WriteSize(sink, userToMatchedTracks.size());

      for (auto const & userIt : userToMatchedTracks)
      {
        rw::Write(sink, userIt.first);

        std::vector<MatchedTrack> const & tracks = userIt.second;
        CHECK(!tracks.empty(), ());
        WriteSize(sink, tracks.size());

        for (MatchedTrack const & track : tracks)
        {
          CHECK(!track.empty(), ());
          WriteSize(sink, track.size());

          std::vector<DataPoint> dataPoints;
          dataPoints.reserve(track.size());
          for (MatchedTrackPoint const & point : track)
          {
            Serialize(point.GetSegment(), sink);
            dataPoints.emplace_back(point.GetDataPoint());
          }

          std::vector<uint8_t> buffer;
          MemWriter<decltype(buffer)> memWriter(buffer);
          coding::TrafficGPSEncoder::SerializeDataPoints(coding::TrafficGPSEncoder::kLatestVersion, memWriter,
                                                         dataPoints);

          WriteSize(sink, buffer.size());
          sink.Write(buffer.data(), buffer.size());
        }
      }
    }
  }

  template <typename Source>
  void Deserialize(MwmToMatchedTracks & mwmToMatchedTracks, Source & src)
  {
    mwmToMatchedTracks.clear();

    auto const numMmws = ReadSize(src);
    for (size_t iMwm = 0; iMwm < numMmws; ++iMwm)
    {
      std::string mwmName;
      rw::Read(src, mwmName);

      auto const mwmId = m_numMwmIds->GetId(platform::CountryFile(mwmName));
      UserToMatchedTracks & userToMatchedTracks = mwmToMatchedTracks[mwmId];

      auto const numUsers = ReadSize(src);
      CHECK_NOT_EQUAL(numUsers, 0, ());
      for (size_t iUser = 0; iUser < numUsers; ++iUser)
      {
        std::string user;
        rw::Read(src, user);

        std::vector<MatchedTrack> & tracks = userToMatchedTracks[user];
        auto const numTracks = ReadSize(src);
        CHECK_NOT_EQUAL(numTracks, 0, ());
        tracks.resize(numTracks);

        for (size_t iTrack = 0; iTrack < numTracks; ++iTrack)
        {
          auto const numSegments = ReadSize(src);
          CHECK_NOT_EQUAL(numSegments, 0, ());
          std::vector<routing::Segment> segments;
          segments.resize(numSegments);

          for (size_t i = 0; i < numSegments; ++i)
            Deserialize(mwmId, segments[i], src);

          std::vector<uint8_t> buffer;
          auto const bufferSize = ReadSize(src);
          buffer.resize(bufferSize);
          src.Read(buffer.data(), bufferSize);

          MemReader memReader(buffer.data(), bufferSize);
          ReaderSource<MemReader> memSrc(memReader);

          std::vector<DataPoint> dataPoints;
          coding::TrafficGPSEncoder::DeserializeDataPoints(coding::TrafficGPSEncoder::kLatestVersion, memSrc,
                                                           dataPoints);
          CHECK_EQUAL(numSegments, dataPoints.size(), ("mwm:", mwmName, "user:", user));

          MatchedTrack & track = tracks[iTrack];
          track.reserve(numSegments);

          for (size_t i = 0; i < numSegments; ++i)
            track.emplace_back(dataPoints[i], segments[i]);
        }
      }
    }
  }

private:
  static uint8_t constexpr kForward = 0;
  static uint8_t constexpr kBackward = 1;

  template <typename Sink>
  static void WriteSize(Sink & sink, size_t size)
  {
    WriteVarUint(sink, base::checked_cast<uint64_t>(size));
  }

  template <typename Source>
  static size_t ReadSize(Source & src)
  {
    return base::checked_cast<size_t>(ReadVarUint<uint64_t>(src));
  }

  template <typename Sink>
  static void Serialize(routing::Segment const & segment, Sink & sink)
  {
    WriteToSink(sink, segment.GetFeatureId());
    WriteToSink(sink, segment.GetSegmentIdx());
    auto const direction = segment.IsForward() ? kForward : kBackward;
    WriteToSink(sink, direction);
  }

  template <typename Source>
  static void Deserialize(routing::NumMwmId numMwmId, routing::Segment & segment, Source & src)
  {
    auto const featureId = ReadPrimitiveFromSource<uint32_t>(src);
    auto const segmentIdx = ReadPrimitiveFromSource<uint32_t>(src);
    auto const direction = ReadPrimitiveFromSource<uint8_t>(src);
    segment = {numMwmId, featureId, segmentIdx, direction == kForward};
  }

  std::shared_ptr<routing::NumMwmIds> m_numMwmIds;
};
}  // namespace track_analyzing
