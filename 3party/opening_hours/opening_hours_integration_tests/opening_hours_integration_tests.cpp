#include "parse_opening_hours.hpp"
#include "rules_evaluation.hpp"

#define BOOST_TEST_MODULE OpeningHoursIntegration

#include <algorithm>
#include <array>
#include <ctime>
#include <map>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/test/included/unit_test.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace
{
template <typename T>
std::string ToString(T const & t)
{
  std::stringstream sstr;
  sstr << t;
  return sstr.str();
}

template <typename T>
bool HasPeriod(std::vector<T> const & v)
{
  auto const hasPeriod = [](T const & t) { return t.HasPeriod(); };
  return std::any_of(begin(v), end(v), hasPeriod);
}

template <typename T>
bool HasPlus(std::vector<T> const & v)
{
  auto const hasPlus = [](T const & t) { return t.HasPlus(); };
  return std::any_of(begin(v), end(v), hasPlus);
}

bool HasOffset(osmoh::TMonthdayRanges const & mr)
{
  auto const hasOffset = [](osmoh::MonthdayRange const & md) {
    return
        md.GetStart().HasOffset() ||
        md.GetEnd().HasOffset();
  };
  return std::any_of(begin(mr), end(mr), hasOffset);
}

bool HasOffset(osmoh::Weekdays const & wd)
{
  auto const hasOffset = [](osmoh::WeekdayRange const & w) { return w.HasOffset(); };
  return std::any_of(begin(wd.GetWeekdayRanges()), end(wd.GetWeekdayRanges()), hasOffset);
}

template <typename ParserResult>
bool CompareNormalized(std::string const & original, ParserResult const & pretendent)
{
  auto originalCopy = original;
  auto pretendentCopy = ToString(pretendent);

  boost::to_lower(originalCopy);
  boost::to_lower(pretendentCopy);

  boost::replace_all(originalCopy, "off", "closed");

  boost::replace_all(originalCopy, " ", "");
  boost::replace_all(pretendentCopy, " ", "");

  return pretendentCopy == originalCopy;
}

enum
{
  Parsed,
  Serialised,
  Period,
  Plus,
  Offset,
  Count_
};
using TRuleFeatures = std::array<bool, Count_>;

std::ostream & operator<<(std::ostream & ost, TRuleFeatures const & f)
{
 std::copy(std::begin(f), std::end(f), std::ostream_iterator<bool>(ost, "\t"));
 return ost;
}

TRuleFeatures DescribeRule(osmoh::TRuleSequences const & rule)
{
  TRuleFeatures features{};
  for (auto const & r : rule)
  {
    features[Period] |= HasPeriod(r.GetTimes());
    features[Period] |= HasPeriod(r.GetMonths());
    features[Period] |= HasPeriod(r.GetYears());
    features[Period] |= HasPeriod(r.GetWeeks());

    features[Plus] |= HasPlus(r.GetTimes());
    features[Plus] |= HasPlus(r.GetMonths());
    features[Plus] |= HasPlus(r.GetYears());

    features[Offset] |= HasOffset(r.GetMonths());
    features[Offset] |= HasOffset(r.GetWeekdays());
  }

  return features;
}
} // namespace

/// How to run:
/// 1. copy opening-count.lst to where the binary is
/// 2. run with --log_level=message
BOOST_AUTO_TEST_CASE(OpeningHours_CountFailed)
{
  std::ifstream datalist("opening-count.lst");
  BOOST_REQUIRE_MESSAGE(datalist.is_open(),
                        "Can't open ./opening-count.lst: " << std::strerror(errno));

  std::string line;

  size_t line_num = 0;
  size_t num_failed = 0;
  size_t num_total = 0;

  std::map<size_t, size_t> hist;
  std::map<TRuleFeatures, size_t> featuresDistrib;

  while (std::getline(datalist, line))
  {
    size_t count = 1;
    std::string datastr;

    auto d = line.find('|');
    if (d == std::string::npos)
    {
      BOOST_WARN_MESSAGE((d != std::string::npos),
                         "Incorrect line " << line_num << " format: " << line);
      datastr = line;
    }
    else
    {
      count = std::stol(line.substr(0, d));
      datastr = line.substr(d + 1);
    }

    line_num++;

    osmoh::TRuleSequences rule;
    auto const isParsed = Parse(datastr, rule);
    TRuleFeatures features{};

    if (isParsed)
      features = DescribeRule(rule);
    features[Parsed] = true;
    features[Serialised] = true;

    if (!isParsed)
    {
      num_failed += count;
      ++hist[count];
      features[Parsed] = false;
      features[Serialised] = false;
      BOOST_TEST_MESSAGE("-- " << count << " :[" << datastr << "]");
    }
    else if (!CompareNormalized(datastr, rule))
    {
      num_failed += count;
      ++hist[count];
      features[Serialised] = false;
      BOOST_TEST_MESSAGE("- " << count << " :[" << datastr << "]");
      BOOST_TEST_MESSAGE("+ " << count << " :[" << ToString(rule) << "]");
    }

    featuresDistrib[features] += count;
    num_total += count;
  }

  BOOST_CHECK_MESSAGE((num_failed == 0),
                      "Failed " << num_failed <<
                      " of " << num_total <<
                      " (" << double(num_failed)/(double(num_total)/100) << "%)");

  {
    std::stringstream message;
    for (auto const & e : hist)
      message << "Weight: " << e.first << " Count: " << e.second << std::endl;

    BOOST_TEST_MESSAGE(message.str());
  }
  {
    std::stringstream message;
    message << "Parsed\tSerialised\tPeriod\tPlus\tOffset\tCount" << std::endl;
    for (auto const & e : featuresDistrib)
      message << e.first  << '\t' << e.second << std::endl;

    BOOST_TEST_MESSAGE(message.str());
  }
}
