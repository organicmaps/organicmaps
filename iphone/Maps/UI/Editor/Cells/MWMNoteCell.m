#import "MWMNoteCell.h"
#import "MWMTextView.h"

static CGFloat const kTopTextViewOffset = 12.;
static NSString * const kTextViewContentSizeKeyPath = @"contentSize";
static CGFloat const kMinimalTextViewHeight = 104.;

@interface MWMNoteCell ()<UITextViewDelegate>

@property(weak, nonatomic) IBOutlet MWMTextView * textView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * textViewHeight;
@property(weak, nonatomic) id<MWMNoteCelLDelegate> delegate;

@end

static void * kContext = &kContext;

@implementation MWMNoteCell

- (void)configWithDelegate:(id<MWMNoteCelLDelegate>)delegate
                  noteText:(NSString *)text
               placeholder:(NSString *)placeholder
{
  self.delegate = delegate;
  self.textView.text = text;
  self.textView.keyboardAppearance =
      [UIColor isNightMode] ? UIKeyboardAppearanceDark : UIKeyboardAppearanceDefault;
  self.textView.placeholder = placeholder;
}

- (void)updateTextViewForHeight:(CGFloat)height
{
  id<MWMNoteCelLDelegate> delegate = self.delegate;
  if (height > kMinimalTextViewHeight)
  {
    self.textViewHeight.constant = height;
    [delegate cellShouldChangeSize:self text:self.textView.text];
  }
  else
  {
    CGFloat currentHeight = self.textViewHeight.constant;
    if (currentHeight > kMinimalTextViewHeight)
    {
      self.textViewHeight.constant = kMinimalTextViewHeight;
      [delegate cellShouldChangeSize:self text:self.textView.text];
    }
  }

  [self setNeedsLayout];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context == kContext)
  {
    NSValue * s = change[@"new"];
    CGFloat height = s.CGSizeValue.height;
    [self updateTextViewForHeight:height];
    return;
  }

  [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

- (CGFloat)cellHeight { return self.textViewHeight.constant + 2 * kTopTextViewOffset; }
- (CGFloat)textViewContentHeight { return self.textView.contentSize.height; }
+ (CGFloat)minimalHeight { return kMinimalTextViewHeight; }
- (void)textViewDidEndEditing:(UITextView *)textView
{
  [self.delegate cell:self didFinishEditingWithText:textView.text];
  [self unregisterObserver];
}

- (void)textViewDidBeginEditing:(UITextView *)textView { [self registerObserver]; }
- (void)unregisterObserver
{
  [self.textView removeObserver:self forKeyPath:kTextViewContentSizeKeyPath context:kContext];
}

- (void)registerObserver
{
  [self.textView addObserver:self
                  forKeyPath:kTextViewContentSizeKeyPath
                     options:NSKeyValueObservingOptionNew
                     context:kContext];
}

@end
