
#import "BookmarkDescriptionVC.h"
#import "Framework.h"
#import "UIKitCategories.h"

@interface BookmarkDescriptionVC ()

@property (weak, nonatomic) IBOutlet UITextView * textView;

@end

@implementation BookmarkDescriptionVC

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"description");

  BookmarkCategory const * category = GetFramework().GetBmCategory(self.bookmarkAndCategory.first);
  Bookmark const * bookmark = category->GetBookmark(self.bookmarkAndCategory.second);

  self.textView.text = [NSString stringWithUTF8String:bookmark->GetDescription().c_str()];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  [self.textView becomeFirstResponder];
}

- (void)viewDidDisappear:(BOOL)animated
{
  [super viewDidDisappear:animated];

  BookmarkCategory * category = GetFramework().GetBmCategory(self.bookmarkAndCategory.first);
  Bookmark * bookmark = category->GetBookmark(self.bookmarkAndCategory.second);
  bookmark->SetDescription([self.textView.text UTF8String]);
  category->SaveToKMLFile();

  [self.delegate bookmarkDescriptionVC:self didUpdateBookmarkAndCategory:self.bookmarkAndCategory];
}

@end
