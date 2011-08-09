#pragma once
#include "version.hpp"

#include "../coding/write_to_sink.hpp"

#include "../base/timer.hpp"


namespace ver
{
  template <class TSink> void WriteVersion(TSink & sink)
  {
    // static is used for equal time stamp for all "mwm" files in one generation process
    static uint32_t generatorStartTime = my::TodayAsYYMMDD();

    WriteToSink(sink, static_cast<uint32_t>(Version::BUILD));
    WriteToSink(sink, static_cast<uint32_t>(Version::GIT_HASH));

    // actual date of data generation
    WriteToSink(sink, generatorStartTime);
  }
}
