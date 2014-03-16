
#import <UIKit/UIKit.h>
#import "SearchActivityProtocol.h"

@class SearchBar;
@protocol SearchBarDelegate <NSObject>

- (void)searchBarDidPressCancelButton:(SearchBar *)searchBar;
- (void)searchBarDidPressClearButton:(SearchBar *)searchBar;

@end

@interface SearchBar : UIView <SearchActivityProtocol>

@property (nonatomic, readonly) UITextField * textField;
@property (nonatomic) BOOL isShowingResult;
@property (nonatomic, weak) id <SearchBarDelegate> delegate;
@property (nonatomic) NSString * apiText;

- (void)setSearching:(BOOL)searching;

@end
