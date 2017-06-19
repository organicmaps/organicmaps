#import "Framework.h"
#import "MWMUGCReviewVM.h"
#import "SwiftBridge.h"

#include "ugc/api.hpp"

#include <vector>

@interface MWMUGCReviewVM () <MWMUGCSpecificReviewDelegate, MWMUGCTextReviewDelegate>
{
  ugc::UGCUpdate m_ugc;
  FeatureID m_fid;
  std::vector<ugc_review::Row> m_rows;
}

@property(copy, nonatomic) NSString * review;
@property(copy, nonatomic) NSString * name;
@property(nonatomic) NSInteger starCount;

@end

@implementation MWMUGCReviewVM

+ (instancetype)fromUGC:(ugc::UGCUpdate const &)ugc
              featureId:(FeatureID const &)fid
                   name:(NSString *)name
{
  auto inst = [[MWMUGCReviewVM alloc] init];
  inst.name = name;
  inst->m_ugc = ugc;
  inst->m_fid = fid;
  [inst config];
  return inst;
}

- (void)config
{
  auto & records = m_ugc.m_ratings.m_ratings;
//  Uncomment for testing.
  records.emplace_back("Price", 0);
  records.emplace_back("Place", 0);

  m_rows.insert(m_rows.begin(), records.size(), ugc_review::Row::Detail);
  m_rows.emplace_back(ugc_review::Row::Message);
}

- (void)setDefaultStarCount:(NSInteger)starCount
{
  for (auto & r : m_ugc.m_ratings.m_ratings)
    r.m_value = starCount;
}

- (void)submit
{
  GetFramework().GetUGCApi().SetUGCUpdate(m_fid, m_ugc);
}

- (NSInteger)numberOfRows { return m_rows.size(); }

- (ugc::RatingRecord const &)recordForIndexPath:(NSIndexPath *)indexPath
{
  return m_ugc.m_ratings.m_ratings[indexPath.row];
}

- (ugc_review::Row)rowForIndexPath:(NSIndexPath *)indexPath { return m_rows[indexPath.row]; }

#pragma mark - MWMUGCSpecificReviewDelegate

- (void)changeReviewRate:(NSInteger)rate atIndexPath:(NSIndexPath *)indexPath
{
  auto & record = m_ugc.m_ratings.m_ratings[indexPath.row];
  record.m_value = rate;
}

#pragma mark - MWMUGCTextReviewDelegate

- (void)changeReviewText:(NSString *)text
{
  self.review = text;
  // TODO: Write the review into ugc object.
}

@end
