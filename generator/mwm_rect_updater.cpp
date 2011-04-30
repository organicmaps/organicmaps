#include "mwm_rect_updater.hpp"
#include "borders_loader.hpp"

#include "../indexer/data_header.hpp"

#include "../coding/file_container.hpp"

#include "../base/logging.hpp"

#include "../defines.hpp"


namespace
{
  class DoUpdateRect
  {
    string const & m_basePath;
  public:
    DoUpdateRect(string const & path) : m_basePath(path) {}

    void operator() (m2::RectD const & r, borders::CountryPolygons const & c)
    {
      string name = m_basePath + c.m_name + DATA_FILE_EXTENSION;

      feature::DataHeader h;

      try
      {
        {
          FilesContainerR contR(name);
          h.Load(contR.GetReader(HEADER_FILE_TAG));
        }

        h.SetBounds(r);

        {
          FilesContainerW contW(name, FileWriter::OP_WRITE_EXISTING);
          FileWriter w = contW.GetExistingWriter(HEADER_FILE_TAG);
          h.Save(w);
        }
      }
      catch (FileReader::OpenException const &)
      {
        // country from countries list not found
      }
      catch (FileWriter::OpenException const &)
      {
        // something real bad happend
        LOG(LERROR, ("Can't open existing file for writing"));
      }
    }
  };
}

void UpdateMWMRectsFromBoundaries(string const & dataPath, int level)
{
  borders::CountriesContainerT countries;
  borders::LoadCountriesList(dataPath, countries, level);

  countries.ForEachWithRect(DoUpdateRect(dataPath));
}
