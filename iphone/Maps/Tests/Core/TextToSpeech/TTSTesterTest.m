#import <XCTest/XCTest.h>
#import "TTSTester.h"

@interface TTSTesterTest : XCTestCase

@end

@implementation TTSTesterTest

TTSTester * ttsTester;

- (void)setUp
{
  ttsTester = [[TTSTester alloc] init];
}

- (void)testTestStringsWithEnglish
{
  XCTAssertTrue([[ttsTester getTestStrings:@"en-US"] containsObject:@"Thank you for using our community-built maps!"]);
}

- (void)testTestStringsWithGerman
{
  XCTAssertTrue([[ttsTester getTestStrings:@"de-DE"]
      containsObject:@"Danke, dass du unsere von der Community erstellten Karten benutzt!"]);
}

- (void)testTestStringsWithInvalidLanguage
{
  XCTAssertNil([ttsTester getTestStrings:@"xxx"]);
}

@end
