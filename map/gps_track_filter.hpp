#pragma once

#include "platform/location.hpp"

#include "std/vector.hpp"

class GpsTrackFilter
{
public:
  void Process(vector<location::GpsInfo> const & inPoints,
               vector<location::GpsTrackInfo> & outPoints);
};
