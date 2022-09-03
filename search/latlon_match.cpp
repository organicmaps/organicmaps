#include "search/latlon_match.hpp"

#include <memory>
#include <string>
#include <optional>
#include <list>
#include <array>
#include <cstring>
#include <functional>

namespace {
// The Check class is an auxiliary class, the purpose of which is to perform
// various kinds of checks during the operation of the module.
class Check {
 public:
  static std::size_t IsDegree(char const * pos)
  {
    return CheckSymbol(m_degrees, pos);
  }
  static std::size_t IsMinute(char const * pos)
  {
    return CheckSymbol(m_minutes, pos);
  }
  static std::size_t IsSecond(char const * pos)
  {
    return CheckSymbol(m_seconds, pos);
  }
  // The IsCardDir function checks whether the character, pointed to by the |pos|,
  // is symbol of cardinal direction. If so, the function returns true,
  // otherwise false.
  static bool IsCardDir(char const * pos)
  {
    return m_cardinalDirections.find(*pos) != std::string_view::npos;
  }
  // The IsNegative function checks whether the character, pointed to by the |pos|,
  // is the beginning of a negative number. If so, the function returns true,
  // otherwise false.
  static bool IsNegative(char const * pos)
  {
    return *pos == '-' && std::isdigit(*(pos + 1));
  }
  // The IsInteger function checks whether the |value| has a fractional part.
  // If so, the function returns false, otherwise true.
  static bool IsInteger(double value)
  {
    return value == static_cast<int>(value);
  }
  // The Latitude function checks whether the |lat| variable meets the latitude
  // requirements. If so, the function returns true, otherwise false.
  static bool Latitude(double lat)
  {
    return (lat < -90.0 || lat > 90.0) ? false : true;
  }
  // The Longitude function checks whether the |lon| variable meets the longitude
  // requirements. If so, the function returns true, otherwise false.
  static bool Longitude(double lon)
  {
    return (lon < -180.0 || lon > 180.0) ? false : true;
  }
 private:
  template<std::size_t Size>
  using Array = std::array<char const *, Size>;
  // The CheckSymbol function checks whether character, pointed to by |pos|, belongs
  // to the set of characters, contained in the array |symbols|. The function returns
  // the size of the character, if it is present, otherwise zero.
  template<std::size_t Size>
  static std::size_t CheckSymbol(Array<Size> const & symbols, char const * pos)
  {
    for (char const * symb : symbols)
    {
      size_t const len = strlen(symb);
      if (strncmp(pos, symb, len) == 0)
        return len;
    }
    return 0;
  }
  static constexpr Array<2> m_degrees = { "*", "°" };
  static constexpr Array<3> m_minutes = { "\'", "’", "′" };
  static constexpr Array<6> m_seconds = { "\"", "”", "″", "\'\'", "’’", "′′" };
  static constexpr std::string_view m_cardinalDirections = "NnSsEeWw";
};

enum class Index
{
  kDegree,
  kMinute,
  kSecond,
  kCardDir
};

// The CoordinatesFormat class is an abstract class that provides an interface for
// interacting with its successors as well as functionality common to them. In order
// to be successfully integrated, each new coordinate format must inherit this class.
class CoordinatesFormat {
  using CoordinatePair = std::optional<std::pair<double, double>>;
 public:
  CoordinatesFormat() : m_start(nullptr) { }
  virtual ~CoordinatesFormat() = default;
  // The SetStart function sets the starting point in the query for coordinate parser.
  void SetStart(char const * start)
  {
    m_start = start;
  }
  // The InterpretNumber function interprets a floating point value in a byte string
  // pointed to by |start|. The function returns two values: floating point value,
  // corresponding to the contents of |start| and the number of characters that was
  // interpreted.
  std::pair<double, std::size_t> InterpretNumber(char const * const start)
  {
    char * end;
    double number = std::strtod(start, &end);
    if (*end == ',' && std::isdigit(*(end + 1)))
    {
      char* another_start = ++end;
      double fractional_part = std::strtod(another_start, &end);
      while (fractional_part >= 1)
        fractional_part /= 10;
      number += number > 0 ? fractional_part : -fractional_part;
    }
    return {number, end - start};
  }
  // The Parse function is pure virtual method, whose implementation in subclasses
  // performs query analysis.
  virtual void Parse() = 0;
  // The Format function is pure virtual method, whose implementation in subclasses
  // checks whether it is possible to analyze the byte string pointed to by the |pos|.
  // The function returns true if the byte string pointed to by the |pos| corresponds
  // to the characteristic features of the coordinate format, otherwise false.
  virtual bool Format(char const * pos) = 0;
  // The Results The function returns the results of the analysis in case of its
  // successful completion, otherwise uninitialized std::optional object.
  virtual CoordinatePair Results() = 0;
 protected:
  CoordinatePair Results(std::optional<double> lat, std::optional<double> lon)
  {
    if (lat && lon && Check::Latitude(lat.value()) && Check::Longitude(lon.value()))
      return std::make_pair(lat.value(), lon.value());
    else
      return std::nullopt;
  }
  char const * StartPos()
  {
    return m_start;
  }
 private:
  // The |m_start| field holds the position in the query with starting from which
  // the analysis is performed.
  char const * m_start;
};


// The DDFormat class provides functionality for parsing coordinates in Decimal
// Degrees (DD) format. The implemented format has the following form:
//
//  dd, DD
//
//  -> dd is a latitude in a range [-90,90] (real number);
//  -> DD is a longitude in a range [-180,180] or [0,360] (real number).
// The longitude ranges are correlated as follows:
// [   0, 360]:  180,  181, ..., 359, 360/0, 1, ..., 179, 180
// [-180, 180]: -180, -179, ...,  -1,     0, 1, ..., 179, 180
class DDFormat : public CoordinatesFormat {
 public:
  DDFormat() : m_lat(std::nullopt), m_lon(std::nullopt), m_in_process(&m_lat) { }
  void Parse() override
  {
    for (auto iter = StartPos(); *iter != '\0';)
    {
      std::size_t shift = 1;
      if (m_in_process)
      {
        if (std::isdigit(*iter) || Check::IsNegative(iter))
          std::tie(*m_in_process, shift) = InterpretNumber(iter);
        else if (*iter == ' ' || *iter == ',')
          m_in_process = m_lon ? nullptr : &m_lon;
        else if (std::size_t size = Check::IsDegree(iter); size != 0)
          shift = size;
        else if (!m_lat || !m_lon || Check::IsCardDir(iter) || Check::IsMinute(iter) || Check::IsSecond(iter))
          return Reset();
      } else if (std::isdigit(*iter))
        return Reset();
      iter += shift;
    }
    if (m_lon > 180.0)
      m_lon.value() -= 360.0;
  }
  std::optional<std::pair<double, double>> Results() override
  {
    return CoordinatesFormat::Results(m_lat, m_lon);
  }
  bool Format(char const * pos) override
  {
    return Check::IsNegative(pos) || std::isdigit(*pos);
  }
 private:
  void Reset()
  {
    std::tie(m_lat, m_lon) = std::tie(std::nullopt, std::nullopt);
  }
  std::optional<double> m_lat;  // The |m_lat| field holds the latitude value during parsing.
  std::optional<double> m_lon;  // The |m_lon| field holds the longitude value dirung parsing.
  std::optional<double> * m_in_process; // The |m_in_process| points to a coordinate that is
                                        // in the process of parsing.
};

// The AbstractDmsDdm class is the abstract base class, providing functionality common to
// parsers of Degrees, Minutes, Seconds (DMS) and Degrees, Decimal Minutes (DDM) coordinate
// formates.
class AbstractDmsDdm : public CoordinatesFormat {
 protected:
  // The Component class represents a separate component from which the DMS or DDM notations
  // are built.
  class Component {
   public:
    Component(Index index, int ratio, std::function<bool(double)>&& check)
        : m_index(index), m_ratio(ratio), m_check(check) { }
    virtual ~Component() = default;
    bool HasValue() const
    {
      return m_value.has_value();
    }
    bool SetValue(double value)
    {
      if (!m_check(value))
        return false;
      m_value = std::make_optional(value);
      return true;
    }
    double Compute() const
    {
      return m_value.value() / m_ratio;
    }
    bool operator==(Index const index) const
    {
      return index == m_index;
    }
   private:
    // The |m_index| field holds the identifier of the component that is used to address
    // this component during runtime.
    Index const m_index;
    // The |m_value| field holds the value of the component.
    std::optional<double> m_value;
    // The |m_ratio| field holds the value of the multiplier that is used in the calculation
    // of the coordinate.
    int const m_ratio;
    // The |m_check| field holds a functor which checks whether component corresponds to
    // a specific notation. If so, the functor returns true, otherwise false.
    std::function<bool(double)> const m_check;
  };
 private:
  // The Coordinate class builds individual instances of the Component class into
  // a structure typical of DMS or DDM notation.
  class Coordinate {
    using CompPtr = std::unique_ptr<Component>;
    using CompList = std::list<CompPtr>;
   public:
    template<typename...Ts>
    Coordinate(Ts... ts)
    {
      (m_list.push_back(std::make_unique<Ts>(ts)), ...);
    }
    template<typename T>
    bool SetValue(T value, Index index)
    {
      if constexpr (std::is_floating_point_v<T>) {
        if (auto iter = Find(index); iter != m_list.end())
        {
          Complete(iter);
          return (*iter)->SetValue(value);
        }
      } else
        card_dir = std::make_optional(value);
      return true;
    }
    // The HasValue function checks whether a component with the index |index| contains
    // value. The function returns true if the component contains a value, otherwise
    // false.
    bool HasValue(Index index)
    {
      if (auto iter = Find(index); iter != m_list.end())
        return (*iter)->HasValue();
      return card_dir.has_value();
    }
    bool IsLatitude() const
    {
      return card_dir == 'N' || card_dir == 'S';
    }
    bool IsLongitude() const
    {
      return card_dir == 'W' || card_dir == 'E';
    }
    // The Calculate function, basing on the information obtained during the parsing,
    // translates the coordinate into Decimal degrees format. The result is recorded
    // in the |m_result| field.
    void Calculate()
    {
      for (auto& ptr : m_list) {
        if (ptr->HasValue())
        {
          if (m_result)
            m_result.value() += m_result < 0 ? -ptr->Compute() : ptr->Compute();
          else
            m_result = ptr->Compute();
        }
      }
      if (m_result && (card_dir == 'S' || card_dir == 'W'))
        m_result.value() = -m_result.value();
    }
    std::optional<double> Result() const
    {
      return m_result;
    }
    // The |card_dir| field holds a symbol indicating the cardinal directions.
    std::optional<char> card_dir;
   private:
    // The Complete function verifies that the components preceding the |comp|
    // have a value. If the previous component does not have a value, then it is
    // assigned zero. The function stops when a component that has a value is
    // reached or if the first component of the coordinate is reached.
    void Complete(CompList::iterator comp)
    {
      if (comp != m_list.begin())
      {
        for (auto iter = std::prev(comp); !(*iter)->HasValue();)
        {
          (*iter)->SetValue(0);
          if (iter != m_list.begin())
            --iter;
        }
      }
    }
    // The Find function searches for a coordinate component with an index |index|.
    // If a component with the specified index is found, the function returns
    // an iterator to it, otherwise an iterator, pointing to the end of the |m_list|.
    CompList::iterator Find(Index const index)
    {
      auto predicate = [index] (CompPtr const & c) { return *c == index; };
      return std::find_if(m_list.begin(), m_list.end(), std::move(predicate));
    }
    // The |m_list| field holds instances of successors of the Component class.
    std::list<std::unique_ptr<Component>> m_list;
    std::optional<double> m_result;
  };
 public:
  template<typename... Ts>
  AbstractDmsDdm(Ts... components) : m_number(0), m_first(Coordinate(components...)),
      m_second(Coordinate(components...)), m_inProcess(&m_first) { }
  void Parse() override
  {
    char const * iter = StartPos();
    while (*iter != '\0') {
      std::size_t shift = 1;
      if (std::isdigit(*iter) || Check::IsNegative(iter))
        std::tie(m_number, shift) = InterpretNumber(iter);
      else if (*iter == ',' && *(iter + 1) == ' ' && m_inProcess == &m_first)
        m_inProcess = &m_second;
      else if ((*iter == ' ' || *iter == ',') && m_number)
        return;
      else if (Check::IsCardDir(iter)) {
        if (!AssignValue(std::toupper(*iter), Index::kCardDir))
          return;
      } else if (auto is_format = FormatCheck(iter); is_format) {
        Index index;
        std::tie(shift, index) = is_format.value();
        if (!AssignValue(m_number, index))
          return;
      } else if (Interrupt(iter))
        return;
      iter += shift;
    }
    if (m_number != 0)
      return;
    Process();
  }
  bool Format(char const * pos) override
  {
    return Check::IsCardDir(pos) || Check::IsNegative(pos) || std::isdigit(*pos);
  }
  std::optional<std::pair<double, double>> Results() override
  {
    return CoordinatesFormat::Results(m_first.Result(), m_second.Result());
  }
  void Process()
  {
    if (!m_first.card_dir && !m_second.card_dir) {
      m_first.card_dir = 'N';
      m_second.card_dir = 'E';
    }
    if (m_first.card_dir && m_second.card_dir) {
      if (m_first.IsLongitude() && m_second.IsLatitude())
        std::swap(m_first, m_second);
      if (m_first.IsLatitude() && m_second.IsLongitude()) {
        m_first.Calculate();
        m_second.Calculate();
      }
    }
  }
 private:
  // The AssignValue function template assigns value |value| to the component |index|
  // of the coordinate. The function returns true if the value was assigned successfully,
  // otherwise false.
  template<typename T>
  bool AssignValue(T value, Index index)
  {
    if (m_inProcess->HasValue(index))
    {
      if (m_inProcess == &m_first)
        m_inProcess = &m_second;
      else if (m_inProcess == &m_second) {
        if constexpr (std::is_integral_v<T>)
          return true;
        return false;
      }
    }
    if (!m_inProcess->SetValue(value, index))
      return false;
    if constexpr (std::is_floating_point_v<T>)
      m_number = 0;
    return true;
  }
  // The Interrupt function defines the conditions (if any) under which the parsing process
  // should be terminated. Parsing stops if the function returns true, otherwise the operation
  // continues.
  virtual bool Interrupt(char const * pos) = 0;
  // The FormatCheck function checks whether the character in the byte string pointed to by
  // the |pos| corresponds to characters of the DMS or DDM notations. The function returns
  // an uninitialized std::optional object if the symbol does not correspond to characters
  // of the DMS or DDM notations, otherwise the function returns the size of the symbol and
  // its index.
  virtual std::optional<std::pair<std::size_t, Index>> FormatCheck(char const * pos) = 0;
  // The |m_number| field holds the value of the coordinate component that is being processed.
  double m_number;
  // The |m_first| and |m_second| fields hold the information that is obtained during parsing.
  Coordinate m_first;
  Coordinate m_second;
  Coordinate* m_inProcess;  // The |m_in_process| field holds an address of the Coordinate
                            // object, to which the information is written.
};

// The DMSFormat class provides functionality for parsing coordinates in Degrees, Minutes,
// Seconds (DMS) format. The DMS format has the following form:
//
//  dd* MM' SS" N, DD* MM' SS" E
//
//  -> dd is in a range [0,90] and DD is in a range [0,180] (both whole numbers);
//  -> MM is in a range [0,59] (whole number);
//  -> SS is in a range [0,60) (real number).
class DMSFormat : public AbstractDmsDdm {
  struct Degree : Component {
    Degree() : Component(Index::kDegree, 1, [] (double v) { return v >= 0 && v <= 180 && Check::IsInteger(v); }) { }
  };
  struct Minute : Component {
    Minute() : Component(Index::kMinute, 60, [] (double v) { return v >= 0 && v <= 59 && Check::IsInteger(v); }) { }
  };
  struct Second : Component {
    Second() : Component(Index::kSecond, 3600, [] (double v) { return v >= 0 && v < 60; }) { }
  };
 public:
  DMSFormat() : AbstractDmsDdm(Degree(), Minute(), Second()) { }
 private:
  bool Interrupt(char const *) override
  {
    return false;
  }
  std::optional<std::pair<std::size_t, Index>> FormatCheck(char const * pos) override
  {
    if (std::size_t length = Check::IsDegree(pos); length)
      return std::make_pair(length, Index::kDegree);
    else if (std::size_t length = Check::IsSecond(pos); length)
      return std::make_pair(length, Index::kSecond);
    else if (std::size_t length = Check::IsMinute(pos); length)
      return std::make_pair(length, Index::kMinute);
    else
      return std::nullopt;
  }
};

// The DDMFormat class provides functionality for parsing coordinates in Degrees, Decimal
// Minutes (DMM) format. The DMM format has the following form:
//
//  dd* MM' N, DD* MM' E
//
//  -> dd is in a range [0,90] and DD is in a range [0,180] (both whole numbers);
//  -> MM is in a range [0,60) (real number).
class DDMFormat : public AbstractDmsDdm {
  struct Degree : Component {
    Degree() : Component(Index::kDegree, 1, [] (double v) { return v >= 0 && v <= 180 && Check::IsInteger(v); }) { }
  };
  struct Minute : Component {
    Minute() : Component(Index::kMinute, 60, [] (double v) { return v >= 0 && v <= 60; }) { }
  };
 public:
  DDMFormat() : AbstractDmsDdm(Degree(), Minute()) { }
 private:
  bool Interrupt(char const * pos) override
  {
    return Check::IsSecond(pos);
  }
  std::optional<std::pair<std::size_t, Index>> FormatCheck(char const * pos) override
  {
    if (std::size_t length = Check::IsDegree(pos); length)
      return std::make_pair(length, Index::kDegree);
    else if (std::size_t length = Check::IsMinute(pos); length)
      return std::make_pair(length, Index::kMinute);
    else
      return std::nullopt;
  }
};

// The Parser class selects the most appropriate coordinate format and parses the query
// according to it. A class, extending the list of supported coordinate formats, should
// be inherited from the CoordinateFormat class and added as an argument to the invocation
// of CreateList funtion template.
class Parser {
  using FormatPtr = std::unique_ptr<CoordinatesFormat>;
  using FormatsList = std::list<FormatPtr>;
  template<typename... Fs>
  FormatsList CreateList()
  {
    FormatsList lst;
    (lst.push_back(std::make_unique<Fs>()), ...);
    return lst;
  }
 public:
  Parser(std::string const & query) : m_pos(query.c_str()), m_parser(nullptr),
      m_formats(CreateList<DDFormat, DMSFormat, DDMFormat>()) { }
  bool AssumeFormat()
  {
    if (m_parser && m_parser->Results())
      return false;
    while (!m_formats.empty() && IsAllowed(m_pos))
    {
      for (auto iter = m_formats.begin(); iter != m_formats.end(); ++iter)
      {
        if ((*iter)->Format(m_pos))
        {
          m_parser = std::move(*iter);
          m_parser->SetStart(m_pos);
          m_formats.erase(iter);
          return true;
        }
      }
      ++m_pos;
    }
    return false;
  }
  void Run()
  {
    m_parser->Parse();
  }
  std::optional<std::pair<double, double>> Results() const
  {
    return m_parser ? m_parser->Results() : std::nullopt;
  }
 private:
  // The IsAllowed function checks whether the character pointed to by the |pos|
  // can be part of the coordinate. If so, the function returns true, otherwise false.
  static bool IsAllowed(char const * pos)
  {
    return std::ispunct(*pos) || std::isspace(*pos) || std::isdigit(*pos) || Check::IsCardDir(pos);
  }
  // The |m_pos| field holds an address of the beginning of the chunk of the query
  // that should be parsed.
  char const * m_pos;
  // The |m_parser| field holds a pointer to the coordinate parser, using at the moment.
  FormatPtr m_parser;
  // The variable holds a list of coordinate parsers, using for the query processing.
  FormatsList m_formats;
};

std::optional<std::pair<double, double>> MatchLatLonDegree(std::string const & query)
{
  Parser parser(query);
  while (parser.AssumeFormat())
    parser.Run();
  return parser.Results();
}
} // end of unnamed namespace

namespace search {
bool MatchLatLonDegree(std::string const & query, double & lat, double & lon)
{
  if (auto success = ::MatchLatLonDegree(query); success)
  {
    std::tie(lat, lon) = success.value();
    return true;
  }
  return false;
}
}
