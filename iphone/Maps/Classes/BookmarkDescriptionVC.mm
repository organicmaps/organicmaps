
#import "BookmarkDescriptionVC.h"
#import "Framework.h"

@interface BookmarkDescriptionVC ()

@property (weak, nonatomic) IBOutlet UITextView * textView;

@end

@implementation BookmarkDescriptionVC

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.title = NSLocalizedString(@"description", nil);

  BookmarkCategory const * category = GetFramework().GetBmCategory(self.bookmarkAndCategory.first);
  Bookmark const * bookmark = category->GetBookmark(self.bookmarkAndCategory.second);

  self.textView.text = [NSString stringWithUTF8String:bookmark->GetDescription().c_str()];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  [self.textView becomeFirstResponder];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];

  BookmarkCategory * category = GetFramework().GetBmCategory(self.bookmarkAndCategory.first);
  Bookmark * bookmark = category->GetBookmark(self.bookmarkAndCategory.second);
  bookmark->SetDescription([self.textView.text UTF8String]);
  category->SaveToKMLFile();

  [self.delegate bookmarkDescriptionVC:self didUpdateBookmarkAndCategory:self.bookmarkAndCategory];
}

@end
