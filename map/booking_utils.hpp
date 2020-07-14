#pragma once

#include "map/booking_filter_params.hpp"

#include "search/result.hpp"

#include <vector>

struct FeatureID;

class SearchMarks;

namespace booking
{
filter::TasksInternal MakeInternalTasks(filter::Tasks const & filterTasks,
                                        SearchMarks & searchMarks, bool inViewport);
filter::TasksRawInternal MakeInternalTasks(filter::Tasks const & filterTasks,
                                           SearchMarks & searchMarks);
}  // namespace booking
