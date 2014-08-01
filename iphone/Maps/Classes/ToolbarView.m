
#import "ToolbarView.h"

@interface ToolbarView ()

@property (nonatomic) UIButton * searchButton;
@property (nonatomic) UIButton * bookmarkButton;
@property (nonatomic) UIButton * menuButton;
@property (nonatomic) UIImageView * backgroundImageView;

@end

@implementation ToolbarView

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.backgroundImageView];

  [self.locationButton addTarget:self action:@selector(locationButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  [self addSubview:self.locationButton];

  self.searchButton = [self buttonWithImageName:@"SearchButton"];
  [self.searchButton addTarget:self action:@selector(searchButtonPressed:) forControlEvents:UIControlEventTouchUpInside];

  self.bookmarkButton = [self buttonWithImageName:@"BookmarkButton"];
  [self.bookmarkButton addTarget:self action:@selector(bookmarkButtonPressed:) forControlEvents:UIControlEventTouchUpInside];

  self.menuButton = [self buttonWithImageName:@"MenuButton"];
  [self.menuButton addTarget:self action:@selector(menuButtonPressed:) forControlEvents:UIControlEventTouchUpInside];

  [self layoutButtons];

  return self;
}

- (void)locationButtonPressed:(id)sender
{
  [self.delegate toolbar:self didPressItemWithName:@"Location"];
}

- (void)searchButtonPressed:(id)sender
{
  [self.delegate toolbar:self didPressItemWithName:@"Search"];
}

- (void)bookmarkButtonPressed:(id)sender
{
  [self.delegate toolbar:self didPressItemWithName:@"Bookmarks"];
}

- (void)menuButtonPressed:(id)sender
{
  [self.delegate toolbar:self didPressItemWithName:@"Menu"];
}

- (void)layoutButtons
{
  CGFloat const xOffsetPercent = 1.0 / 8;
  CGFloat const xBetweenPercent = 1.0 / 4;

  self.locationButton.midX = self.width * xOffsetPercent;
  self.searchButton.midX = self.locationButton.midX + self.width * xBetweenPercent;
  self.bookmarkButton.midX = self.searchButton.midX + self.width * xBetweenPercent;
  self.menuButton.midX = self.bookmarkButton.midX + self.width * xBetweenPercent;
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
  UIView * view = [super hitTest:point withEvent:event];
  return view == self ? nil : view;
}

- (UIButton *)buttonWithImageName:(NSString *)imageName
{
  UIButton * button = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 79, 44)];
  button.contentMode = UIViewContentModeCenter;
  button.midY = self.height / 2;
  button.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
  [button setImage:[UIImage imageNamed:imageName] forState:UIControlStateNormal];
  [self addSubview:button];
  return button;
}

- (LocationButton *)locationButton
{
  if (!_locationButton)
  {
    _locationButton = [[LocationButton alloc] initWithFrame:CGRectMake(0, 0, 79, 44)];
    _locationButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    _locationButton.midY = self.height / 2;
    [_locationButton addTarget:self action:@selector(locationButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _locationButton;
}

- (UIImageView *)backgroundImageView
{
  if (!_backgroundImageView)
  {
    _backgroundImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"ToolbarGradient"]];
    _backgroundImageView.frame = self.bounds;
    _backgroundImageView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _backgroundImageView;
}

@end
