#pragma once

#include "../../platform/platform.hpp"

namespace stats
{

class StatsWriter;

class Client
{
public:
  Client();
  ~Client();
  bool Search(string const & query);
  bool DropPin(long long latlong, string const & label);

private:
  StatsWriter * m_writer;
};

}  // namespace stats
