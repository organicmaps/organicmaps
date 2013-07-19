#pragma once

#include "../std/map.hpp"
#include "../std/string.hpp"
#include "../std/iostream.hpp"

#include "../graphics/defines.hpp"

namespace srv
{
  class RequestDumper;
  class Request
  {
  public:
    struct Params
    {
      Params();
      srv::Viewport m_viewport;
      graphics::EDensity m_density;
    };

    Request(const Params & params);

    typedef map<string, string> request_params_t;
    Request(request_params_t const & params);

    srv::Viewport GetViewport() const;
    graphics::EDensity GetDensity() const;

    srv::Response * CreateResponse(QByteArray const & data) const;

  private:
    friend class RequestDumper;
    string GetParam(const string & paramName) const;

    map<string, string> m_params;
  };

  Request GetRequest(istream & is);
  void SendRequest(ostream & os, Request const & r);
}
