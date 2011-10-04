#include "../../testing/testing.hpp"

#include "../url_generator.hpp"

#include "../../std/algorithm.hpp"

void VectorContains(vector<string> & v, string const & s)
{
  vector<string>::iterator found = find(v.begin(), v.end(), s);
  std::cout << s << endl;
  TEST(found != v.end(), (s, "was not found in", v));
  v.erase(found);
}

UNIT_TEST(UrlGenerator)
{
  vector<string> first;
  first.push_back("A");
  first.push_back("B");
  first.push_back("C");
  first.push_back("D");
  first.push_back("E");

  vector<string> second;
  second.push_back("F");
  second.push_back("G");
  second.push_back("H");

  UrlGenerator g(first, second);

  {
    size_t const count = first.size();
    for (size_t i = 0; i < count; ++i)
      VectorContains(first, g.PopNextUrl());
  }
  {
    size_t const count = second.size();
    for (size_t i = 0; i < count; ++i)
      VectorContains(second, g.PopNextUrl());
  }
}
