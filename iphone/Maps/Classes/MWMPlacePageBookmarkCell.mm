#import "MWMBasePlacePageView.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageBookmarkCell.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "MWMTextView.h"
#import "Statistics.h"

extern CGFloat const kBookmarkCellHeight = 136.0;

static CGFloat const kSeparatorAndTitleHeight = 52.0;

static NSUInteger sWebViewHeight = 0;

@interface MWMPlacePageBookmarkCell () <UITextFieldDelegate, UIWebViewDelegate>

@property (weak, nonatomic) IBOutlet UITextField * title;
@property (weak, nonatomic) IBOutlet UIButton * categoryButton;
@property (weak, nonatomic) IBOutlet UIButton * markButton;

@property (weak, nonatomic) IBOutlet UIView * note;
@property (weak, nonatomic) IBOutlet UILabel * noteLabel;
@property (weak, nonatomic) IBOutlet UIWebView * noteWebView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * noteViewHeight;
@property (weak, nonatomic) IBOutlet UIButton * editBookmarkButton;

@property (weak, nonatomic) MWMPlacePage * placePage;

@property (nonatomic) BOOL forHeight;

@property (nonatomic) NSString * webViewContent;

@end

@implementation MWMPlacePageBookmarkCell

- (void)config:(MWMPlacePage *)placePage forHeight:(BOOL)forHeight
{
  self.placePage = placePage;
  self.forHeight = forHeight;

  [self configNote];

  if (forHeight && self.entity.isHTMLDescription)
    return;

  self.title.text = self.entity.bookmarkTitle;
  [self.categoryButton setTitle:[NSString stringWithFormat:@"%@ >", self.entity.bookmarkCategory]
                       forState:UIControlStateNormal];
  [self.markButton
   setImage:[UIImage imageNamed:[NSString stringWithFormat:@"%@-on", self.entity.bookmarkColor]]
   forState:UIControlStateNormal];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillShown:)
                                               name:UIKeyboardWillShowNotification
                                             object:nil];

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillBeHidden)
                                               name:UIKeyboardWillHideNotification
                                             object:nil];
}

- (void)configNote
{
  if (self.entity.bookmarkDescription.length == 0)
  {
    self.note.hidden = YES;
    [self.editBookmarkButton setTitle:L(@"description") forState:UIControlStateNormal];
  }
  else
  {
    self.note.hidden = NO;
    [self.editBookmarkButton setTitle:L(@"edit") forState:UIControlStateNormal];
    if (self.entity.isHTMLDescription)
    {
      self.noteWebView.hidden = NO;
      self.noteLabel.hidden = YES;
      if (!self.forHeight && ![self.webViewContent isEqualToString:self.entity.bookmarkDescription])
      {
        sWebViewHeight = 0.0;
        self.webViewContent = self.entity.bookmarkDescription;
        [self.noteWebView loadHTMLString:self.entity.bookmarkDescription baseURL:nil];
        self.noteWebView.scrollView.scrollEnabled = NO;
      }
    }
    else
    {
      self.noteWebView.hidden = YES;
      self.noteLabel.hidden = NO;
      self.noteLabel.text = self.entity.bookmarkDescription;
      [self.noteLabel sizeToFit];
      self.noteViewHeight.constant = kSeparatorAndTitleHeight + self.noteLabel.height;
    }
  }
}

- (void)keyboardWillShown:(NSNotification *)aNotification
{
  if ([self.title isEditing])
    [self.placePage willStartEditingBookmarkTitle];
}

- (void)keyboardWillBeHidden
{
  if ([self.title isEditing])
    [self.placePage willFinishEditingBookmarkTitle:self.title.text.length > 0 ? self.title.text : self.entity.title];
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  self.entity.bookmarkTitle = textField.text.length > 0 ? textField.text : self.entity.title;
  [self.entity synchronize];
  [textField resignFirstResponder];
}

- (BOOL)textFieldShouldClear:(UITextField *)textField
{
  return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  return YES;
}

- (void)dealloc
{
  if (!self.forHeight)
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (IBAction)colorPickerButtonTap
{
  [self.placePage changeBookmarkColor];
  [self endEditing:YES];
}

- (IBAction)categoryButtonTap
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatChangeBookmarkGroup)];
  [self.placePage changeBookmarkCategory];
  [self endEditing:YES];
}

- (IBAction)editTap
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatChangeBookmarkDescription)];
  [self.placePage changeBookmarkDescription];
  [self endEditing:YES];
}

- (MWMPlacePageEntity *)entity
{
  return self.placePage.manager.entity;
}

- (CGFloat)cellHeight
{
  CGFloat const noteViewHeight = self.entity.isHTMLDescription ? sWebViewHeight : self.noteViewHeight.constant;
  return kBookmarkCellHeight + ceil(self.note.hidden ? 0.0 : noteViewHeight);
}

#pragma mark - UIWebViewDelegate

- (void)webViewDidFinishLoad:(UIWebView * _Nonnull)webView
{
  webView.height = webView.scrollView.contentSize.height;
  NSUInteger webViewHeight = ceil(kSeparatorAndTitleHeight + webView.height);
  self.noteViewHeight.constant = webViewHeight;
  if (sWebViewHeight != webViewHeight)
  {
    sWebViewHeight = webViewHeight;
    [self.placePage reloadBookmark];
  }
}

- (BOOL)webView:(UIWebView *)inWeb
    shouldStartLoadWithRequest:(NSURLRequest *)inRequest
                navigationType:(UIWebViewNavigationType)inType
{
  if (inType == UIWebViewNavigationTypeLinkClicked)
  {
    [[UIApplication sharedApplication] openURL:[inRequest URL]];
    return NO;
  }
  return YES;
}

@end
