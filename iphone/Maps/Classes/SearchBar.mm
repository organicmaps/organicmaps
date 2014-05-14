
#import "SearchBar.h"
#import "UIKitCategories.h"
#import "Framework.h"

@interface SearchSpotView : UIView

- (void)setAnimating:(BOOL)animating;

@end

@interface SearchSpotView ()

@property (nonatomic) UIImageView * spotImageView;
@property (nonatomic) UIActivityIndicatorView * activityView;
@property (nonatomic) NSTimer * timer;

@end

@implementation SearchSpotView

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.activityView];
  [self addSubview:self.spotImageView];

  return self;
}

- (void)setAnimating:(BOOL)animating
{
  if (animating)
  {
    self.timer = [NSTimer scheduledTimerWithTimeInterval:0.15 target:self selector:@selector(timerSelector) userInfo:nil repeats:NO];
  }
  else
  {
    [self.activityView stopAnimating];
    self.spotImageView.hidden = NO;
    [self.timer invalidate];
  }
}

- (void)timerSelector
{
  [self.activityView startAnimating];
  self.spotImageView.hidden = YES;
}

- (UIActivityIndicatorView *)activityView
{
  if (!_activityView)
  {
    _activityView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    _activityView.center = CGPointMake(self.width / 2, self.height / 2);
  }
  return _activityView;
}

- (UIImageView *)spotImageView
{
  if (!_spotImageView)
  {
    _spotImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"SearchBarIcon"]];
    _spotImageView.center = CGPointMake(self.width / 2, self.height / 2);
  }
  return _spotImageView;
}

@end

@interface SearchBar ()

@property (nonatomic) UITextField * textField;
@property (nonatomic) SearchSpotView * searchImageView;
@property (nonatomic) UILabel * searchLabel;
@property (nonatomic) UIImageView * backgroundImageView;
@property (nonatomic) UIButton * clearButton;
@property (nonatomic) UIButton * cancelButton;

@end

@implementation SearchBar
@synthesize active = _active;

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.backgroundImageView];

  self.clearButton.midY = self.backgroundImageView.height / 2;
  self.clearButton.maxX = self.backgroundImageView.width;
  [self.backgroundImageView addSubview:self.clearButton];

  [self addSubview:self.searchImageView];
  [self addSubview:self.searchLabel];

  self.textField.midY = self.backgroundImageView.height / 2;
  CGFloat fieldOffsetL = 39;
  CGFloat fieldOffsetR = 24;
  self.textField.frame = CGRectMake(fieldOffsetL, self.textField.origin.y, self.backgroundImageView.width - fieldOffsetL - fieldOffsetR, self.textField.height);
  [self.backgroundImageView addSubview:self.textField];

  self.cancelButton.midY = self.backgroundImageView.height / 2;
  [self addSubview:self.cancelButton];

  [self setActive:NO animated:NO];

  if ([UITextField instancesRespondToSelector:@selector(setTintColor:)])
    [[UITextField appearance] setTintColor:[UIColor darkGrayColor]];

  return self;
}

- (void)layoutSubviews
{
  [self performAfterDelay:0 block:^{
    if (!self.active)
      [self layoutImageAndLabelInNonActiveState];
  }];
}

- (void)setSearching:(BOOL)searching
{
  [self.searchImageView setAnimating:searching];
}

#define LABEL_MIN_X 23

- (void)setActive:(BOOL)active animated:(BOOL)animated
{
  CGFloat backgroundImageOffset = 10;
  if (active)
  {
    [UIView animateWithDuration:(animated ? 0.1 : 0) delay:0.1 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.textField.alpha = 1;
    } completion:nil];

    [UIView animateWithDuration:(animated ? 0.35 : 0) delay:0 damping:0.8 initialVelocity:1 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      if (self.resultText)
      {
        self.searchLabel.minX = 47;
      }
      else
      {
        CGFloat spaceBetween = self.searchLabel.minX - self.searchImageView.minX;
        self.searchImageView.minX = LABEL_MIN_X;
        self.searchLabel.minX = self.searchImageView.minX + spaceBetween;
      }

      self.searchImageView.alpha = 1;
      self.searchLabel.alpha = 0;

      self.backgroundImageView.frame = CGRectMake(backgroundImageOffset, 0, self.width - backgroundImageOffset - self.cancelButton.width, self.backgroundImageView.height);

      self.cancelButton.alpha = 1;
      self.cancelButton.maxX = self.width;

      self.clearButton.alpha = 1;

    } completion:nil];

    [self performAfterDelay:0.1 block:^{
        [self.textField becomeFirstResponder];
    }];
  }
  else
  {
    [UIView animateWithDuration:(animated ? 0.1 : 0) animations:^{
      self.textField.alpha = 0;
    }];

    [UIView animateWithDuration:(animated ? 0.45 : 0) delay:0 damping:0.8 initialVelocity:1 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.backgroundImageView.frame = CGRectMake(backgroundImageOffset, 0, self.width - 2 * backgroundImageOffset, self.backgroundImageView.height);

      self.searchLabel.text = self.resultText ? self.resultText : NSLocalizedString(@"search", nil);
      [self.searchLabel sizeToFit];

      [self layoutImageAndLabelInNonActiveState];

      self.cancelButton.alpha = 0;
      self.cancelButton.minX = self.width;

      if (!self.resultText)
        self.clearButton.alpha = 0;

      [self.textField resignFirstResponder];
    } completion:nil];
  }
  _active = active;
}

- (void)layoutImageAndLabelInNonActiveState
{
  self.searchLabel.alpha = 1;
  if (self.resultText)
  {
    self.searchLabel.midX = self.searchLabel.superview.width / 2;
    self.searchImageView.alpha = 0;
    self.searchImageView.minX = LABEL_MIN_X;
    self.searchLabel.width = MIN(self.searchLabel.width, self.backgroundImageView.width - 54);
  }
  else
  {
    CGFloat const width = self.searchImageView.width + 8 + self.searchLabel.width;
    CGFloat const midX = self.backgroundImageView.width / 2 - 3;
    self.searchImageView.midY = self.backgroundImageView.height / 2;
    self.searchImageView.minX = midX - width / 2 + self.backgroundImageView.minX;
    self.searchImageView.alpha = 1;
    self.searchLabel.midY = self.backgroundImageView.height / 2;
    self.searchLabel.maxX = midX + width / 2 + self.backgroundImageView.minX;
  }
}

- (void)cancelButtonPressed:(id)sender
{
  [self.delegate searchBarDidPressCancelButton:self];
}

- (void)clearButtonPressed:(id)sender
{
  Framework & framework = GetFramework();
  BookmarkManager & manager = framework.GetBookmarkManager();
  manager.UserMarksClear(UserMarkContainer::API_MARK);
  if (self.active)
  {
    self.textField.text = nil;
    self.resultText = nil;
    [self.textField becomeFirstResponder];
  }
  else
  {
    [self hideSearchedText];
  }
  [self.delegate searchBarDidPressClearButton:self];

  manager.UserMarksClear(UserMarkContainer::SEARCH_MARK);
  framework.GetBalloonManager().RemovePin();
  framework.GetBalloonManager().Dismiss();
  framework.Invalidate();
}

- (void)hideSearchedText
{
  self.textField.text = nil;
  self.resultText = nil;
  self.searchLabel.text = NSLocalizedString(@"search", nil);
  [self.searchLabel sizeToFit];
  [UIView animateWithDuration:0.4 delay:0 damping:0.8 initialVelocity:1 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    [self layoutImageAndLabelInNonActiveState];
    self.clearButton.alpha = 0;
  } completion:nil];
}

- (void)setApiText:(NSString *)apiText
{
  if (apiText)
  {
    self.resultText = apiText;
    [self setActive:NO animated:YES];
    self.clearButton.alpha = 1;
  }
  else if (_apiText)
  {
    [self hideSearchedText];
    GetFramework().GetBookmarkManager().UserMarksClear(UserMarkContainer::API_MARK);
  }
  _apiText = apiText;
}

- (UILabel *)searchLabel
{
  if (!_searchLabel)
  {
    _searchLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 100, 25)];
    _searchLabel.textColor = [UIColor blackColor];
    _searchLabel.backgroundColor = [UIColor clearColor];
    _searchLabel.textAlignment = NSTextAlignmentLeft;
    _searchLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:15];
  }
  return _searchLabel;
}

- (SearchSpotView *)searchImageView
{
  if (!_searchImageView)
  {
    _searchImageView = [[SearchSpotView alloc] initWithFrame:CGRectMake(0, 0, 20, 20)];
    _searchImageView.autoresizingMask = UIViewAutoresizingFlexibleRightMargin;
  }
  return _searchImageView;
}

- (UIButton *)cancelButton
{
  if (!_cancelButton)
  {
    _cancelButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 64, 44)];
    _cancelButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    _cancelButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:15];
    [_cancelButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    [_cancelButton setTitleColor:[UIColor grayColor] forState:UIControlStateHighlighted];
    [_cancelButton setTitle:NSLocalizedString(@"cancel", nil) forState:UIControlStateNormal];
    [_cancelButton addTarget:self action:@selector(cancelButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _cancelButton;
}

- (UIButton *)clearButton
{
  if (!_clearButton)
  {
    _clearButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 32, 44)];
    _clearButton.contentMode = UIViewContentModeCenter;
    _clearButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
    [_clearButton setImage:[UIImage imageNamed:@"SearchBarClearButton"] forState:UIControlStateNormal];
    [_clearButton addTarget:self action:@selector(clearButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _clearButton;
}

- (UIImageView *)backgroundImageView
{
  if (!_backgroundImageView)
  {
    UIImage * image = [[UIImage imageNamed:@"SearchBarInactiveBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _backgroundImageView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, image.size.height)];
    _backgroundImageView.image = image;
    _backgroundImageView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _backgroundImageView.userInteractionEnabled = YES;
  }
  return _backgroundImageView;
}

- (UITextField *)textField
{
  if (!_textField)
  {
    _textField = [[UITextField alloc] initWithFrame:CGRectMake(0, 0, self.width, 22)];
    _textField.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _textField.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:15];
    _textField.returnKeyType = UIReturnKeySearch;
    _textField.autocorrectionType = UITextAutocorrectionTypeNo;
  }
  return _textField;
}

@end
