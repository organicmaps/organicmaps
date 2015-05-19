//
//  TestsOnDeviceTests.m
//  TestsOnDeviceTests
//
//  Created by Sergey Yershov on 23.03.15.
//  Copyright (c) 2015 Sergey Yershov. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include "osm_time_range.hpp"

#include <chrono>
#include <iomanip>
#include <ctime>
#include <string>
#include <sstream>

#include <boost/spirit/include/qi.hpp>

template <typename Char, typename Parser>
bool test(Char const* in, Parser const& p, bool full_match = true)
{
  // we don't care about the result of the "what" function.
  // we only care that all parsers have it:
  boost::spirit::qi::what(p);

  Char const* last = in;
  while (*last)
    last++;
  return boost::spirit::qi::parse(in, last, p) && (!full_match || (in == last));
}


@interface TestsOnDeviceTests : XCTestCase

@end

@implementation TestsOnDeviceTests

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testBoostSpirit {
  namespace charset = boost::spirit::standard_wide;
  namespace qi = boost::spirit::qi;

  class alltime_ : public qi::symbols<wchar_t>
  {
  public:
    alltime_()
    {
      add
      (L"пн")(L"uu")(L"œæ")
      ;
    }
  } alltime;

  XCTAssert(test(L"TeSt",charset::no_case[qi::lit("test")]), @"Passed");
  XCTAssert(test(L"Пн",charset::no_case[alltime]), @"Passed");
  XCTAssert(test(L"UU",charset::no_case[alltime]), @"Passed");
  XCTAssert(test(L"ŒÆ",charset::no_case[alltime]), @"Passed");
  XCTAssert(test("КАР",charset::no_case[charset::string(L"кар")]), @"Passed");
  //  BOOST_CHECK(test("крУглосуточно",charset::no_case[charset::char_(L"круглосуточно")]));

  std::cout << std::iswupper(L'К') << " " << boost::spirit::char_encoding::standard_wide::tolower(L'К') << " -- " << L'К' << std::endl;
  const wchar_t c = L'\u00de'; // capital letter thorn

  std::locale loc1("C");
  std::cout << "isupper('Þ', C locale) returned " << std::boolalpha << std::isupper(c, loc1) << '\n';

  std::locale loc2("ru_RU.UTF8");
  std::cout << "isupper('Þ', Unicode locale) returned " << std::boolalpha << std::isupper(c, loc2) << '\n';
}

- (void)testExample {
    // This is an example of a functional test case.
  OSMTimeRange oh("06:13-15:00; 16:30+");
  XCTAssert(oh.IsValid(), @"Incorrect schedule string");
  XCTAssert(oh("12-12-2013 7:00").IsOpen(), @"Passed");
  XCTAssert(oh("12-12-2013 16:00").IsClosed(), @"Passed");
  XCTAssert(oh("12-12-2013 20:00").IsOpen(), @"Passed");
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
//      OSMTimeRange oh("06:13-15:00; 16:30+");
//      oh.isValid();
//      (void)(oh.check(make_time("12-12-2013 7:00")) == osmoh::State::eOpen);
//      (void)(oh.check(make_time("12-12-2013 16:00")) == osmoh::State::eClosed);
//      (void)(oh.check(make_time("12-12-2013 20:00")) == osmoh::State::eOpen);
    }];
}

@end
