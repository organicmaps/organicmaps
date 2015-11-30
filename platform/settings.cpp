#include "settings.hpp"
#include "platform.hpp"

#include "defines.hpp"
#include "location.hpp"

#include "coding/reader_streambuf.hpp"
#include "coding/file_writer.hpp"
#include "coding/file_reader.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/any_rect2d.hpp"

#include "base/logging.hpp"

#include "std/cmath.hpp"
#include "std/iostream.hpp"
#include "std/sstream.hpp"


#define FIRST_LAUNCH_KEY "FirstLaunchOnDate"

static char const DELIM_CHAR = '=';

namespace Settings
{
  StringStorage::StringStorage()
  {
    lock_guard<mutex> guard(m_mutex);

    try
    {
      ReaderStreamBuf buffer(new FileReader(GetPlatform().SettingsPathForFile(SETTINGS_FILE_NAME)));
      istream stream(&buffer);

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
    catch (RootException const & ex)
    {
      LOG(LWARNING, (ex.Msg()));
    }
  }

  void StringStorage::Save() const
  {
    try
    {
      FileWriter file(GetPlatform().SettingsPathForFile(SETTINGS_FILE_NAME));
      for (auto const & value : m_values)
      {
        string line(value.first);
        line += DELIM_CHAR;
        line += value.second;
        line += "\n";
        file.Write(line.data(), line.size());
      }
    }
    catch (RootException const & ex)
    {
      // Ignore all settings saving exceptions.
      LOG(LWARNING, (ex.Msg()));
    }
  }

  StringStorage & StringStorage::Instance()
  {
    static StringStorage inst;
    return inst;
  }

  bool StringStorage::GetValue(string const & key, string & outValue) const
  {
    lock_guard<mutex> guard(m_mutex);

    auto const found = m_values.find(key);
    if (found == m_values.end())
      return false;

    outValue = found->second;
    return true;
  }

  void StringStorage::SetValue(string const & key, string && value)
  {
    lock_guard<mutex> guard(m_mutex);

    m_values[key] = move(value);
    Save();
  }

  void StringStorage::DeleteKeyAndValue(string const & key)
  {
    lock_guard<mutex> guard(m_mutex);

    auto const found = m_values.find(key);
    if (found != m_values.end())
    {
      m_values.erase(found);
      Save();
    }
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

  namespace impl
  {
    template <class T, size_t N> bool FromStringArray(string const & s, T (&arr)[N])
    {
      istringstream in(s);
      size_t count = 0;
      while (in.good() && count < N)
      {
        in >> arr[count];
        if (!std::isfinite(arr[count]))
          return false;
        ++count;
      }

      return (!in.fail() && count == N);
    }
  }

  template <> string ToString<m2::AnyRectD>(m2::AnyRectD const & rect)
  {
    ostringstream out;
    out.precision(12);
    m2::PointD glbZero(rect.GlobalZero());
    out << glbZero.x << " " << glbZero.y << " ";
    out << rect.Angle().val() << " ";
    m2::RectD const & r = rect.GetLocalRect();
    out << r.minX() << " " << r.minY() << " " << r.maxX() << " " << r.maxY();
    return out.str();
  }

  template <> bool FromString<m2::AnyRectD>(string const & str, m2::AnyRectD & rect)
  {
    double val[7];
    if (!impl::FromStringArray(str, val))
      return false;

    // Will get an assertion in DEBUG and false return in RELEASE.
    m2::RectD const r(val[3], val[4], val[5], val[6]);
    if (!r.IsValid())
      return false;

    rect = m2::AnyRectD(m2::PointD(val[0], val[1]), ang::AngleD(val[2]), r);
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
    double val[4];
    if (!impl::FromStringArray(str, val))
      return false;

    // Will get an assertion in DEBUG and false return in RELEASE.
    rect = m2::RectD(val[0], val[1], val[2], val[3]);
    return rect.IsValid();
  }

  template <> string ToString<bool>(bool const & v)
  {
    return v ? "true" : "false";
  }
  template <> bool FromString<bool>(string const & str, bool & v)
  {
    if (str == "true")
      v = true;
    else if (str == "false")
      v = false;
    else
      return false;
    return true;
  }

  namespace impl
  {
    template <typename T>
    string ToStringScalar(T const & v)
    {
      ostringstream stream;
      stream.precision(12);
      stream << v;
      return stream.str();
    }

    template <typename T>
    bool FromStringScalar(string const & str, T & v)
    {
      istringstream stream(str);
      if (stream.good())
      {
        stream >> v;
        return !stream.fail();
      }
      else return false;
    }
  }

  template <> string ToString<double>(double const & v)
  {
    return impl::ToStringScalar<double>(v);
  }

  template <> bool FromString<double>(string const & str, double & v)
  {
    return impl::FromStringScalar<double>(str, v);
  }

  template <> string ToString<int32_t>(int32_t const & v)
  {
    return impl::ToStringScalar<int32_t>(v);
  }

  template <> bool FromString<int32_t>(string const & str, int32_t & v)
  {
    return impl::FromStringScalar<int32_t>(str, v);
  }

  template <> string ToString<int64_t>(int64_t const & v)
  {
    return impl::ToStringScalar<int64_t>(v);
  }

  template <> bool FromString<int64_t>(string const & str, int64_t & v)
  {
    return impl::FromStringScalar<int64_t>(str, v);
  }

  template <> string ToString<uint32_t>(uint32_t const & v)
  {
    return impl::ToStringScalar<uint32_t>(v);
  }

  template <> bool FromString<uint32_t>(string const & str, uint32_t & v)
  {
    return impl::FromStringScalar<uint32_t>(str, v);
  }

  namespace impl
  {
    template <class TPair> string ToStringPair(TPair const & value)
    {
      ostringstream stream;
      stream.precision(12);
      stream << value.first << " " << value.second;
      return stream.str();
    }
    template <class TPair> bool FromStringPair(string const & str, TPair & value)
    {
      istringstream stream(str);
      if (stream.good())
      {
        stream >> value.first;
        if (stream.good())
        {
          stream >> value.second;
          return !stream.fail();
        }
      }
      return false;
    }
  }

  typedef pair<int, int> IPairT;
  typedef pair<double, double> DPairT;

  template <> string ToString<IPairT>(IPairT const & v)
  {
    return impl::ToStringPair(v);
  }
  template <> bool FromString<IPairT>(string const & s, IPairT & v)
  {
    return impl::FromStringPair(s, v);
  }

  template <> string ToString<DPairT>(DPairT const & v)
  {
    return impl::ToStringPair(v);
  }
  template <> bool FromString<DPairT>(string const & s, DPairT & v)
  {
    return impl::FromStringPair(s, v);
  }

  template <> string ToString<Units>(Units const & v)
  {
    switch (v)
    {
    case Foot: return "Foot";
    default: return "Metric";
    }
  }

  template <> bool FromString<Units>(string const & s, Units & v)
  {
    if (s == "Metric")
      v = Metric;
    else if (s == "Foot")
      v = Foot;
    else
      return false;

    return true;
  }

  template <> string ToString<location::EMyPositionMode>(location::EMyPositionMode const & v)
  {
    switch (v)
    {
    case location::MODE_UNKNOWN_POSITION: return "Unknown";
    case location::MODE_PENDING_POSITION: return "Pending";
    case location::MODE_NOT_FOLLOW: return "NotFollow";
    case location::MODE_FOLLOW: return "Follow";
    case location::MODE_ROTATE_AND_FOLLOW: return "RotateAndFollow";
    default: return "Unknown";
    }
  }

  template <> bool FromString<location::EMyPositionMode>(string const & s, location::EMyPositionMode & v)
  {
    if (s == "Unknown")
      v = location::MODE_UNKNOWN_POSITION;
    else if (s == "Pending")
      v = location::MODE_PENDING_POSITION;
    else if (s == "NotFollow")
      v = location::MODE_NOT_FOLLOW;
    else if (s == "Follow")
      v = location::MODE_FOLLOW;
    else if (s == "RotateAndFollow")
      v = location::MODE_ROTATE_AND_FOLLOW;
    else
      return false;

    return true;
  }

  bool IsFirstLaunchForDate(int date)
  {
    int savedDate;
    if (!Get(FIRST_LAUNCH_KEY, savedDate) || savedDate < date)
    {
      Set(FIRST_LAUNCH_KEY, date);
      return true;
    }
    else
      return false;
  }
}
