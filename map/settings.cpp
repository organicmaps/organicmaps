#include "settings.hpp"

#include "../defines.hpp"

#include "../base/logging.hpp"

#include "../geometry/rect2d.hpp"

#include "../platform/platform.hpp"

#include "../std/fstream.hpp"
#include "../std/sstream.hpp"

#define DELIM_CHAR  '='

namespace Settings
{
  StringStorage::StringStorage()
  {
    ifstream stream(GetPlatform().WritablePathForFile(SETTINGS_FILE_NAME).c_str());
    string line;

    while (stream.good())
    {
      std::getline(stream, line);
      if (line.empty())
        continue;

      size_t const delimPos = line.find(DELIM_CHAR);
      if (delimPos == string::npos)
        continue;

      string const key = line.substr(0, delimPos);
      string const value = line.substr(delimPos + 1);
      if (!key.empty() && !value.empty())
      {
        m_values[key] = value;
        LOG(LINFO, ("Loaded settings:", key, value));
      }
    }
  }

  void StringStorage::Save() const
  {
    ofstream stream(GetPlatform().WritablePathForFile(SETTINGS_FILE_NAME).c_str());
    for (ContainerT::const_iterator it = m_values.begin(); it != m_values.end(); ++it)
      stream << it->first << DELIM_CHAR << it->second << std::endl;
  }

  StringStorage & StringStorage::Instance()
  {
    return m_instance;
  }

  bool StringStorage::GetValue(string const & key, string & outValue)
  {
    ContainerT::const_iterator found = m_values.find(key);
    if (found == m_values.end())
      return false;
    outValue = found->second;
    return true;
  }

  void StringStorage::SetValue(string const & key, string const & value)
  {
    m_values[key] = value;
    Save();
  }

  StringStorage StringStorage::m_instance;
////////////////////////////////////////////////////////////////////////////////////////////

  template <> string ToString<m2::RectD>(m2::RectD const & rect)
  {
    ostringstream stream;
    stream.precision(12);
    stream << rect.minX() << " " << rect.minY() << " " << rect.maxX() << " " << rect.maxY();
    return stream.str();
  }
  template <> bool FromString<m2::RectD>(string const & str, m2::RectD & rect)
  {
    istringstream stream(str);
    m2::RectD::value_type values[4];
    size_t count = 0;
    while (stream.good() && count < 4)
      stream >> values[count++];

    if (count != 4)
      return false;

    rect.setMinX(values[0]);
    rect.setMinY(values[1]);
    rect.setMaxX(values[2]);
    rect.setMaxY(values[3]);
    return true;
  }

  template <> string ToString(bool const & value)
  {
    return value ? "true" : "false";
  }
  template <> bool FromString(string const & str, bool & outValue)
  {
    if (str == "true")
      outValue = true;
    else if (str == "false")
      outValue = false;
    else
      return false;
    return true;
  }

  typedef std::pair<uint32_t, uint32_t> UPairT;
  template <> string ToString<UPairT>(UPairT const & value)
  {
    ostringstream stream;
    stream.precision(12);
    stream << value.first << " " << value.second;
    return stream.str();
  }
  template <> bool FromString<UPairT>(string const & str, UPairT & value)
  {
    istringstream stream(str);
    if (stream.good())
    {
      stream >> value.first;
      if (stream.good())
      {
        stream >> value.second;
        return true;
      }
    }
    return false;
  }
}
