
#import "SearchBar.h"
#import "UIKitCategories.h"
#import "Framework.h"

@interface SearchBar ()

@property (nonatomic) UITextField * textField;
@property (nonatomic) UIButton * clearButton;
@property (nonatomic) UIButton * cancelButton;
@property (nonatomic) UIImageView * fieldBackgroundView;

@end

@implementation SearchBar

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.fieldBackgroundView];
  self.fieldBackgroundView.maxY = self.height - 12;

  [self.fieldBackgroundView addSubview:self.clearButton];
  self.clearButton.midY = self.fieldBackgroundView.height / 2;
  self.clearButton.midX = self.fieldBackgroundView.width - 14;

  self.cancelButton.midY = self.height / 2 - 5;
  self.cancelButton.maxX = self.width - 3;
  [self addSubview:self.cancelButton];

  self.textField.midY = self.height / 2 - 3;
  [self addSubview:self.textField];

  if ([self.textField respondsToSelector:@selector(setTintColor:)])
    self.textField.tintColor = [UIColor whiteColor];

  return self;
}

- (void)setSearching:(BOOL)searching
{

}

- (void)cancelButtonPressed:(id)sender
{
  [self.delegate searchBarDidPressCancelButton:self];
}

- (void)clearButtonPressed:(id)sender
{
  [self.delegate searchBarDidPressClearButton:self];
}

- (void)hideSearchedText
{
  [UIView animateWithDuration:0.4 delay:0 damping:0.8 initialVelocity:1 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    self.clearButton.alpha = 0;
  } completion:nil];
}

- (UIButton *)cancelButton
{
  if (!_cancelButton)
  {
    _cancelButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 64, 44)];
    _cancelButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:15];
    _cancelButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    [_cancelButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [_cancelButton setTitleColor:[UIColor lightGrayColor] forState:UIControlStateHighlighted];
    [_cancelButton setTitle:@"cancel" forState:UIControlStateNormal];
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
    [_clearButton addTarget:self action:@selector(clearButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _clearButton;
}

- (UIImageView *)fieldBackgroundView
{
  if (!_fieldBackgroundView)
  {
    UIImage * image = [[UIImage imageNamed:@"SearchFieldBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _fieldBackgroundView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 0, image.size.height)];
    _fieldBackgroundView.image = image;
    _fieldBackgroundView.userInteractionEnabled = YES;
    _fieldBackgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _fieldBackgroundView;
}

- (UITextField *)textField
{
  if (!_textField)
  {
    _textField = [[UITextField alloc] initWithFrame:CGRectMake(0, 0, self.width, 22)];
    _textField.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _textField.font = [UIFont fontWithName:@"HelveticaNeue" size:17.5];
    _textField.textColor = [UIColor whiteColor];
    _textField.returnKeyType = UIReturnKeySearch;
    _textField.autocorrectionType = UITextAutocorrectionTypeNo;
    _textField.enablesReturnKeyAutomatically = YES;
  }
  return _textField;
}

@end
