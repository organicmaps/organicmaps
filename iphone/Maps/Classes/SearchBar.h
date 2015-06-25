
#import <UIKit/UIKit.h>
#import "UIKitCategories.h"

@class SearchBar;
@protocol SearchBarDelegate <NSObject>

- (void)searchBarDidPressCancelButton:(SearchBar *)searchBar;
- (void)searchBarDidPressClearButton:(SearchBar *)searchBar;

@end

@interface SearchBar : UIView

@property (nonatomic, readonly) UITextField * textField;
@property (nonatomic, readonly) UIButton * cancelButton;
@property (nonatomic, readonly) UIButton * clearButton;
@property (nonatomic, readonly) SolidTouchView * fieldBackgroundView;

@property (nonatomic, weak) id <SearchBarDelegate> delegate;

- (void)setSearching:(BOOL)searching;

@end
