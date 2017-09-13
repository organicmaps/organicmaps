#import "MWMTextView.h"
#import "MWMCommon.h"

@interface MWMTextView ()

@property (nonatomic, readwrite) UILabel * placeholderView;

@end

@implementation MWMTextView

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self setTextContainerInset:UIEdgeInsetsZero];

  [self updatePlaceholderVisibility];

  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(textDidChange:)
                                             name:UITextViewTextDidChangeNotification
                                           object:self];
  self.clipsToBounds = YES;
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self
                                                name:UITextViewTextDidChangeNotification
                                              object:nil];
}

- (UILabel *)placeholderView
{
  if (!_placeholderView)
  {
    _placeholderView = [[UILabel alloc] initWithFrame:self.bounds];
    _placeholderView.opaque = NO;
    _placeholderView.backgroundColor = UIColor.clearColor;
    _placeholderView.textColor = [UIColor lightGrayColor];
    _placeholderView.textAlignment = self.textAlignment;
    _placeholderView.userInteractionEnabled = NO;
    _placeholderView.font = self.font;
    _placeholderView.isAccessibilityElement = NO;
    _placeholderView.numberOfLines = 0;
  }
  return _placeholderView;
}

#pragma mark - Setters

- (void)setPlaceholder:(NSString *)placeholder
{
  _placeholder = placeholder.copy;
  self.placeholderView.text = placeholder;
  [self resizePlaceholderFrame];
}

- (void)setFont:(UIFont *)font
{
  [super setFont:font];
  self.placeholderView.font = font;
}

- (void)setAttributedText:(NSAttributedString *)attributedText
{
  [super setAttributedText:attributedText];
  [self updatePlaceholderVisibility];
}

- (void)setText:(NSString *)text
{
  [super setText:text];
  [self updatePlaceholderVisibility];
}

- (void)setTextAlignment:(NSTextAlignment)textAlignment
{
  [super setTextAlignment:textAlignment];
  self.placeholderView.textAlignment = textAlignment;
}

- (void)setTextContainerInset:(UIEdgeInsets)textContainerInset
{
  [super setTextContainerInset:textContainerInset];
  [self updatePlaceholderInset:textContainerInset];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  [self resizePlaceholderFrame];
}

- (void)resizePlaceholderFrame
{
  [self.placeholderView sizeToFit];
}

- (void)textDidChange:(NSNotification *)aNotification
{
  [self updatePlaceholderVisibility];
}

- (void)updatePlaceholderInset:(UIEdgeInsets)inset
{
  CGFloat const kDefaultPlaceholderLeftInset = 5.0;
  self.placeholderView.frame = CGRectMake(inset.left + kDefaultPlaceholderLeftInset, inset.top, self.bounds.size.width - inset.right, self.bounds.size.height - inset.bottom);
  [self resizePlaceholderFrame];
}

- (void)updatePlaceholderVisibility
{
  if (!self.text.length)
  {
    [self addSubview:self.placeholderView];
    [self sendSubviewToBack:self.placeholderView];
  }
  else
  {
    [self.placeholderView removeFromSuperview];
  }
}

@end
