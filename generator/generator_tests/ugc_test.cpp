#include "testing/testing.hpp"

#include "generator/ugc_db.hpp"
#include "generator/ugc_translator.hpp"
#include "generator/osm_id.hpp"

#include "ugc/types.hpp"

#include "base/math.hpp"

std::string g_database(R"LLL(
                       PRAGMA foreign_keys=OFF;
                       BEGIN TRANSACTION;
                       CREATE TABLE ratings (key bigint, value blob);
                       INSERT INTO "ratings" VALUES(9826352,'{"osm_id":9826352,"total_rating":10.34,"votes":721,"ratings":[{"id":2,"value":3.4},{"id":2,"value":6.0001}],"reviews":[{"id":7864532,"text":"The best service on the Earth","lang":"en","author":"Robert","rating":8.5,"date":1234567}]}');
                       INSERT INTO "ratings" VALUES(9826353,'{"osm_id":9826353,"total_rating":0.34,"votes":1,"ratings":[{"id":2,"value":3.4},{"id":3,"value":6.0001},{"id":6,"value":0.0001}],"reviews":[{"id":78645323924,"text":"Изумительно!","lang":"ru","author":"Вася","rating":10,"date":1234569}]}');
                       CREATE INDEX key_index ON ratings (key);
                       COMMIT;
)LLL");

UNIT_TEST(UGC_SmokeTest)
{
  generator::UGCDB db(":memory:");
  bool create = db.Exec(g_database);
  TEST(create, ("Can't open database"));
  osm::Id id = osm::Id(1);
  std::vector<uint8_t> blob;
  bool rc = db.Get(id, blob);
  TEST(rc, ("Can't load data for", id));
  std::string result(blob.cbegin(), blob.cend());
  std::cout << result << std::endl;
}

UNIT_TEST(UGC_TranslateRatingTest)
{
  generator::UGCTranslator tr;
  tr.CreateDb(g_database);
  osm::Id id = osm::Id(9826352);

  ugc::UGC ugc;
  bool rc = tr.TranslateUGC(id, ugc);
  TEST(rc, ("Can't translate rating for", id));

  TEST_EQUAL(ugc.m_ratings.size(), 2, ());
  TEST_EQUAL(ugc.m_ratings[0].m_key, "2", ());
  TEST(my::AlmostEqualAbs(ugc.m_ratings[0].m_value, 3.4f, 1e-6f), ());
}
