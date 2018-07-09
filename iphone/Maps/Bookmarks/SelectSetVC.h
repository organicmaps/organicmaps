#import "MWMTableViewController.h"

#include "kml/type_utils.hpp"

@protocol MWMSelectSetDelegate <NSObject>

- (void)didSelectCategory:(NSString *)category withCategoryId:(kml::MarkGroupId)categoryId;

@end

@interface SelectSetVC : MWMTableViewController

- (instancetype)initWithCategory:(NSString *)category
                      categoryId:(kml::MarkGroupId)categoryId
                        delegate:(id<MWMSelectSetDelegate>)delegate;

@end
