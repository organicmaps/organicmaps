#import "Common.h"
#import "MWMBookmarkDescriptionViewController.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageEntity.h"
#import "UIViewController+Navigation.h"

static NSString * const kBookmarkDescriptionViewControllerNibName = @"MWMBookmarkDescriptionViewController";

typedef NS_ENUM(NSUInteger, BookmarkDescriptionState)
{
  BookmarkDescriptionStateEditText,
  BookmarkDescriptionStateViewHTML,
  BookmarkDescriptionStateEditHTML
};

@interface MWMBookmarkDescriptionViewController () <UIWebViewDelegate>

@property (weak, nonatomic) IBOutlet UITextView * textView;
@property (weak, nonatomic) IBOutlet UIWebView * webView;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * textViewBottomOffset;

@property (nonatomic) UIBarButtonItem * leftButton;
@property (nonatomic) UIBarButtonItem * rightButton;

@property (weak, nonatomic) MWMPlacePageViewManager * manager;
@property (nonatomic) BookmarkDescriptionState state;

@end

@implementation MWMBookmarkDescriptionViewController

- (instancetype)initWithPlacePageManager:(MWMPlacePageViewManager *)manager
{
  self = [super initWithNibName:kBookmarkDescriptionViewControllerNibName bundle:nil];
  if (self)
    self.manager = manager;

  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.navigationItem.title = L(@"description");
  MWMPlacePageEntity const * entity = self.manager.entity;
  if (!IPAD)
  {
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillChangeFrame:)
                                                 name:UIKeyboardWillChangeFrameNotification
                                               object:nil];
  }
  if (entity.isHTMLDescription)
    self.state = BookmarkDescriptionStateViewHTML;
  else
    self.state = BookmarkDescriptionStateEditText;

  if (self.iPadOwnerNavigationController)
  {
    UIBarButtonItem * leftButton = [[UIBarButtonItem alloc] initWithCustomView:self.backButton];
    [self.navigationItem setLeftBarButtonItem:leftButton];
  }
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self.iPadOwnerNavigationController setNavigationBarHidden:NO];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  [self.manager reloadBookmark];
}

- (void)viewDidLayoutSubviews
{
  [super viewDidLayoutSubviews];
  if (!IPAD)
    return;
  self.view.height = self.iPadOwnerNavigationController.view.height - self.iPadOwnerNavigationController.navigationBar.height;
}

- (void)setState:(BookmarkDescriptionState)state
{
  MWMPlacePageEntity * entity = self.manager.entity;
  NSString * description = entity.bookmarkDescription;
  switch (state)
  {
    case BookmarkDescriptionStateEditText:
    case BookmarkDescriptionStateEditHTML:
      [self setupForEditingWithText:description];
      break;

    case BookmarkDescriptionStateViewHTML:
      [self setupForViewWithText:description];
      break;
  }
  _state = state;
}

- (void)setupForEditingWithText:(NSString *)text
{
  self.textView.hidden = NO;
  self.textView.text = text;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.webView.alpha = 0.;
    self.textView.alpha = 1.;
  }
  completion:^(BOOL finished)
  {
    self.webView.hidden = YES;
    [self.textView becomeFirstResponder];
  }];
  [self configureNavigationBarForEditing];
}

- (void)setupForViewWithText:(NSString *)text
{
  self.webView.hidden = NO;
  [self.webView loadHTMLString:text baseURL:nil];
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.webView.alpha = 1.;
    self.textView.alpha = 0.;
  }
  completion:^(BOOL finished)
  {
    self.textView.hidden = YES;
  }];
  [self configureNavigationBarForView];
}

- (void)configureNavigationBarForEditing
{
  self.leftButton = [[UIBarButtonItem alloc] initWithTitle:L(@"cancel") style:UIBarButtonItemStylePlain target:self action:@selector(cancelTap)];
  self.rightButton = [[UIBarButtonItem alloc] initWithTitle:L(@"done") style:UIBarButtonItemStylePlain target:self action:@selector(doneTap)];
  [self setupButtons];
}

- (void)configureNavigationBarForView
{
  self.leftButton = [[UIBarButtonItem alloc] initWithCustomView:self.backButton];
  self.rightButton = [[UIBarButtonItem alloc] initWithTitle:L(@"edit") style:UIBarButtonItemStylePlain target:self action:@selector(editTap)];
  [self setupButtons];
}

- (void)setupButtons
{
  [self.navigationItem setLeftBarButtonItem:self.leftButton];
  [self.navigationItem setRightBarButtonItem:self.rightButton];
}

- (void)cancelTap
{
  [self.textView resignFirstResponder];
  if (self.manager.entity.isHTMLDescription)
    self.state = BookmarkDescriptionStateViewHTML;
  else
    [self popViewController];
}

- (void)doneTap
{
  MWMPlacePageEntity * entity = self.manager.entity;
  entity.bookmarkDescription = self.textView.text;
  [entity synchronize];
  [self cancelTap];
}

- (void)backTap
{
  [self.textView resignFirstResponder];
  [self popViewController];
}

- (void)editTap
{
  self.state = BookmarkDescriptionStateEditHTML;
}

- (void)popViewController
{
  [self.iPadOwnerNavigationController setNavigationBarHidden:YES];
  [self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - Notifications

- (void)keyboardWillChangeFrame:(NSNotification *)aNotification
{
  NSDictionary * info = [aNotification userInfo];
  CGSize const kbSize = [info[UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
  CGFloat const offsetToKeyboard = 8.0;
  CGFloat const navBarHeight = IPAD ? self.navigationController.navigationBar.height : 0.0;
  self.textViewBottomOffset.constant = kbSize.height + offsetToKeyboard - navBarHeight;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - UIWebViewDelegate

- (BOOL)webView:(UIWebView *)inWeb shouldStartLoadWithRequest:(NSURLRequest *)inRequest navigationType:(UIWebViewNavigationType)inType
{
  if (inType == UIWebViewNavigationTypeLinkClicked)
  {
    [[UIApplication sharedApplication] openURL:[inRequest URL]];
    return NO;
  }
  return YES;
}

#pragma mark - Buttons

- (UIButton *)backButton
{
  UIImage * backImage = [UIImage imageNamed:@"NavigationBarBackButton"];
  CGFloat const imageSide = backImage.size.width;
  UIButton * button = [[UIButton alloc] initWithFrame:CGRectMake(0., 0., imageSide, imageSide)];
  [button setImage:backImage forState:UIControlStateNormal];
  [button addTarget:self action:@selector(backTap) forControlEvents:UIControlEventTouchUpInside];
  button.imageEdgeInsets = UIEdgeInsetsMake(0., -imageSide, 0., 0.);
  return button;
}

@end
