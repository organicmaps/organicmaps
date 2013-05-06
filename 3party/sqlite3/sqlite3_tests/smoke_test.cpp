#include "../../../testing/testing.hpp"

#include "../../../base/scope_guard.hpp"
#include "../../../base/logging.hpp"

#include "../../../coding/file_writer.hpp"

#include "../../../std/bind.hpp"

#include "../sqlite3.h"

static int callback(void * NotUsed, int argc, char ** argv, char ** azColName)
{
//  for(int i = 0; i < argc; ++i)
//    LOG(LINFO, (azColName[i], '=', argv[i] ? argv[i] : "NULL"));
  return 0;
}

UNIT_TEST(Sqlite_Smoke_C)
{
  char const * DBFILE = "sqlite3_test.db";
  sqlite3 * db;
  int rc = ::sqlite3_open(DBFILE, &db);
  MY_SCOPE_GUARD(SqliteDbClose, bind(&::sqlite3_close, db));
  TEST_EQUAL(rc, SQLITE_OK, ("Can't open database", DBFILE, sqlite3_errmsg(db)));
  MY_SCOPE_GUARD(DeleteFileAtTheEnd, bind(&FileWriter::DeleteFileX, DBFILE));

  char * zErrMsg = 0;
  rc = sqlite3_exec(db, "CREATE TABLE logs (stamp VARCHAR, user VARCHAR, msg TEXT);", callback, 0, &zErrMsg);
  TEST_EQUAL(rc, SQLITE_OK, ("Can't create table", zErrMsg));
  if (zErrMsg)
  {
    ::sqlite3_free(zErrMsg);
    zErrMsg = 0;
  }

  rc = sqlite3_exec(db,
                    "INSERT INTO logs (stamp, user, msg) VALUES ('05:00:36', 'shapr', 'hi hi!');" \
                    "INSERT INTO logs (stamp, user, msg) VALUES ('05:53:23', 'shapr', 'hi!');" \
                    "INSERT INTO logs (stamp, user, msg) VALUES ('05:53:27', 'delYsid', 'hi');" \
                    "INSERT INTO logs (stamp, user, msg) VALUES ('05:54:30', 'shapr', 'what''s up? :)');" \
                    "INSERT INTO logs (stamp, user, msg) VALUES ('05:55:47', 'delYsid', 'I am sitting at work, having nothing todo, and my stumach hurts like someone gave me H2SO4 for breakfast');" \
                    "INSERT INTO logs (stamp, user, msg) VALUES ('05:55:59', 'delYsid', 's/stumach/stomach/');" \
                    "INSERT INTO logs (stamp, user, msg) VALUES ('05:56:00', 'shapr', 'hydrogen sulfate?');",
                    callback, 0, &zErrMsg);
  TEST_EQUAL(rc, SQLITE_OK, ("INSERT failed", zErrMsg));
  if (zErrMsg)
  {
    ::sqlite3_free(zErrMsg);
    zErrMsg = 0;
  }

  rc = sqlite3_exec(db, "SELECT * FROM logs;", callback, 0, &zErrMsg);
  TEST_EQUAL(rc, SQLITE_OK, ("SELECT * failed", zErrMsg));
  if (zErrMsg)
  {
    ::sqlite3_free(zErrMsg);
    zErrMsg = 0;
  }
}

