#include "testing/testing.hpp"

#include "base/url_helpers.hpp"

UNIT_TEST(Url_Join)
{
  TEST_EQUAL("", base::url::Join("", ""), ());
  TEST_EQUAL("omim/", base::url::Join("", "omim/"), ());
  TEST_EQUAL("omim/", base::url::Join("omim/", ""), ());
  TEST_EQUAL("omim/strings", base::url::Join("omim", "strings"), ());
  TEST_EQUAL("omim/strings", base::url::Join("omim/", "strings"), ());
  TEST_EQUAL("../../omim/strings", base::url::Join("..", "..", "omim", "strings"), ());
  TEST_EQUAL("../../omim/strings", base::url::Join("../", "..", "omim/", "strings"), ());
  TEST_EQUAL("omim/strings", base::url::Join("omim/", "/strings"), ());
  TEST_EQUAL("../../omim/strings", base::url::Join("../", "/../", "/omim/", "/strings"), ());
  TEST_EQUAL("../omim/strings", base::url::Join("../", "", "/omim/", "/strings"), ());
}
