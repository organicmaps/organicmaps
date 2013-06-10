#include "stats_client.hpp"
#include "stats_writer.hpp"

#include "../common/wire.pb.h"

namespace stats
{

Client::Client()
  : m_writer(new StatsWriter(GetPlatform().UniqueClientId(), GetPlatform().WritableDir() + "stats"))
{
}

Client::~Client()
{
  delete m_writer;
}

bool Client::Search(string const & query)
{
  class Search s;
  s.set_query(query);
  return m_writer->Write(s);
}

bool Client::DropPin(long long latlong, string const & label)
{
  PinDrop p;
  p.set_latlong(latlong);
  p.set_label(label);
  return m_writer->Write(p);
}

}  // namespace stats
