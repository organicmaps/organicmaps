#pragma once

#include "map/booking_filter_params.hpp"

class SearchMarks;

namespace booking
{
filter::TasksInternal MakeInternalTasks(filter::Tasks const & filterTasks,
                                        SearchMarks & searchMarks, bool inViewport);
filter::TasksRawInternal MakeInternalTasks(filter::Tasks const & filterTasks,
                                           SearchMarks & searchMarks);
std::string FormatPrice(double price, std::string const & currency);
}  // namespace booking
