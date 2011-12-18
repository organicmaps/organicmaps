#include "settings.hpp"

#include "../defines.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/any_rect2d.hpp"

#include "../platform/platform.hpp"

#include "../std/sstream.hpp"

#define DELIM_CHAR  '='

namespace Settings
{
  StringStorage::StringStorage()
  {
    try
    {
      string str;
      ReaderPtr<Reader>(GetPlatform().GetReader(SETTINGS_FILE_NAME)).ReadAsString(str);
      istringstream stream(str);
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
          m_values[key] = value;
      }
    }
    catch (std::exception const & e)
    {}
  }

  void StringStorage::Save() const
  {
    // @TODO add mutex
    FileWriter file(GetPlatform().WritablePathForFile(SETTINGS_FILE_NAME));
    for (ContainerT::const_iterator it = m_values.begin(); it != m_values.end(); ++it)
    {
      string line(it->first);
      line += DELIM_CHAR;
      line += it->second;
      line += "\n";
      file.Write(line.data(), line.size());
    }
  }

  StringStorage & StringStorage::Instance()
  {
    static StringStorage inst;
    return inst;
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

////////////////////////////////////////////////////////////////////////////////////////////

  template <> string ToString<string>(string const & str)
  {
    return str;
  }

  template <> bool FromString<string>(string const & strIn, string & strOut)
  {
    strOut = strIn;
    return true;
  }

  template <> string ToString<m2::AnyRectD>(m2::AnyRectD const & rect)
  {
    ostringstream out;
    out.precision(12);
    m2::PointD glbZero(rect.GlobalZero());
    out << glbZero.x << " " << glbZero.y << " ";
    out << rect.angle().val() << " ";
    m2::RectD r = rect.GetLocalRect();
    out << r.minX() << " " << r.minY() << " " << r.maxX() << " " << r.maxY();
    return out.str();
  }

  template <> bool FromString<m2::AnyRectD>(string const & str, m2::AnyRectD & rect)
  {
    istringstream in(str);
    double val[7];
    size_t count = 0;
    while (in.good() && count < 7)
      in >> val[count++];

    if (count != 7)
      return false;

    rect = m2::AnyRectD(m2::PointD(val[0], val[1]),
                       ang::AngleD(val[2]),
                       m2::RectD(val[3], val[4], val[5], val[6]));

    return true;
  }

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

  typedef std::pair<int, int> UPairT;
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

  template <> string ToString<Units>(Units const & v)
  {
    switch (v)
    {
    case Yard: return "Yard";
    case Foot: return "Foot";
    default: return "Metric";
    }
  }

  template <> bool FromString<Units>(string const & s, Units & v)
  {
    if (s == "Metric")
      v = Metric;
    else if (s == "Yard")
      v = Yard;
    else if (s == "Foot")
      v = Foot;
    else
      return false;

    return true;
  }
}
