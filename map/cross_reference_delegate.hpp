#pragma once

#include "partners_api/cross_reference_api.hpp"

#include "geometry/point2d.hpp"

#include <string>

class DataSource;

namespace search
{
class CityFinder;
}

class CrossReferenceDelegate : public cross_reference::Api::Delegate
{
public:
  CrossReferenceDelegate(DataSource const & dataSource, search::CityFinder & cityFinder);

  std::string GetCityOsmId(m2::PointD const & point) override;

private:
  DataSource const & m_dataSource;
  search::CityFinder & m_cityFinder;
};
