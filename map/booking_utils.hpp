#pragma once

#include "map/booking_filter_params.hpp"

#include <string>

struct FeatureID;

class SearchMarks;

namespace booking
{
filter::TasksInternal MakeInternalTasks(filter::Tasks const & filterTasks,
                                        SearchMarks & searchMarks, bool inViewport);
filter::TasksRawInternal MakeInternalTasks(filter::Tasks const & filterTasks,
                                           SearchMarks & searchMarks);
class PriceFormatter
{
public:
  std::string Format(double price, std::string const & currency);

private:
  std::string m_currencyCode;
  std::string m_currencySymbol;
};
}  // namespace booking
