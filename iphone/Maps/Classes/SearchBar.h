
#import <UIKit/UIKit.h>

@class SearchBar;
@protocol SearchBarDelegate <NSObject>

- (void)searchBarDidPressCancelButton:(SearchBar *)searchBar;
- (void)searchBarDidPressClearButton:(SearchBar *)searchBar;

@end

@interface SearchBar : UIView

@property (nonatomic, readonly) UITextField * textField;
@property (nonatomic, readonly) UIButton * clearButton;
@property (nonatomic, readonly) UIButton * cancelButton;
@property (nonatomic, readonly) UIImageView * fieldBackgroundView;

@property (nonatomic, weak) id <SearchBarDelegate> delegate;

- (void)setSearching:(BOOL)searching;

@end
