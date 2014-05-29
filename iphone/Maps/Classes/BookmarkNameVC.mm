
#import "BookmarkNameVC.h"
#import "Framework.h"
#import "UIKitCategories.h"

@interface BookmarkNameVC ()

@property (weak, nonatomic) IBOutlet UITextField * textField;

@end

@implementation BookmarkNameVC

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.title = NSLocalizedString(@"name", nil);

  BookmarkCategory const * category = GetFramework().GetBmCategory(self.bookmarkAndCategory.first);
  Bookmark const * bookmark = category->GetBookmark(self.bookmarkAndCategory.second);

  self.textField.text = bookmark->GetName().empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:bookmark->GetName().c_str()];
  [self.textField addTarget:self action:@selector(textDidChange:) forControlEvents:UIControlEventEditingChanged];
}

- (void)textDidChange:(UITextField *)textField
{
  textField.text = [textField.text capitalizedString];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  [self.textField becomeFirstResponder];
}

- (void)viewDidDisappear:(BOOL)animated
{
  [super viewDidDisappear:animated];

  BookmarkCategory * category = GetFramework().GetBmCategory(self.bookmarkAndCategory.first);
  Bookmark * bookmark = category->GetBookmark(self.bookmarkAndCategory.second);
  bookmark->SetName(([self.textField.text UTF8String]));
  category->SaveToKMLFile();

  [self.delegate bookmarkNameVC:self didUpdateBookmarkAndCategory:self.bookmarkAndCategory];
}

@end
