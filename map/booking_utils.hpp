#pragma once

#include "map/booking_filter_params.hpp"

#include <string>
#include <vector>

struct FeatureID;

class SearchMarks;

namespace search
{
class Results;
}

namespace booking
{
filter::TasksInternal MakeInternalTasks(search::Results const & results,
                                        filter::Tasks const & filterTasks,
                                        SearchMarks & searchMarks, bool inViewport);
filter::TasksRawInternal MakeInternalTasks(std::vector<FeatureID> const & features,
                                           filter::Tasks const & filterTasks,
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
