#include "request.hpp"
#include "response_impl/response_cout.hpp"

#include "../coding/base64.hpp"

#include "../base/string_utils.hpp"
#include "../base/stl_add.hpp"

#include "../std/iostream.hpp"
#include "../std/vector.hpp"

#include "../platform/settings.hpp"
#include "../3party/jansson/myjansson.hpp"

namespace srv
{
  namespace
  {
    inline string ViewportToString(const Viewport & v)
    {
      my::Json root(json_object());
      m2::PointD center = v.GetCenter();
      json_object_set_new(root, "CenterX", json_real(center.x));
      json_object_set_new(root, "CenterY", json_real(center.y));
      json_object_set_new(root, "Scale", json_real(v.GetScale()));
      json_object_set_new(root, "Width", json_integer(v.GetWidth()));
      json_object_set_new(root, "Height", json_integer(v.GetHeight()));

      char * res = json_dumps(root, JSON_PRESERVE_ORDER | JSON_COMPACT | JSON_INDENT(1));

      if (res == NULL)
        return "";

      string s(res);
      free(res);
      return s;
    }

    inline srv::Viewport StringToViewport(const string & v)
    {
      my::Json root(v.c_str());

      m2::PointD center;

      center.x = json_number_value(json_object_get(root, "CenterX"));
      center.y = json_number_value(json_object_get(root, "CenterY"));
      double scale = json_number_value(json_object_get(root, "Scale"));
      int width = json_integer_value(json_object_get(root, "Width"));
      int height = json_integer_value(json_object_get(root, "Height"));

      return srv::Viewport(center, scale, width, height);
    }
  }

  Request::Params::Params()
    : m_viewport(m2::PointD(0.0, 0.0), 0, 0, 0)
  {
    m_density = graphics::EDensityMDPI;
  }

  class RequestDumper
  {
  public:
    RequestDumper(const Request & r)
      : m_r(r)
    {
    }

    Request::request_params_t::const_iterator begin()
    {
      return m_r.m_params.begin();
    }

    Request::request_params_t::const_iterator end()
    {
      return m_r.m_params.end();
    }

    size_t count() const
    {
      return m_r.m_params.size();
    }

  private:
    const Request & m_r;
  };

  Request::Request(const Params & params)
  {
    m_params["viewport"] = ViewportToString(params.m_viewport);
    m_params["density"] = ::Settings::ToString((int)params.m_density) ;
  }

  Request::Request(request_params_t const & params)
    : m_params(params)
  {
  }

  string Request::GetParam(string const & name) const
  {
    map<string, string>::const_iterator it = m_params.find(name);
    if (it != m_params.end())
      return it->second;
    else
      return "";
  }

  srv::Viewport Request::GetViewport() const
  {
    string const & viewportString = GetParam("viewport");
    return StringToViewport(viewportString);
  }

  graphics::EDensity Request::GetDensity() const
  {
    int density;
    if (!::Settings::FromString(GetParam("density"), density))
      return graphics::EDensityMDPI;

    return (graphics::EDensity)density;
  }

  srv::Response * Request::CreateResponse(const QByteArray &data) const
  {
    return new srv::impl::ResponseCout(data);
  }

  Request GetRequest(istream & is)
  {
    string line;
    Request::request_params_t params;

    getline(is, line);
    vector<string> v;
    strings::Tokenize(line, "&", MakeBackInsertFunctor(v));
    for (unsigned i = 0; i < v.size(); ++i)
    {
      string name;
      string value;

      int pos = v[i].find_first_of("=");

      if (pos != string::npos)
      {
        name = v[i].substr(0, pos);
        value = v[i].substr(pos + 1);
        params[name] = base64::Decode(value);
      }
    }

    return Request(params);
  }

  void SendRequest(ostream & os, Request const & r)
  {
    RequestDumper dumper(r);

    vector<string> dumpedParams;
    dumpedParams.reserve(dumper.count());

    Request::request_params_t::const_iterator it;
    for (it = dumper.begin(); it != dumper.end(); ++it)
    {
      string param = it->first;
      param += "=";
      param += base64::Encode(it->second);
      dumpedParams.push_back(param);
    }

    string result;
    for (size_t i = 0; i < dumpedParams.size(); ++i)
    {
      result += dumpedParams[i];
      if (i != dumpedParams.size() - 1)
        result += "&";
    }

    os << result << endl;
  }
}
