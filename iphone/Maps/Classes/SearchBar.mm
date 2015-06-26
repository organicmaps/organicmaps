
#import "SearchBar.h"
#import "Framework.h"
#import "UIColor+MapsMeColor.h"

#import "../../../3party/Alohalytics/src/alohalytics_objc.h"

@interface SearchBar ()

@property (nonatomic) UITextField * textField;
@property (nonatomic) UIButton * cancelButton;
@property (nonatomic) UIButton * clearButton;
@property (nonatomic) UIImageView * spotImageView;
@property (nonatomic) UIActivityIndicatorView * activity;
@property (nonatomic) SolidTouchView * fieldBackgroundView;

@end

extern NSString * const kAlohalyticsTapEventKey;

@implementation SearchBar

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.fieldBackgroundView];
  self.fieldBackgroundView.maxY = self.height - 12;
  self.fieldBackgroundView.minX = 8;

  [self.fieldBackgroundView addSubview:self.spotImageView];
  self.spotImageView.center = CGPointMake(14, self.fieldBackgroundView.height / 2 - 1);
  [self.fieldBackgroundView addSubview:self.activity];
  self.activity.center = CGPointMake(self.spotImageView.center.x, self.spotImageView.center.y + INTEGRAL(0.5));

  [self.fieldBackgroundView addSubview:self.clearButton];
  self.clearButton.midY = self.fieldBackgroundView.height / 2;
  self.clearButton.midX = self.fieldBackgroundView.width - 14;

  self.cancelButton.midY = self.height / 2 - 5;
  self.cancelButton.maxX = self.width - 8;
  [self addSubview:self.cancelButton];

  [self addSubview:self.textField];
  self.textField.midY = self.height / 2 - 3;
  self.textField.minX = 36.;

  self.textField.tintColor = [UIColor blackHintText];

  return self;
}

- (void)setSearching:(BOOL)searching
{
  if (searching)
  {
    [self.activity startAnimating];
    self.spotImageView.alpha = 0;
  }
  else
  {
    [self.activity stopAnimating];
    self.spotImageView.alpha = 1;
  }
}

- (void)cancelButtonPressed:(id)sender
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"searchCancel"];
  [self.delegate searchBarDidPressCancelButton:self];
}

- (void)clearButtonPressed
{
  [self.delegate searchBarDidPressClearButton:self];
}

- (UIButton *)cancelButton
{
  if (!_cancelButton)
  {
    _cancelButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 52, 44)];
    _cancelButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:15];
    _cancelButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    [_cancelButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [_cancelButton setTitleColor:[UIColor colorWithWhite:1 alpha:0.5] forState:UIControlStateHighlighted];
    NSString * title = L(@"cancel");
    CGFloat const titleWidth = [title sizeWithDrawSize:CGSizeMake(80, 25) font:_cancelButton.titleLabel.font].width + 2;
    _cancelButton.width = MAX(titleWidth, _cancelButton.width);
    [_cancelButton setTitle:title forState:UIControlStateNormal];
    [_cancelButton addTarget:self action:@selector(cancelButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _cancelButton;
}

- (UIButton *)clearButton
{
 if (!_clearButton)
  {
    _clearButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 44, 44)];
    _clearButton.contentMode = UIViewContentModeCenter;
    _clearButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
    [_clearButton setImage:[UIImage imageNamed:@"SearchBarClearButton"] forState:UIControlStateNormal];
    [_clearButton addTarget:self action:@selector(clearButtonPressed) forControlEvents:UIControlEventTouchUpInside];
  }
  return _clearButton;
}

- (SolidTouchView *)fieldBackgroundView
{
  if (!_fieldBackgroundView)
  {
    _fieldBackgroundView = [[SolidTouchView alloc] initWithFrame:CGRectMake(0, 0, 0, 27)];
    _fieldBackgroundView.backgroundColor = [UIColor whiteColor];
    _fieldBackgroundView.userInteractionEnabled = YES;
    _fieldBackgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _fieldBackgroundView.layer.cornerRadius = 4.;
  }
  return _fieldBackgroundView;
}

- (UITextField *)textField
{
  if (!_textField)
  {
    _textField = [[UITextField alloc] initWithFrame:CGRectMake(1, 0, self.width, 22)];
    _textField.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _textField.font = [UIFont fontWithName:@"HelveticaNeue" size:16.];
    _textField.textColor = [UIColor blackPrimaryText];
    _textField.placeholder = L(@"search");
    _textField.returnKeyType = UIReturnKeySearch;
    _textField.autocorrectionType = UITextAutocorrectionTypeNo;
    _textField.enablesReturnKeyAutomatically = YES;
  }
  return _textField;
}

- (UIImageView *)spotImageView
{
  if (!_spotImageView)
  {
    _spotImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"ic_search"]];
    _spotImageView.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _spotImageView;
}

- (UIActivityIndicatorView *)activity
{
  if (!_activity)
  {
    _activity = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    _activity.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _activity;
}

@end
