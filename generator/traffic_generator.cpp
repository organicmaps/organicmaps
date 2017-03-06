#include "generator/traffic_generator.hpp"

#include "routing/routing_helpers.hpp"

#include "traffic/traffic_info.hpp"

#include "routing_common/car_model.hpp"

#include "platform/mwm_traits.hpp"

#include "indexer/feature_algo.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/features_offsets_table.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "std/vector.hpp"

namespace traffic
{
bool GenerateTrafficKeysFromDataFile(string const & mwmPath)
{
  try
  {
    vector<TrafficInfo::RoadSegmentId> keys;
    TrafficInfo::ExtractTrafficKeys(mwmPath, keys);

    vector<uint8_t> buf;
    TrafficInfo::SerializeTrafficKeys(keys, buf);

    FilesContainerW writeContainer(mwmPath, FileWriter::OP_WRITE_EXISTING);
    FileWriter writer = writeContainer.GetWriter(TRAFFIC_KEYS_FILE_TAG);
    writer.Write(buf.data(), buf.size());
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Failed to build traffic keys:", e.Msg()));
    return false;
  }

  return true;
}
}  // namespace traffic
