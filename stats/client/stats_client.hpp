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

private:
  StatsWriter * m_writer;
};

}  // namespace stats
