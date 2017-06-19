namespace ugc
{
struct RatingRecord;
struct UGCUpdate;
}

struct FeatureID;

namespace ugc_review
{
enum class Row
{
  Detail,
  SpecialQuestion,
  Message
};
}  // namespace ugc

@interface MWMUGCReviewVM : NSObject

+ (instancetype)fromUGC:(ugc::UGCUpdate const &)ugc
              featureId:(FeatureID const &)fid
                   name:(NSString *)name;

- (NSInteger)numberOfRows;
- (ugc_review::Row)rowForIndexPath:(NSIndexPath *)indexPath;
- (ugc::RatingRecord const &)recordForIndexPath:(NSIndexPath *)indexPath;
- (NSString *)review;
- (NSString *)name;

- (void)setDefaultStarCount:(NSInteger)starCount;
- (void)submit;

@end
