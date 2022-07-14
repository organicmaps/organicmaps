#import "MWMSearchManagerState.h"

@protocol MWMSearchTabbedViewProtocol <NSObject>

@required

@property(nonatomic) MWMSearchManagerState state;

- (void)searchText:(NSString *)text forInputLocale:(NSString *)locale withCategory:(BOOL)isCategory;
- (void)dismissKeyboard;

@end
