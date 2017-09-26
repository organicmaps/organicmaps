#import "MWMUGCReviewVM.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "ugc/types.hpp"

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
  // TODO: Config controller with ugc.
}

- (void)setDefaultStarCount:(NSInteger)starCount
{
  // TODO: Set stars count.
}

- (void)submit
{
  GetFramework().GetUGCApi()->SetUGCUpdate(m_fid, m_ugc);
}

- (NSInteger)numberOfRows { return m_rows.size(); }

- (ugc::RatingRecord const &)recordForIndexPath:(NSIndexPath *)indexPath
{
  // TODO: Return rating record from ugc.
  return ugc::RatingRecord();
}

- (ugc_review::Row)rowForIndexPath:(NSIndexPath *)indexPath { return m_rows[indexPath.row]; }

#pragma mark - MWMUGCSpecificReviewDelegate

- (void)changeReviewRate:(NSInteger)rate atIndexPath:(NSIndexPath *)indexPath
{
  //TODO: Change review rate.
}

#pragma mark - MWMUGCTextReviewDelegate

- (void)changeReviewText:(NSString *)text
{
  self.review = text;
  // TODO: Write the review into ugc object.
}

@end
