#include "generator/key_value_storage.hpp"

#include "coding/reader.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>


namespace generator
{
KeyValueStorage::KeyValueStorage(std::string const & path, size_t cacheValuesCountLimit,
                                 std::function<bool(KeyValue const &)> const & pred)
  : m_storage{path, std::ios_base::in | std::ios_base::out | std::ios_base::app}
  , m_cacheValuesCountLimit{cacheValuesCountLimit}
{
  if (!m_storage)
    MYTHROW(Reader::OpenException, ("Failed to open file", path));

  std::string line;
  std::streamoff lineNumber = 0;
  while (std::getline(m_storage, line))
  {
    ++lineNumber;

    uint64_t key;
    auto value = std::string{};
    if (!ParseKeyValueLine(line, lineNumber, key, value))
      continue;

    std::shared_ptr<JsonValue> json;
    try
    {
      json = std::make_shared<JsonValue>(base::LoadFromString(value));
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LWARNING, ("Cannot create base::Json in line", lineNumber, ":", e.Msg()));
      continue;
    }

    if (!pred({key, json}))
      continue;

    if (m_cacheValuesCountLimit <= m_values.size())
      m_values.emplace(key, std::move(value));
    else
      m_values.emplace(key, std::move(json));
  }

  m_storage.clear();
}

// static
bool KeyValueStorage::ParseKeyValueLine(std::string const & line, std::streamoff lineNumber,
                                        uint64_t & key, std::string & value)
{
  auto const pos = line.find(" ");
  if (pos == std::string::npos)
  {
    LOG(LWARNING, ("Cannot find separator in line", lineNumber));
    return false;
  }

  std::string idStr = line.substr(0, pos);

  if (!strings::to_uint64(idStr, key, 16))
  {
    LOG(LWARNING, ("Cannot parse id", line.substr(0, pos), "in line", lineNumber));
    return false;
  }

  value = line.c_str() + pos + 1;
  return true;
}

// static
std::string KeyValueStorage::SerializeFullLine(uint64_t key, JsonValue && value)
{
  auto json = Serialize(value);
  CHECK(!json.empty(), ());

  std::stringstream result;
  result << SerializeDref(key) << " " << json << "\n";
  return result.str();
}

void KeyValueStorage::Insert(uint64_t key, JsonValue && value)
{
  auto json = Serialize(value);

  CHECK(!json.empty(), ());

  auto emplaceResult = m_values.emplace(key, std::move(json));

  if (!emplaceResult.second)  // it is ok for OSM relation with several outer borders
    return;

  auto const & emplaceIterator = emplaceResult.first;
  auto const & result = boost::get<std::string>(emplaceIterator->second);

  m_storage << SerializeDref(key) << " " << result << "\n";
}

std::shared_ptr<JsonValue> KeyValueStorage::Find(uint64_t key) const
{
  auto const it = m_values.find(key);
  if (it == std::end(m_values))
    return {};

  if (auto json = boost::get<std::shared_ptr<JsonValue>>(&it->second))
    return *json;

  auto const & jsonString = boost::get<std::string>(it->second);

  auto json = std::make_shared<JsonValue>(base::LoadFromString(jsonString));
  CHECK(json, ());
  return json;
}

std::string KeyValueStorage::SerializeDref(uint64_t number)
{
  std::stringstream stream;
  stream << std::setw(16) << std::setfill('0') << std::hex << std::uppercase << number;

  return stream.str();
}

size_t KeyValueStorage::Size() const { return m_values.size(); }
}  // namespace generator
