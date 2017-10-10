#include "generator/ugc_db.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include <sqlite3.h>

#include <sstream>

namespace
{
struct Results
{
  bool empty = true;
  std::ostringstream values;
};
}  // namespace

namespace generator
{
static int callback(void * results_ptr, int argc, char ** argv, char ** azColName)
{
  Results & results = *reinterpret_cast<Results *>(results_ptr);
  for (size_t i = 0; i < argc; i++)
  {
    if (results.empty)
      results.empty = false;
    else
      results.values << ",";

    results.values << (argv[i] ? argv[i] : "{}");
  }

  return 0;
}

UGCDB::UGCDB(std::string const & path)
{
  int rc = sqlite3_open(path.c_str(), &m_db);
  if (rc)
  {
    LOG(LERROR, ("Can't open database:", sqlite3_errmsg(m_db)));
    sqlite3_close(m_db);
    m_db = nullptr;
  }
}

UGCDB::~UGCDB()
{
  if (m_db)
    sqlite3_close(m_db);
}

bool UGCDB::Get(osm::Id const & id, std::vector<uint8_t> & blob)
{
  if (!m_db)
    return false;
  Results results;
  results.values << "[";

  std::ostringstream cmd;
  cmd << "SELECT value FROM ratings WHERE key=" << id.EncodedId() << ";";

  char * zErrMsg = nullptr;
  auto rc = sqlite3_exec(m_db, cmd.str().c_str(), callback, &results, &zErrMsg);
  if (rc != SQLITE_OK)
  {
    LOG(LERROR, ("SQL error:", zErrMsg));
    sqlite3_free(zErrMsg);
    return false;
  }
  results.values << "]";

  return ValueToBlob(results.values.str(), blob);
}

bool UGCDB::Exec(std::string const & statement)
{
  if (!m_db)
    return false;

  char * zErrMsg = nullptr;
  auto rc = sqlite3_exec(m_db, statement.c_str(), nullptr, nullptr, &zErrMsg);
  if (rc != SQLITE_OK)
  {
    LOG(LERROR, ("SQL error:", zErrMsg));
    sqlite3_free(zErrMsg);
    return false;
  }
  return true;
}

bool UGCDB::ValueToBlob(std::string const & src, std::vector<uint8_t> & blob)
{
  blob.assign(src.cbegin(), src.cend());
  return true;
}

}  // namespace generator
