#import "MWMTableViewController.h"

#include "drape_frontend/user_marks_global.hpp"

@protocol MWMSelectSetDelegate <NSObject>

- (void)didSelectCategory:(NSString *)category withCategoryId:(df::MarkGroupID)categoryId;

@end

@interface SelectSetVC : MWMTableViewController

- (instancetype)initWithCategory:(NSString *)category
                      categoryId:(df::MarkGroupID)categoryId
                        delegate:(id<MWMSelectSetDelegate>)delegate;

@end
