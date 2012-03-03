#import <UIKit/UIKit.h>

@protocol SearchSuggestionDelegate

- (void)onSuggestionSelected:(NSString *)suggestion;

@end

@interface SearchSuggestionsCell : UITableViewCell
{
  NSMutableArray * images;
  // Contains corresponding suggestions for each icon in icons array
  NSMutableArray * suggestions;
}

@property(nonatomic, assign) id <SearchSuggestionDelegate> delegate;

- (id)initWithReuseIdentifier:(NSString *)identifier;
- (void)addIcon:(UIImage *)icon withSuggestion:(NSString *)suggestion;

@end
