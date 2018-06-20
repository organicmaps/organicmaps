#include "string_storage_base.hpp"

#include "coding/reader_streambuf.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "base/logging.hpp"
#include "base/exception.hpp"
#include "base/stl_add.hpp"

#include <istream>

using namespace std;

namespace
{
constexpr char kDelimChar = '=';
}  // namespace

namespace platform
{
StringStorageBase::StringStorageBase(string const & path) : m_path(path)
{
  try
  {
    LOG(LINFO, ("Settings path:", m_path));
    ReaderStreamBuf buffer(make_unique<FileReader>(m_path));
    istream stream(&buffer);

    string line;
    while (getline(stream, line))
    {
      if (line.empty())
        continue;

      size_t const delimPos = line.find(kDelimChar);
      if (delimPos == string::npos)
        continue;

      string const key = line.substr(0, delimPos);
      string const value = line.substr(delimPos + 1);
      if (!key.empty() && !value.empty())
        m_values[key] = value;
    }
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Loading settings:", ex.Msg()));
  }
}

void StringStorageBase::Save() const
{
  try
  {
    FileWriter file(m_path);
    for (auto const & value : m_values)
    {
      string line(value.first);
      line += kDelimChar;
      line += value.second;
      line += '\n';
      file.Write(line.data(), line.size());
    }
  }
  catch (RootException const & ex)
  {
    // Ignore all settings saving exceptions.
    LOG(LWARNING, ("Saving settings:", ex.Msg()));
  }
}

void StringStorageBase::Clear()
{
  lock_guard<mutex> guard(m_mutex);
  m_values.clear();
  Save();
}

bool StringStorageBase::GetValue(string const & key, string & outValue) const
{
  lock_guard<mutex> guard(m_mutex);

  auto const found = m_values.find(key);
  if (found == m_values.end())
    return false;

  outValue = found->second;
  return true;
}

void StringStorageBase::SetValue(string const & key, string && value)
{
  lock_guard<mutex> guard(m_mutex);

  m_values[key] = move(value);
  Save();
}

void StringStorageBase::DeleteKeyAndValue(string const & key)
{
  lock_guard<mutex> guard(m_mutex);

  auto const found = m_values.find(key);
  if (found != m_values.end())
  {
    m_values.erase(found);
    Save();
  }
}
}  // namespace platform
