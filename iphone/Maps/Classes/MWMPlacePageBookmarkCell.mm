#import "MapsAppDelegate.h"
#import "MWMPlacePageBookmarkCell.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

namespace
{
CGFloat const kBoundedTextViewHeight = 240.;
CGFloat const kTextViewTopOffset = 12.;
CGFloat const kMoreButtonHeight = 33.;
CGFloat const kTextViewLeft = 16.;

}  // namespace

@interface MWMPlacePageBookmarkCell () <UITextFieldDelegate, UITextViewDelegate>

@property (weak, nonatomic) IBOutlet UITextView * textView;
@property (weak, nonatomic) IBOutlet UIButton * moreButton;
@property (weak, nonatomic) IBOutlet UIButton * editButton;
@property (weak, nonatomic) IBOutlet UIImageView * separator;
@property (weak, nonatomic) IBOutlet UIImageView * gradient;
@property (weak, nonatomic) IBOutlet UIImageView * spinner;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * textViewTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * textViewBottomOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * textViewHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * moreButtonHeight;

@property (weak, nonatomic) id<MWMPlacePageBookmarkDelegate> delegate;

@property (copy, nonatomic) NSAttributedString * attributedHtml;
@property (copy, nonatomic) NSString * cachedHtml;

@end

@implementation MWMPlacePageBookmarkCell

- (void)configWithText:(NSString *)text
              delegate:(id<MWMPlacePageBookmarkDelegate>)delegate
        placePageWidth:(CGFloat)width
                isOpen:(BOOL)isOpen
                isHtml:(BOOL)isHtml
{
  self.delegate = delegate;
  self.textView.width = width - 2 * kTextViewLeft;
  BOOL const isEmpty = text.length == 0;
  if (isEmpty)
    [self configEmptyDescription];
  else if (isHtml)
    [self configHtmlDescription:text isOpen:isOpen];
  else
    [self configPlaintTextDescription:text isOpen:isOpen];
}

- (void)configEmptyDescription
{
  self.textView.hidden = self.separator.hidden = self.gradient.hidden = self.moreButton.hidden = self.spinner.hidden = YES;
  self.textViewTopOffset.constant = self.textViewBottomOffset.constant = self.textViewHeight.constant =
                                                                         self.moreButtonHeight.constant = 0;
}

- (void)startSpinner
{
  self.editButton.hidden = YES;
  NSUInteger const animationImagesCount = 12;
  NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  NSString * postfix = [UIColor isNightMode] ? @"dark" : @"light";
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"Spinner_%@_%@", @(i+1), postfix]];

  self.spinner.animationDuration = 0.8;
  self.spinner.animationImages = animationImages;
  self.spinner.hidden = NO;
  [self.spinner startAnimating];
}

- (void)stopSpinner
{
  [self.spinner stopAnimating];
  self.editButton.hidden = NO;
  self.spinner.hidden = YES;
}

- (void)configPlaintTextDescription:(NSString *)text isOpen:(BOOL)isOpen
{
  self.spinner.hidden = YES;
  self.textView.scrollEnabled = YES;
  self.textViewTopOffset.constant = kTextViewTopOffset;
  self.textView.hidden = self.separator.hidden = NO;
  self.textView.text = text;
  CGFloat const textViewHeight = self.textView.contentSize.height;
  if (textViewHeight > kBoundedTextViewHeight && !isOpen)
  {
    self.textViewHeight.constant = kBoundedTextViewHeight;
    self.moreButton.hidden = self.gradient.hidden = NO;
    self.moreButtonHeight.constant = kMoreButtonHeight;
    self.textViewBottomOffset.constant = 0;
  }
  else
  {
    self.textViewHeight.constant = textViewHeight;
    self.moreButton.hidden = self.gradient.hidden = YES;
    self.moreButtonHeight.constant = 0;
    self.textViewBottomOffset.constant = kTextViewTopOffset;
  }
  self.textView.scrollEnabled = NO;
}

- (void)configHtmlDescription:(NSString *)text isOpen:(BOOL)isOpen
{
  // html already was rendered and text is same as text which was cached into html
  if (self.attributedHtml && [self.cachedHtml isEqualToString:text])
  {
    self.textView.scrollEnabled = YES;
    self.textViewTopOffset.constant = kTextViewTopOffset;
    self.textView.hidden = self.separator.hidden = NO;
    self.textView.attributedText = self.attributedHtml;
    CGFloat const textViewHeight = self.textView.contentSize.height;
    if (textViewHeight > kBoundedTextViewHeight && !isOpen)
    {
      self.textViewHeight.constant = kBoundedTextViewHeight;
      self.moreButton.hidden = self.gradient.hidden = NO;
      self.moreButtonHeight.constant = kMoreButtonHeight;
      self.textViewBottomOffset.constant = 0;
    }
    else
    {
      self.textViewHeight.constant = textViewHeight;
      self.moreButton.hidden = self.gradient.hidden = YES;
      self.moreButtonHeight.constant = 0;
      self.textViewBottomOffset.constant = kTextViewTopOffset;
    }
    self.textView.scrollEnabled = NO;
  }
  else
  {
    [self configEmptyDescription];
    [self startSpinner];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
    {
      self.cachedHtml = text;
      NSDictionary<NSString *, id> * attr = @{NSForegroundColorAttributeName : [UIColor blackPrimaryText],
                                              NSFontAttributeName : [UIFont regular12]};
      NSMutableAttributedString * str = [[NSMutableAttributedString alloc]
                                              initWithData:[text dataUsingEncoding:NSUnicodeStringEncoding]
                                              options:@{NSDocumentTypeDocumentAttribute : NSHTMLTextDocumentType}
                                   documentAttributes:nil
                                                error:nil];
      [str addAttributes:attr range:{0, str.length}];
      self.attributedHtml = str;
      dispatch_async(dispatch_get_main_queue(), ^
      {
        [self stopSpinner];
        [self.delegate reloadBookmark];
      });
    });
  }
}

- (IBAction)moreTap
{
  [self.delegate moreTap];
}

- (IBAction)editTap
{
  [self.delegate editBookmarkTap];
}

- (CGFloat)cellHeight
{
  return self.textViewTopOffset.constant + self.textViewHeight.constant +
         self.textViewBottomOffset.constant + self.moreButtonHeight.constant +
         self.separator.height + self.editButton.height;
}

#pragma mark - UITextViewDelegate

- (BOOL)textView:(UITextView *)textView shouldInteractWithURL:(NSURL *)URL inRange:(NSRange)characterRange
{
  UIViewController * vc = static_cast<UIViewController *>(MapsAppDelegate.theApp.mapViewController);
  [vc openUrl:URL];
  return NO;
}

@end
