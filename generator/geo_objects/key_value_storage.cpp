#include "generator/geo_objects/key_value_storage.hpp"

#include "base/logging.hpp"

namespace generator
{
namespace geo_objects
{
KeyValueStorage::KeyValueStorage(std::istream & stream, std::function<bool(KeyValue const &)> const & pred)
{
  std::string line;
  std::streamoff lineNumber = 0;
  while (std::getline(stream, line))
  {
    ++lineNumber;

    KeyValue kv;
    if (!ParseKeyValueLine(line, kv, lineNumber) || !pred(kv))
      continue;

    m_values.insert(kv);
  }
}

// static
bool KeyValueStorage::ParseKeyValueLine(std::string const & line, KeyValue & res, std::streamoff lineNumber)
{
  auto const pos = line.find(" ");
  if (pos == std::string::npos)
  {
    LOG(LWARNING, ("Cannot find separator in line", lineNumber));
    return false;
  }

  int64_t id;
  if (!strings::to_int64(line.substr(0, pos), id))
  {
    LOG(LWARNING, ("Cannot parse id", line.substr(0, pos) , "in line", lineNumber));
    return false;
  }

  base::Json json;
  try
  {
    json = base::Json(line.c_str() + pos + 1);
    if (!json.get())
      return false;
  }
  catch (base::Json::Exception const & err)
  {
    LOG(LWARNING, ("Cannot create base::Json in line", lineNumber, ":", err.Msg()));
    return false;
  }

  res = std::make_pair(static_cast<uint64_t>(id), json);
  return true;
}

boost::optional<base::Json> KeyValueStorage::Find(uint64_t key) const
{
  auto const it = m_values.find(key);
  if (it == std::end(m_values))
    return {};

  return it->second;
}
  
size_t KeyValueStorage::Size() const
{
  return m_values.size();
}
}  // namespace geo_objects
}  // namespace generator
