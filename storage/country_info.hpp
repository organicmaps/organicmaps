#pragma once
#include "country_polygon.hpp"

#include "../coding/file_container.hpp"


namespace storage
{
  class CountryInfoGetter
  {
    FilesContainerR m_reader;
    vector<CountryDef> m_countries;

    template <class ToDo>
    void ForEachCountry(m2::PointD const & pt, ToDo & toDo);

    class GetByPoint
    {
      CountryInfoGetter const & m_info;
      m2::PointD const & m_pt;

    public:
      size_t m_res;

      GetByPoint(CountryInfoGetter & info, m2::PointD const & pt)
        : m_info(info), m_pt(pt), m_res(-1) {}
      bool operator() (size_t id);
    };

  public:
    CountryInfoGetter(ModelReaderPtr reader);

    string GetRegionName(m2::PointD const & pt);
  };
}
