#import <CoreApi/MWMNetworkPolicy.h>
#import <CoreSpotlight/CoreSpotlight.h>
#import <XCTest/XCTest.h>
#import "MWMSearch+CoreSpotlight.h"

@interface MWMSearch (CoreSpotlightTesting)
+ (NSArray<CSSearchableItem *> *)spotlightItemsForLanguageIds:(NSArray<NSString *> *)languageIds;
+ (void)updateSpotlightIndex:(CSSearchableIndex *)index languageIds:(NSArray<NSString *> *)languageIds;
@end

@interface MockSpotlightIndex : CSSearchableIndex
@property(nonatomic) NSUInteger deletionCount;
@property(nonatomic) NSUInteger indexingCount;
@property(nonatomic, copy) void (^deletionCompletion)(NSError *);
@property(nonatomic, copy) void (^indexingCompletion)(NSError *);
@end

@implementation MockSpotlightIndex

- (void)deleteSearchableItemsWithDomainIdentifiers:(NSArray<NSString *> *)domainIdentifiers
                                 completionHandler:(void (^)(NSError *))completionHandler
{
  NSAssert(NSThread.isMainThread, @"Index operations must be serialized on main");
  self.deletionCount++;
  self.deletionCompletion = completionHandler;
}

- (void)indexSearchableItems:(NSArray<CSSearchableItem *> *)items
           completionHandler:(void (^)(NSError *))completionHandler
{
  NSAssert(NSThread.isMainThread, @"Index operations must be serialized on main");
  self.indexingCount++;
  self.indexingCompletion = completionHandler;
}

@end

@interface MWMSearchSpotlightTests : XCTestCase
@property(nonatomic) MockSpotlightIndex * index;
@end

@implementation MWMSearchSpotlightTests

- (void)setUp
{
  [super setUp];
  XCTAssertTrue(NSThread.isMainThread);
  self.index = [[MockSpotlightIndex alloc] initWithName:@"SpotlightTests"];
}

- (void)completeIndexOperation:(BOOL)indexing error:(NSError *)error
{
  void (^completion)(NSError *) = indexing ? self.index.indexingCompletion : self.index.deletionCompletion;
  XCTAssertNotNil(completion);
  if (!completion)
    return;
  if (indexing)
    self.index.indexingCompletion = nil;
  else
    self.index.deletionCompletion = nil;
  XCTestExpectation * finished = [self expectationWithDescription:@"Index callback processed on main"];
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    completion(error);
    dispatch_async(dispatch_get_main_queue(), ^{ [finished fulfill]; });
  });
  [self waitForExpectations:@[finished] timeout:5];
}

- (void)updateForLanguages:(NSArray<NSString *> *)languages
{
  [MWMSearch updateSpotlightIndex:self.index languageIds:languages];
}

- (void)testFirstUpdateRebuildsThenUnchangedStateSkips
{
  [self updateForLanguages:@[@"en-US"]];
  XCTAssertEqual(self.index.deletionCount, 1u);
  [self completeIndexOperation:NO error:nil];
  [self completeIndexOperation:YES error:nil];
  [self updateForLanguages:@[@"en-US"]];
  XCTAssertEqual(self.index.deletionCount, 1u);
}

- (void)testEachIndexTracksSuccessfulLanguagesIndependently
{
  [self updateForLanguages:@[@"en-US"]];
  XCTAssertEqual(self.index.deletionCount, 1u);
  [self completeIndexOperation:NO error:nil];
  [self completeIndexOperation:YES error:nil];

  self.index = [[MockSpotlightIndex alloc] initWithName:@"OtherSpotlightIndex"];
  [self updateForLanguages:@[@"en-US"]];
  XCTAssertEqual(self.index.deletionCount, 1u);
  [self completeIndexOperation:NO error:nil];
  [self completeIndexOperation:YES error:nil];
}

- (void)testChangedLanguagesReindex
{
  [self updateForLanguages:@[@"en-US"]];
  XCTAssertEqual(self.index.deletionCount, 1u);
  [self completeIndexOperation:NO error:nil];
  [self completeIndexOperation:YES error:nil];

  [self updateForLanguages:@[@"en-US", @"pl-PL"]];
  XCTAssertEqual(self.index.deletionCount, 2u);
  [self completeIndexOperation:NO error:nil];
  [self completeIndexOperation:YES error:nil];
}

- (void)testUpdatesDoNotOverlapAndCanRunAgainAfterSuccess
{
  [self updateForLanguages:@[@"en-US"]];
  [self updateForLanguages:@[@"pl-PL"]];
  XCTAssertEqual(self.index.deletionCount, 1u);
  [self completeIndexOperation:NO error:nil];
  [self updateForLanguages:@[@"pl-PL"]];
  XCTAssertEqual(self.index.deletionCount, 1u);
  [self completeIndexOperation:YES error:nil];
  [self updateForLanguages:@[@"pl-PL"]];
  XCTAssertEqual(self.index.deletionCount, 2u);
  [self completeIndexOperation:NO error:nil];
  [self completeIndexOperation:YES error:nil];
}

- (void)testDeletionFailureAllowsRetry
{
  [self updateForLanguages:@[@"en-US"]];
  NSError * error = [NSError errorWithDomain:@"SpotlightTests" code:1 userInfo:nil];
  [self completeIndexOperation:NO error:error];
  XCTAssertEqual(self.index.indexingCount, 0u);
  [self updateForLanguages:@[@"en-US"]];
  XCTAssertEqual(self.index.deletionCount, 2u);
  [self completeIndexOperation:NO error:nil];
  [self completeIndexOperation:YES error:nil];
}

- (void)testIndexingFailureAllowsRetry
{
  [self updateForLanguages:@[@"en-US"]];
  [self completeIndexOperation:NO error:nil];
  NSError * error = [NSError errorWithDomain:@"SpotlightTests" code:1 userInfo:nil];
  [self completeIndexOperation:YES error:error];
  [self updateForLanguages:@[@"en-US"]];
  XCTAssertEqual(self.index.deletionCount, 2u);
  [self completeIndexOperation:NO error:nil];
  [self completeIndexOperation:YES error:nil];
}

- (void)testOnlyPreferredLanguageCategoryNamesAreIndexed
{
  NSArray<CSSearchableItem *> * items = [MWMSearch spotlightItemsForLanguageIds:@[@"en-US", @"ru-PL", @"pl-PL"]];
  NSMutableSet<NSString *> * titles = [NSMutableSet set];
  for (CSSearchableItem * item in items)
    if ([item.uniqueIdentifier hasPrefix:@"category_eat:"])
      [titles addObject:item.attributeSet.title];
  XCTAssertEqualObjects(titles, ([NSSet setWithArray:@[@"Where to eat", @"Где поесть", @"Gdzie zjeść"]]));
  for (NSString * synonym in @[@"eat", @"Food", @"Поесть", @"Еда", @"Jedzenie"])
    XCTAssertFalse([titles containsObject:synonym], @"Unexpected Spotlight synonym: %@", synonym);
}

- (void)testEveryLocalizedCategoryNameOpensItsCategory
{
  NSArray<CSSearchableItem *> * items = [MWMSearch spotlightItemsForLanguageIds:@[@"en-US", @"ru-PL", @"pl-PL"]];
  NSMutableSet<NSString *> * identifiers = [NSMutableSet set];
  for (CSSearchableItem * item in items)
  {
    XCTAssertGreaterThan(item.attributeSet.title.length, 0u);
    XCTAssertGreaterThan(item.attributeSet.thumbnailData.length, 0u);
    NSString * categoryKey = [MWMSearch categoryKeyForSpotlightIdentifier:item.uniqueIdentifier];
    XCTAssertNotNil(categoryKey);
    NSString * prefix = [categoryKey stringByAppendingString:@":"];
    XCTAssertTrue([item.uniqueIdentifier hasPrefix:prefix]);
    NSString * locale = [item.uniqueIdentifier substringFromIndex:prefix.length];
    SearchQuery * query = [MWMSearch searchQueryForSpotlightIdentifier:item.uniqueIdentifier];
    XCTAssertEqualObjects([(id)query valueForKey:@"locale"], locale);
    XCTAssertEqualObjects([(id)query valueForKey:@"text"], [item.attributeSet.title stringByAppendingString:@" "]);
    XCTAssertEqual([[(id)query valueForKey:@"source"] unsignedIntegerValue], SearchTextSourceCategory);
    XCTAssertEqualObjects(item.attributeSet.displayName, item.attributeSet.title);
    [identifiers addObject:item.uniqueIdentifier];
  }
  XCTAssertEqual(identifiers.count, items.count);
}

- (void)testSharedSynonymsAreNotIndexed
{
  NSArray * items = [MWMSearch spotlightItemsForLanguageIds:@[@"ru-PL"]];
  NSDictionary * itemsByIdentifier = [NSDictionary dictionaryWithObjects:items
                                                                 forKeys:[items valueForKey:@"uniqueIdentifier"]];
  CSSearchableItem * eat = itemsByIdentifier[@"category_eat:ru"];
  CSSearchableItem * food = itemsByIdentifier[@"category_food:ru"];
  XCTAssertEqualObjects(eat.attributeSet.title, @"Где поесть");
  XCTAssertEqualObjects(food.attributeSet.title, @"Продукты");
  XCTAssertNotEqualObjects(eat.attributeSet.title, @"Еда");
  XCTAssertNotEqualObjects(food.attributeSet.title, @"Еда");
}

- (void)testLocalizedTitlesMatchInAppCategoryLabels
{
  for (NSString * locale in @[@"de", @"es"])
  {
    NSString * path = [NSBundle.mainBundle pathForResource:locale ofType:@"lproj"];
    XCTAssertNotNil(path);
    NSBundle * bundle = [NSBundle bundleWithPath:path];
    XCTAssertNotNil(bundle);
    for (CSSearchableItem * item in [MWMSearch spotlightItemsForLanguageIds:@[locale]])
    {
      NSString * key = [MWMSearch categoryKeyForSpotlightIdentifier:item.uniqueIdentifier];
      NSString * label = [bundle localizedStringForKey:key value:nil table:nil];
      XCTAssertEqualObjects(item.attributeSet.title, label, @"%@ %@", locale, key);
    }
  }
}

- (void)testLanguageVariantsDoNotDuplicateItems
{
  NSArray * english = [MWMSearch spotlightItemsForLanguageIds:@[@"en-US"]];
  NSArray * variants = [MWMSearch spotlightItemsForLanguageIds:@[@"en-US", @"en-GB"]];
  XCTAssertEqualObjects([english valueForKey:@"uniqueIdentifier"], [variants valueForKey:@"uniqueIdentifier"]);
}

- (void)testRegionalLanguageUsesExactLocaleBeforeBaseLocale
{
  NSArray * brazilianPortuguese = [MWMSearch spotlightItemsForLanguageIds:@[@"pt-BR"]];
  NSArray * portuguese = [MWMSearch spotlightItemsForLanguageIds:@[@"pt-PT"]];
  NSDictionary * brazilianItems =
      [NSDictionary dictionaryWithObjects:brazilianPortuguese
                                  forKeys:[brazilianPortuguese valueForKey:@"uniqueIdentifier"]];
  NSDictionary * portugueseItems = [NSDictionary dictionaryWithObjects:portuguese
                                                               forKeys:[portuguese valueForKey:@"uniqueIdentifier"]];

  XCTAssertEqualObjects([brazilianItems[@"category_food:pt-BR"] attributeSet].title, @"Mercados");
  XCTAssertNil(brazilianItems[@"category_food:pt"]);
  XCTAssertEqualObjects([portugueseItems[@"category_food:pt"] attributeSet].title, @"Mercearias");
  XCTAssertNil(portugueseItems[@"category_food:pt-BR"]);
}

- (void)testChineseLanguageUsesScriptLocale
{
  NSArray * traditional = [MWMSearch spotlightItemsForLanguageIds:@[@"zh-TW"]];
  NSArray * simplified = [MWMSearch spotlightItemsForLanguageIds:@[@"zh-CN"]];
  NSDictionary * traditionalItems = [NSDictionary dictionaryWithObjects:traditional
                                                                forKeys:[traditional valueForKey:@"uniqueIdentifier"]];
  NSDictionary * simplifiedItems = [NSDictionary dictionaryWithObjects:simplified
                                                               forKeys:[simplified valueForKey:@"uniqueIdentifier"]];
  XCTAssertEqualObjects([traditionalItems[@"category_eat:zh-Hant"] attributeSet].title, @"在哪裡吃");
  XCTAssertEqualObjects([simplifiedItems[@"category_eat:zh-Hans"] attributeSet].title, @"在哪儿吃");
}

- (void)testUnsupportedAndEmptyLanguageListsFallBackToEnglish
{
  NSArray * english = [MWMSearch spotlightItemsForLanguageIds:@[@"en-US"]];
  for (NSArray * languages in @[@[], @[@"zz-ZZ"]])
  {
    NSArray * fallback = [MWMSearch spotlightItemsForLanguageIds:languages];
    XCTAssertEqualObjects([english valueForKey:@"uniqueIdentifier"], [fallback valueForKey:@"uniqueIdentifier"]);
  }
}

- (void)testUnsupportedPrimaryLanguageOpensIndexedEnglishCategory
{
  NSArray<CSSearchableItem *> * items = [MWMSearch spotlightItemsForLanguageIds:@[@"az"]];
  NSPredicate * predicate = [NSPredicate predicateWithFormat:@"uniqueIdentifier == %@", @"category_eat:en"];
  CSSearchableItem * item = [items filteredArrayUsingPredicate:predicate].firstObject;
  XCTAssertEqualObjects(item.attributeSet.title, @"Where to eat");

  SearchQuery * query = [MWMSearch searchQueryForSpotlightIdentifier:item.uniqueIdentifier];
  XCTAssertEqualObjects([(id)query valueForKey:@"text"], @"Where to eat ");
  XCTAssertEqualObjects([(id)query valueForKey:@"locale"], @"en");
}

- (void)testIdentifiersAreStableWhenLanguagesAreReordered
{
  NSArray * first = [MWMSearch spotlightItemsForLanguageIds:@[@"en-US", @"ru-PL", @"pl-PL"]];
  NSArray * reordered = [MWMSearch spotlightItemsForLanguageIds:@[@"pl-PL", @"ru-PL", @"en-US"]];
  XCTAssertEqualObjects([NSSet setWithArray:[first valueForKey:@"uniqueIdentifier"]],
                        [NSSet setWithArray:[reordered valueForKey:@"uniqueIdentifier"]]);
}

- (void)testLegacyIdentifiersStillOpenTheirCategory
{
  XCTAssertEqualObjects([MWMSearch categoryKeyForSpotlightIdentifier:@"category_eat"], @"category_eat");
  XCTAssertEqualObjects([MWMSearch categoryKeyForSpotlightIdentifier:@"category_eat:ru"], @"category_eat");
  XCTAssertEqualObjects([MWMSearch categoryKeyForSpotlightIdentifier:@"category_eat:Где поесть"], @"category_eat");
  XCTAssertNil([MWMSearch categoryKeyForSpotlightIdentifier:@"unknown:Где поесть"]);
  XCTAssertNil([MWMSearch categoryKeyForSpotlightIdentifier:@""]);
  XCTAssertNil([MWMSearch categoryKeyForSpotlightIdentifier:nil]);
}

@end
