#include "query_saver.hpp"

#include "platform/settings.hpp"

#include "coding/base64.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"
#include "coding/write_to_sink.hpp"

#include "base/logging.hpp"

#include "std/limits.hpp"

namespace
{
char constexpr kSettingsKey[] = "UserQueries";
using TLength = uint16_t;
TLength constexpr kMaxSuggestionsCount = 10;

// Reader from memory that throws exceptions.
class SecureMemReader : public Reader
{
  void CheckPosAndSize(uint64_t pos, uint64_t size) const
  {
    if (pos + size > m_size || size > numeric_limits<size_t>::max())
      MYTHROW(SizeException, (pos, size, m_size) );
  }

public:
  // Construct from block of memory.
  SecureMemReader(void const * pData, size_t size)
    : m_pData(static_cast<char const *>(pData)), m_size(size)
  {
  }

  inline uint64_t Size() const
  {
    return m_size;
  }

  inline void Read(uint64_t pos, void * p, size_t size) const
  {
    CheckPosAndSize(pos, size);
    memcpy(p, m_pData + pos, size);
  }

  inline SecureMemReader SubReader(uint64_t pos, uint64_t size) const
  {
    CheckPosAndSize(pos, size);
    return SecureMemReader(m_pData + pos, static_cast<size_t>(size));
  }

  inline SecureMemReader * CreateSubReader(uint64_t pos, uint64_t size) const
  {
    CheckPosAndSize(pos, size);
    return new SecureMemReader(m_pData + pos, static_cast<size_t>(size));
  }

private:
  char const * m_pData;
  size_t m_size;
};

}  // namespace

namespace search
{
QuerySaver::QuerySaver()
{
  Load();
}

void QuerySaver::Add(string const & query)
{
  if (query.empty())
    return;

  // Remove items if needed.
  auto const it = find(m_topQueries.begin(), m_topQueries.end(), query);
  if (it != m_topQueries.end())
    m_topQueries.erase(it);
  else if (m_topQueries.size() >= kMaxSuggestionsCount)
    m_topQueries.pop_back();

  // Add new query and save it to drive.
  m_topQueries.push_front(query);
  Save();
}

void QuerySaver::Clear()
{
  m_topQueries.clear();
  Settings::Delete(kSettingsKey);
}

void QuerySaver::Serialize(string & data) const
{
  vector<uint8_t> rawData;
  MemWriter<vector<uint8_t>> writer(rawData);
  TLength size = m_topQueries.size();
  WriteToSink(writer, size);
  for (auto const & query : m_topQueries)
  {
    size = query.size();
    WriteToSink(writer, size);
    writer.Write(query.c_str(), size);
  }
  data = base64::Encode(string(rawData.begin(), rawData.end()));
}

void QuerySaver::Deserialize(string const & data)
{
  string decodedData = base64::Decode(data);
  SecureMemReader rawReader(decodedData.c_str(), decodedData.size());
  ReaderSource<SecureMemReader> reader(rawReader);

  TLength queriesCount = ReadPrimitiveFromSource<TLength>(reader);
  queriesCount = min(queriesCount, kMaxSuggestionsCount);

  for (TLength i = 0; i < queriesCount; ++i)
  {
    TLength stringLength = ReadPrimitiveFromSource<TLength>(reader);
    vector<char> str(stringLength);
    reader.Read(&str[0], stringLength);
    m_topQueries.emplace_back(&str[0], stringLength);
  }
}

void QuerySaver::Save()
{
  string data;
  Serialize(data);
  Settings::Set(kSettingsKey, data);
}

void QuerySaver::Load()
{
  string hexData;
  Settings::Get(kSettingsKey, hexData);
  if (hexData.empty())
    return;
  try
  {
    Deserialize(hexData);
  }
  catch (Reader::SizeException const & /* exception */)
  {
    Clear();
    LOG(LWARNING, ("Search history data corrupted! Creating new one."));
  }
}
}  // namesapce search
