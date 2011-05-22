#include "../base/SRC_FIRST.hpp"

#include "drawing_rules.hpp"
#include "file_reader_stream.hpp"
#include "file_writer_stream.hpp"
#include "scales.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"

#include "../base/std_serialization.hpp"
#include "../base/assert.hpp"
#include "../base/macros.hpp"
#include "../base/string_utils.hpp"

#include "../std/bind.hpp"
#include "../std/algorithm.hpp"
#include "../std/tuple.hpp"
#include "../std/fstream.hpp"
#include "../std/exception.hpp"
#include "../std/limits.hpp"

#include "../base/start_mem_debug.hpp"


namespace drule {

  unsigned char alpha_opacity(double d)
  {
    ASSERT ( d >= 0.0 && d <= 1.0, (d) );
    return static_cast<unsigned char>(255 * d);
  }

  /// @name convertors for standart types
  //@{
  template <class T> T get_value(string const & s);

  template <> bool get_value<bool>(string const & s)
  {
    return (s == "yes" || s == "1");
  }
  template <> double get_value<double>(string const & s)
  {
    double d;
    VERIFY ( string_utils::to_double(s, d), ("Bad double in drawing rule : ", s) );
    return d;
  }
  template <> string get_value<string>(string const & s)
  {
    string ss(s);
    string_utils::make_lower_case(ss);
    return ss;
  }
  //@}

  /// parameters tuple initialization
  //@{
  class assign_element
  {
    attrs_map_t const & m_attrs;
    string * m_keys;

  public:
    assign_element(attrs_map_t const & attrs, string * keys) : m_attrs(attrs), m_keys(keys) {}

    template <class T> void operator() (T & t, int n)
    {
      attrs_map_t::const_iterator i = m_attrs.find(m_keys[n]);
      if (i != m_attrs.end())
        t = get_value<T>(i->second);
    }
  };

  template <class Tuple, int N>
  void parse_tuple(Tuple & t, attrs_map_t const & attrs, string (&keys)[N])
  {
    STATIC_ASSERT ( N == tuple_length<Tuple>::value );
    assign_element toDo(attrs, keys);
    for_each_tuple(t, toDo);
  }
  //@}

  /// main function for rule creation
  template <class T> T * create_rule(attrs_map_t const & attrs)
  {
    T * p = new T();
    parse_tuple(p->m_params, attrs, T::arrKeys);
    return p;
  }

  /// compare rules
  template <class T> bool is_equal_rules(T const * p1, BaseRule const * p2)
  {
    T const * pp2 = dynamic_cast<T const *>(p2);
    if (pp2)
      return (p1->IsEqualBase(p2) && (p1->m_params == pp2->m_params));
    return false;
  }

  template <class TArchive, class T> void write_rules(TArchive & ar, T const * p)
  {
    p->WriteBase(ar);
    serial::save_tuple(ar, p->m_params);
  }
  template <class TArchive, class T> void read_rules(TArchive & ar, T * p)
  {
    p->ReadBase(ar);
    serial::load_tuple(ar, p->m_params);
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // px_metric_t
  ////////////////////////////////////////////////////////////////////////////////////////
  struct px_metric_t
  {
    double m_v;
    px_metric_t(double v = 0.0) : m_v(v) {}
    bool operator==(px_metric_t const & r) const { return m_v == r.m_v; }
  };

  template <class TArchive> TArchive & operator << (TArchive & ar, px_metric_t const & t)
  {
    ar << t.m_v;
    return ar;
  }
  template <class TArchive> TArchive & operator >> (TArchive & ar, px_metric_t & t)
  {
    ar >> t.m_v;
    return ar;
  }

  template <> px_metric_t get_value<px_metric_t>(string const & s)
  {
    size_t i = s.find("px");
    if (i == string::npos)
      i = s.size();

    return px_metric_t(atof(s.substr(0, i).c_str()));
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // color_t
  ////////////////////////////////////////////////////////////////////////////////////////
  struct color_t
  {
    int32_t m_v;
    enum XXX { none = -1 };

    color_t(int32_t v = 0) : m_v(v) {}
    bool operator==(color_t const & r) const { return m_v == r.m_v; }
  };

  template <class TArchive> TArchive & operator << (TArchive & ar, color_t const & t)
  {
    ar << t.m_v;
    return ar;
  }
  template <class TArchive> TArchive & operator >> (TArchive & ar, color_t & t)
  {
    ar >> t.m_v;
    return ar;
  }

  template <> color_t get_value<color_t>(string const & s)
  {
    int v = 0;
    if (s[0] == '#')
    {
      char * dummy;
      v = strtol(&s[1], &dummy, 16);
    }
    else if (s == "none") v = color_t::none;
    else if (s == "black") { /*already initialized*/ }
    else if (s == "white") v = 0xFFFFFF;
    else if (s == "red") v = 0xFF0000;
    else if (s == "green") v = 0x00FF00;
    else if (s == "blue") v = 0x0000FF;
    else if (s == "lightblue") v = 0xADD8E6;
    else if (s == "yellow") v = 0xFFFF00;
    else
    {
      ASSERT ( !"check color values", (s) );
    }
    return v;
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // dash_array_t
  ////////////////////////////////////////////////////////////////////////////////////////
  struct dash_array_t
  {
    vector<double> m_v;
    void add(string const & s)
    {
      double const v = atof(s.c_str());
      if (v != 0.0) m_v.push_back(v);
    }

    bool operator==(dash_array_t const & r) const { return m_v == r.m_v; }
  };

  template <class TArchive> TArchive & operator << (TArchive & ar, dash_array_t const & t)
  {
    ar << t.m_v;
    return ar;
  }
  template <class TArchive> TArchive & operator >> (TArchive & ar, dash_array_t & t)
  {
    ar >> t.m_v;
    return ar;
  }

  template <> dash_array_t get_value<dash_array_t>(string const & s)
  {
    dash_array_t ret;
    string_utils::TokenizeString(s, " \tpx,", bind(&dash_array_t::add, ref(ret), _1));

    /// @see http://www.w3.org/TR/SVG/painting.html stroke-dasharray
    size_t const count = ret.m_v.size();
    if (count % 2 != 0)
      for (size_t i = 0; i < count; ++i)
      {
        double const d = ret.m_v[i];
        ret.m_v.push_back(d);
      }

    return ret;
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // line_cap_t
  ////////////////////////////////////////////////////////////////////////////////////////
  struct line_cap_t
  {
    enum { round, butt, square };
    int32_t m_v;

    line_cap_t(int32_t v = round) : m_v(v) {}
    bool operator==(line_cap_t const & r) const { return m_v == r.m_v; }
  };

  template <class TArchive> TArchive & operator << (TArchive & ar, line_cap_t const & t)
  {
    ar << t.m_v;
    return ar;
  }
  template <class TArchive> TArchive & operator >> (TArchive & ar, line_cap_t & t)
  {
    ar >> t.m_v;
    return ar;
  }

  template <> line_cap_t get_value<line_cap_t>(string const & s)
  {
    int v = line_cap_t::round;
    if (s == "round") { /*initialized*/ }
    else if (s == "butt") v = line_cap_t::butt;
    else if (s == "square") v = line_cap_t::square;
    else
    {
      ASSERT ( !"check stroke-linecap values", (s) );
    }
    return line_cap_t(v);
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // pattern_url_t
  ////////////////////////////////////////////////////////////////////////////////////////
  struct pattern_url_t : public color_t
  {
    string m_pattern;

    pattern_url_t() : color_t(color_t::none) {}
    pattern_url_t(string const & s) : color_t(color_t::none), m_pattern(s) {}
    pattern_url_t(color_t const & r) : color_t(r) {}

    bool operator==(pattern_url_t const & r) const
    {
      return (m_v == r.m_v) && (m_pattern == r.m_pattern);
    }
  };

  template <class TArchive> TArchive & operator << (TArchive & ar, pattern_url_t const & t)
  {
    ar << t.m_v << t.m_pattern;
    return ar;
  }
  template <class TArchive> TArchive & operator >> (TArchive & ar, pattern_url_t & t)
  {
    ar >> t.m_v >> t.m_pattern;
    return ar;
  }

  template <> pattern_url_t get_value<pattern_url_t>(string const & s)
  {
    if (s[0] == 'u')
    {
      /// @todo make fill pattern by symbol
      return pattern_url_t(s);
    }
    else
      return get_value<color_t>(s);
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // position_t
  ////////////////////////////////////////////////////////////////////////////////////////
  struct position_t
  {
    enum { center = 0 };
    int32_t m_v;

    position_t(int32_t v = center) : m_v(v) {}
    bool operator==(position_t const & r) const { return m_v == r.m_v; }
  };

  template <class TArchive> TArchive & operator << (TArchive & ar, position_t const & t)
  {
    ar << t.m_v;
    return ar;
  }
  template <class TArchive> TArchive & operator >> (TArchive & ar, position_t & t)
  {
    ar >> t.m_v;
    return ar;
  }

  template <> position_t get_value<position_t>(string const & s)
  {
    int v = position_t::center;
    if (s == "center") { /*initialized*/ }
    else
    {
      ASSERT ( !"check symbol position values", (s) );
    }

    return position_t(v);
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // txt_anchor_t
  ////////////////////////////////////////////////////////////////////////////////////////
  struct txt_anchor_t
  {
    enum { start, middle, end };
    int32_t m_v;

    txt_anchor_t(int32_t v = start) : m_v(v) {}
    bool operator==(txt_anchor_t const & r) const { return m_v == r.m_v; }
  };

  template <class TArchive> TArchive & operator << (TArchive & ar, txt_anchor_t const & t)
  {
    ar << t.m_v;
    return ar;
  }
  template <class TArchive> TArchive & operator >> (TArchive & ar, txt_anchor_t & t)
  {
    ar >> t.m_v;
    return ar;
  }

  template <> txt_anchor_t get_value<txt_anchor_t>(string const & s)
  {
    int v = txt_anchor_t::start;
    if (s == "start") { /*initialized*/ }
    else if (s == "middle") v = txt_anchor_t::middle;
    else if (s == "end") v = txt_anchor_t::end;
    else
    {
      ASSERT ( !"check text-anchor values", (s) );
    }

    return txt_anchor_t(v);
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // font_family_t
  ////////////////////////////////////////////////////////////////////////////////////////
  struct font_family_t
  {
    string m_v;
    font_family_t() : m_v("DejaVu Sans") {}
    bool operator==(font_family_t const & r) const { return m_v == r.m_v; }
  };

  template <class TArchive> TArchive & operator << (TArchive & ar, font_family_t const & t)
  {
    ar << t.m_v;
    return ar;
  }
  template <class TArchive> TArchive & operator >> (TArchive & ar, font_family_t & t)
  {
    ar >> t.m_v;
    return ar;
  }

  template <> font_family_t get_value<font_family_t>(string const & /*s*/)
  {
    /// @todo process font (example: "DejaVu Sans",sans-serif)
    return font_family_t();
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // percent_t
  ////////////////////////////////////////////////////////////////////////////////////////
  struct percent_t
  {
    int32_t m_v;
    percent_t(int32_t v = 0) : m_v(v) {}
    bool operator==(percent_t const & r) const { return m_v == r.m_v; }
  };

  template <class TArchive> TArchive & operator << (TArchive & ar, percent_t const & t)
  {
    ar << t.m_v;
    return ar;
  }
  template <class TArchive> TArchive & operator >> (TArchive & ar, percent_t & t)
  {
    ar >> t.m_v;
    return ar;
  }

  template <> percent_t get_value<percent_t>(string const & s)
  {
    size_t i = s.find_first_of('%');
    if (i == string::npos)
    {
      ASSERT ( !"percent string has no % mark", (s) );
      i = s.size();
    }

    return percent_t(atoi(s.substr(0, i).c_str()));
  }

  ////////////////////////////////////////////////////////////////////////////////////////
  // LineRule
  ////////////////////////////////////////////////////////////////////////////////////////
  struct LineRule : public BaseRule
  {
    tuple<bool, double, double, double,
          pattern_url_t, px_metric_t, dash_array_t, line_cap_t, double, px_metric_t> m_params;

    LineRule()
      : m_params(make_tuple(false, 1.0, 0.0, std::numeric_limits<double>::max(),
                            pattern_url_t(), -1.0, dash_array_t(), line_cap_t(), 1.0,
                            0.0))
    {}

    virtual bool IsEqual(BaseRule const * p) const { return is_equal_rules(this, p); }
    virtual void Read(FileReaderStream & ar) { read_rules(ar, this); }
    virtual void Write(FileWriterStream & ar) const { write_rules(ar, this); }

    virtual int GetColor() const { return m_params.get<4>().m_v; }
    virtual unsigned char GetAlpha() const { return alpha_opacity(m_params.get<8>()); }
    virtual double GetWidth() const
    {
      double w = m_params.get<5>().m_v;
      if (w == -1) return 0.0;

      if (m_params.get<0>())
      {
        double minw = m_params.get<2>();
        double maxw = m_params.get<3>();

        if (w < minw) w = minw;
        if (w > maxw) w = maxw;
        w *= m_params.get<1>();
      }
      return w;
    }
    virtual void GetPattern(vector<double> & p, double & offset) const
    {
      offset = m_params.get<9>().m_v;
      p = m_params.get<6>().m_v;
    }

    static string arrKeys[10];
  };
  string LineRule::arrKeys[] = {
    // If it is yes, the line will be drawn with the width reflecting the width=* value,
    // if the way has the width tag.
    "honor-width",
    // Is scale factor from width tag value (meter) to pixel width of the SVG.
    "width-scale-factor",
    // Specify the minimum and the maximum width. If the way doesn't have the width tag,
    // the line is drawn with the width specified by CSS
    "minimum-width", "maximum-width",
    // The color of the line.
    "stroke",
    // The width of the line.
    "stroke-width",
    // Specifying the line style.
    "stroke-dasharray",
    // How to draw the terminal. Choice one from round, butt or square.
    "stroke-linecap",
    // The opacity of the line. The value takes from 0.0 (completely invisible) to
    // 1.0 (completely overdrawing).
    // The default is 1.0
    "stroke-opacity",

    // undocumented
    "stroke-dashoffset"
  };

  ////////////////////////////////////////////////////////////////////////////////////////
  // AreaRule
  ////////////////////////////////////////////////////////////////////////////////////////
  struct AreaRule : public BaseRule
  {
    tuple<pattern_url_t, double, pattern_url_t, px_metric_t, double> m_params;

    AreaRule() : m_params(make_tuple(pattern_url_t(), 1.0, pattern_url_t(), 1.0, 1.0)) {}

    virtual bool IsEqual(BaseRule const * p) const { return is_equal_rules(this, p); }
    virtual void Read(FileReaderStream & ar) { read_rules(ar, this); }
    virtual void Write(FileWriterStream & ar) const { write_rules(ar, this); }

    virtual int GetFillColor() const { return m_params.get<0>().m_v; }
    virtual int GetColor() const { return m_params.get<2>().m_v; }
    virtual unsigned char GetAlpha() const { return alpha_opacity(m_params.get<1>()); }
    virtual double GetWidth() const { return m_params.get<3>().m_v; }

    static string arrKeys[5];
  };
  string AreaRule::arrKeys[] =
  {
    "fill", "fill-opacity", "stroke", "stroke-width", "stroke-opacity"
  };

  ////////////////////////////////////////////////////////////////////////////////////////
  // SymbolRule
  ////////////////////////////////////////////////////////////////////////////////////////
  struct SymbolRule : public BaseRule
  {
    tuple<string, string, position_t> m_params;

    virtual bool IsEqual(BaseRule const * p) const { return is_equal_rules(this, p); }
    virtual void Read(FileReaderStream & ar) { read_rules(ar, this); }
    virtual void Write(FileWriterStream & ar) const { write_rules(ar, this); }

    virtual void GetSymbol(string & s) const
    {
      s = m_params.get<0>();
      if (s.empty())
      {
        string const & ss = m_params.get<1>();
        if (!ss.empty()) s = ss.substr(1, ss.size()-1);
      }
    }

    static string arrKeys[3];
  };
  // "width", "height", "transform"
  string SymbolRule::arrKeys[] = { "ref", "xlink:href", "position" };

  ////////////////////////////////////////////////////////////////////////////////////////
  // CaptionRule
  ////////////////////////////////////////////////////////////////////////////////////////
  struct CaptionRule : public BaseRule
  {
    tuple<string, px_metric_t, px_metric_t, txt_anchor_t,
          font_family_t, px_metric_t, color_t, double, color_t> m_params;

    CaptionRule()
      : m_params(make_tuple("", 0, 0, txt_anchor_t(),
                            font_family_t(), 2.0, color_t(), 1.0, color_t::none))
    {}

    virtual bool IsEqual(BaseRule const * p) const { return is_equal_rules(this, p); }
    virtual void Read(FileReaderStream & ar) { read_rules(ar, this); }
    virtual void Write(FileWriterStream & ar) const { write_rules(ar, this); }

    virtual double GetTextHeight() const { return m_params.get<5>().m_v; }
    virtual int GetFillColor() const { return m_params.get<6>().m_v; }
    virtual int GetColor() const {return m_params.get<8>().m_v; }

    static string arrKeys[9];
  };
  string CaptionRule::arrKeys[] = {
    // it specified the key of the text instruction (eg k="name" ).
    "k",
    // Translational length on x or y axis from the center point.
    "dx", "dy",
    // It specify the anchor point on the text. You can choice form start, middle or end.
    // The default value is start.
    // It is the same meaning with svg:text-anchor property
    "text-anchor",

    "font-family",  // The font family of the text. (ex. serif, "DejaVu Sans")
    "font-size",    // The size of the font.
    "fill",         // The color of the text.
    "fill-opacity", // The opacity of the text. The value takes from 0.0 (completely transparent)
                    // to 1.0 (completely overdrawing). The default is 1.0 .
    "stroke"        // The color of the font outline. Usually it should be none.
  };

  ////////////////////////////////////////////////////////////////////////////////////////
  // CircleRule
  ////////////////////////////////////////////////////////////////////////////////////////
  struct CircleRule : public BaseRule
  {
    tuple<px_metric_t, color_t, double, color_t, px_metric_t, double> m_params;

    CircleRule() : m_params(make_tuple(1, color_t(), 1.0, color_t::none, 0.0, 1.0)) {}

    virtual bool IsEqual(BaseRule const * p) const { return is_equal_rules(this, p); }
    virtual void Read(FileReaderStream & ar) { read_rules(ar, this); }
    virtual void Write(FileWriterStream & ar) const { write_rules(ar, this); }

    virtual double GetRadius() const { return m_params.get<0>().m_v; }
    virtual int GetFillColor() const { return m_params.get<1>().m_v; }
    virtual unsigned char GetAlpha() const { return alpha_opacity(m_params.get<2>()); }

    virtual int GetColor() const { return m_params.get<3>().m_v; }
    virtual double GetWidth() const { return m_params.get<4>().m_v; }

    static string arrKeys[6];
  };
  string CircleRule::arrKeys[] = {
    "r",              // The radius of a circle.
    "fill",           // The color of the filling.
    "fill-opacity",   // The opacity of the filling. The value takes from 0.0 (completely transparent)
                      // to 1.0 (completely overdrawing). The default is 1.0 .
    "stroke",         // The color of the outline. If you don't want to draw the outline,
                      // set it as none.
    "stroke-width",   // The width of the outline.
    "stroke-opacity"  // The opacity of the line.
  };

  ////////////////////////////////////////////////////////////////////////////////////////
  // PathTextRule
  ////////////////////////////////////////////////////////////////////////////////////////
  struct PathTextRule : public BaseRule
  {
    tuple<string, px_metric_t, px_metric_t, txt_anchor_t, percent_t, /*bool,*/
          font_family_t, px_metric_t, color_t, double, color_t> m_params;

    PathTextRule()
      : m_params(make_tuple("", 0, 0, txt_anchor_t(), 0, /*true,*/
                            font_family_t(), 2.0, color_t(), 1.0, color_t::none))
    {}

    virtual bool IsEqual(BaseRule const * p) const { return is_equal_rules(this, p); }
    virtual void Read(FileReaderStream & ar) { read_rules(ar, this); }
    virtual void Write(FileWriterStream & ar) const { write_rules(ar, this); }

    virtual double GetTextHeight() const { return m_params.get<6>().m_v; }
    virtual int GetColor() const {return m_params.get<7>().m_v;}

    static string arrKeys[10];
  };
  string PathTextRule::arrKeys[] = {
    // It specified the key of the text instruction (eg k="name" ).
    "k",
    // Translational length on y axis from the center line of the way.
    // Usually this is used for drawing ref=*.
    "dx", "dy",
    // It specify the anchor point on the text. You can choice form start,middle or end.
    // The default value is start. It is the same meaning with svg:text-anchor property
    "text-anchor",
    // It specify the anchor point on the path. The value is given by %.
    // The range is form 0% (start of the path) to 100%
    // (end of the path). The default value is 0%.
    // It is the same meaning with <svg:textPath StartOffset="">
    "startOffset",
    // This works only on or/p now
    //"avoid-duplicate",

    "font-family",  // The font family of the text. (ex. serif, "DejaVu Sans")
    "font-size",    // The size of the font.
    "fill",         // The color of the text.
    "fill-opacity", // The opacity of the text. The value takes from 0.0 (completely transparent)
                    // to 1.0 (completely overdrawing). The default is 1.0 .
    "stroke"        // The color of the font outline. Usually it should be none.
  };

  ////////////////////////////////////////////////////////////////////////////////////////
  // WayMarkerRule
  ////////////////////////////////////////////////////////////////////////////////////////
  struct WayMarkerRule : public BaseRule
  {
    tuple<string, color_t, px_metric_t, line_cap_t, double> m_params;

    WayMarkerRule() : m_params(make_tuple("", color_t(), 1.0, line_cap_t(), 1.0)) {}

    virtual bool IsEqual(BaseRule const * p) const { return is_equal_rules(this, p); }
    virtual void Read(FileReaderStream & ar) { read_rules(ar, this); }
    virtual void Write(FileWriterStream & ar) const { write_rules(ar, this); }

    static string arrKeys[5];
  };
  string WayMarkerRule::arrKeys[] = {
    // It specified the key of the way that passes through the node. (eg k="highway" ).
    "k",
    "stroke",           // The color of the line.
    "stroke-width",     // The width of the line.
    "stroke-linecap",   // How to draw the terminal. Choice one from round, butt or square.
    "stroke-opacity"    // The opacity of the line. The value takes from 0.0 (completely invisible)
                        // to 1.0 (completely overdrawing). The default is 1.0 .
  };

////////////////////////////////////////////////////////////////////////////////////////////
// BaseRule implementation
////////////////////////////////////////////////////////////////////////////////////////////
void BaseRule::ReadBase(FileReaderStream & ar)
{
  ar >> m_class >> m_type;
}

void BaseRule::WriteBase(FileWriterStream & ar) const
{
  ar << m_class << m_type;
}

namespace
{
  bool find_sub_str(string const & s, char const * p)
  {
    size_t i = s.find(p);
    return (i != string::npos);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
// RulesHolder implementation
////////////////////////////////////////////////////////////////////////////////////////////
RulesHolder::~RulesHolder()
{
  Clean();
}

void RulesHolder::Clean()
{
  for (size_t i = 0; i < m_container.size(); ++i)
  {
    rule_vec_t & v = m_container[i];
    for (size_t j = 0; j < v.size(); ++j)
      delete v[j];
    v.clear();
  }

  m_rules.clear();
}

void RulesHolder::SetParseFile(char const * fPath, int scale)
{
  m_file.clear();
  m_currScale = scale;

#ifndef OMIM_OS_BADA
  ifstream file(fPath);
  if (!file.is_open()) return;

  vector<char> v(500);
  bool doPush = false;

  // push the part of file <style>...</style> to the string buffer
  while (!file.eof())
  {
    file.getline(&v[0], v.size());

    if (!doPush)
    {
      if (find_sub_str(&v[0], "<style"))
        doPush = true;
    }

    if (doPush)
    {
      m_file += ' ';  // add delimeter between strings
      m_file += &v[0];

      if (find_sub_str(&v[0], "</style>"))
        break;
    }
  }
#else
  ASSERT ( false, ("SetParseFile uses only in indexer_tool") );
#endif
}

/// parse pairs (<key>:<value>;) for a specified 'class' attribute obj in file buffer
void RulesHolder::PushAttributes(string objClass, attrs_map_t & attrs)
{
  objClass = '.' + objClass;

  size_t objSz = objClass.size();
  size_t buffSz = m_file.size();

  // find 'objClass' string in buffer
  size_t i = 0;
  while (i < buffSz)
  {
    i = m_file.find(objClass, i);
    if (i == string::npos) return;

    i += objSz;
    if (i >= buffSz) return;

    if (m_file[i] == ' ' || m_file[i] == '\t' || m_file[i] == '{') break;
  }

  // parse attributes in {...} scope
  while (++i < buffSz)
  {
    i = m_file.find_first_not_of("{ \t", i);
    if (i == string::npos || m_file[i] == '}') return;

    size_t e = m_file.find_first_of(':', i);
    if (e == string::npos) return;

    string const k = m_file.substr(i, e-i);

    i = m_file.find_first_not_of(" \t", e + 1);
    if (i == string::npos) return;

    e = m_file.find_first_of(';', i);
    if (e == string::npos) return;

    string const v = m_file.substr(i, e-i);

    // How to push values, if the params are duplicating?
    // Assume that the latest rule is better than previous.
    attrs[k] = v;

    i = e;
  }
}

namespace
{
  char const * arrClassTags[] = { "class", "mask-class" };
}

void RulesHolder::CreateRules(string const & name, uint8_t type, attrs_map_t const & attrs, vector<Key> & v)
{
  bool added = false;

  for (size_t i = 0; i < ARRAY_SIZE(arrClassTags); ++i)
  {
    attrs_map_t::const_iterator it = attrs.find(arrClassTags[i]);
    if (it != attrs.end())
    {
      added = true;
      v.push_back(CreateRuleImpl1(name, type, it->second, attrs, i == 1));
    }
  }

  if (!added)
    v.push_back(CreateRuleImpl2(name, type, "", attrs));
}

Key RulesHolder::CreateRuleImpl1(string const & name,
                                 uint8_t type,
                                 string const & clValue,
                                 attrs_map_t const & attrs,
                                 bool isMask)
{
#ifdef DEBUG
  if (clValue.find("highway-pedestrian-area") != string::npos)
  {
    ASSERT (true, ());
  }
#endif

  attrs_map_t a;
  string_utils::TokenizeString(clValue, " \t", bind(&RulesHolder::PushAttributes, this, _1, ref(a)));

  for (attrs_map_t::const_iterator i = attrs.begin(); i != attrs.end(); ++i)
    if (!string_utils::IsInArray(arrClassTags, i->first))
      a[i->first] = i->second;

  // background color (imitation of masks in tunnel patterns)
  if (isMask) a["stroke"] = "#ffffff";

  // patch the tunnel draw rules -> make line draw rule
  if (name == "tunnel")
  {
    attrs_map_t::iterator i = a.find("width");
    if (i != a.end())
    {
      a["stroke-width"] = i->second;
      a.erase(i);
    }

    return CreateRuleImpl2("line", type, clValue, a);
  }
  else
    return CreateRuleImpl2(name, type, clValue, a);
}

Key RulesHolder::CreateRuleImpl2(string const & name,
                                 uint8_t rType,
                                 string const & clName,
                                 attrs_map_t const & attrs)
{
  BaseRule * pRule = 0;
  int type = -1;

  if (name == "line")
  {
    pRule = create_rule<LineRule>(attrs);
    type = line;
  }
  else if (name == "area")
  {
    pRule = create_rule<AreaRule>(attrs);
    type = area;
  }
  else if (name == "symbol")
  {
    pRule = create_rule<SymbolRule>(attrs);
    type = symbol;
  }
  else if (name == "caption" || name == "text")
  {
    pRule = create_rule<CaptionRule>(attrs);
    type = caption;
  }
  else if (name == "circle")
  {
    pRule = create_rule<CircleRule>(attrs);
    type = circle;
  }
  else if (name == "pathText")
  {
    pRule = create_rule<PathTextRule>(attrs);
    type = pathtext;
  }
  else if (name == "wayMarker")
  {
    pRule = new WayMarkerRule();
    type = waymarker;
  }

  if (pRule)
  {
    pRule->SetType(rType);

    // find existing equal rule for scale and type
    vector<uint32_t> & rInd = m_rules[m_currScale][type];
    vector<BaseRule*> & rCont = m_container[type];
    size_t ind = 0;
    for (; ind < rInd.size(); ++ind)
    {
      ASSERT ( rInd[ind] < rCont.size(), (rInd[ind], rCont.size()) );
      if (rCont[rInd[ind]]->IsEqual(pRule))
        break;
    }

    if (ind == rInd.size())
    {
      // add new rule
      pRule->SetClassName(clName);

      rCont.push_back(pRule);
      rInd.push_back(rCont.size()-1);
      ind = rInd.size()-1;
    }
    else
      delete pRule;

    return Key(m_currScale, type, ind);
  }
  else
  {
    ASSERT ( !"check possible rules", (name) );
    return Key();
  }
}

size_t RulesHolder::AddRule(int32_t scale, rule_type_t type, BaseRule * p)
{
  ASSERT ( 0 <= scale && scale <= scales::GetUpperScale(), (scale) );
  ASSERT ( 0 <= type && type < count_of_rules, () );

  m_container[type].push_back(p);

  vector<uint32_t> & v = m_rules[scale][type];
  v.push_back(m_container[type].size()-1);

  size_t const ret = v.size() - 1;
  ASSERT ( Find(Key(scale, type, ret)) == p, (ret) );
  return ret;
}

size_t RulesHolder::AddLineRule(int32_t scale, int color, double pixWidth)
{
  LineRule * p = new LineRule();
  p->m_params.get<4>() = color_t(color);
  p->m_params.get<5>() = pixWidth / scales::GetM2PFactor(scale);
  return AddRule(scale, line, p);
}

BaseRule const * RulesHolder::Find(Key const & k) const
{
  rules_map_t::const_iterator i = m_rules.find(k.m_scale);
  if (i == m_rules.end()) return 0;

  vector<uint32_t> const & v = (i->second)[k.m_type];

  ASSERT ( k.m_index >= 0, (k.m_index) );
  if (static_cast<size_t>(k.m_index) < v.size())
    return m_container[k.m_type][v[k.m_index]];
  else
    return 0;
}

FileWriterStream & operator << (FileWriterStream & ar, BaseRule * p)
{
  p->Write(ar);
  return ar;
}

void do_load(FileReaderStream & ar, size_t ind, BaseRule * & p)
{
  switch (ind)
  {
  case line: p = new LineRule(); break;
  case area: p = new AreaRule(); break;
  case symbol: p = new SymbolRule(); break;
  case caption: p = new CaptionRule(); break;
  case circle: p = new CircleRule(); break;
  case pathtext: p = new PathTextRule(); break;
  case waymarker: p = new WayMarkerRule(); break;
  default:
    ASSERT ( !"Incorrect draw rule type for reading.", (ind) );
    throw std::out_of_range("Bad draw rule index");
  }

  p->Read(ar);
}

void RulesHolder::Read(FileReaderStream & s)
{
  Clean();

  serial::do_load(s, m_container);
  s >> m_rules;
}

void RulesHolder::Write(FileWriterStream & s)
{
  s << m_container << m_rules;
}

void WriteRules(char const * fPath)
{
  FileWriterStream file(fPath);
  rules().Write(file);
}

void ReadRules(char const * fPath)
{
  FileReaderStream file(fPath);
  rules().Read(file);
}

RulesHolder & rules()
{
  static RulesHolder holder;
  return holder;
}

}
