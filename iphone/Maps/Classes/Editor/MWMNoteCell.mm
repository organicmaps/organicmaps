#import "MWMNoteCell.h"

namespace
{
  CGFloat const kMinimalTextViewHeight = 104.;
  CGFloat const kTopTextViewOffset = 12.;
  NSString * const kTextViewContentSizeKeyPath = @"contentSize";
} // namespace

@interface MWMNoteCell () <UITextViewDelegate>

@property (weak, nonatomic) IBOutlet UITextView * textView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * textViewHeight;
@property (weak, nonatomic) id<MWMNoteCelLDelegate> delegate;

@end

static void * kContext = &kContext;

@implementation MWMNoteCell

- (void)configWithDelegate:(id<MWMNoteCelLDelegate>)delegate noteText:(NSString *)text
{
  self.delegate = delegate;
  self.textView.text = text;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context == kContext)
  {
    NSValue * s = change[@"new"];
    CGFloat const height = s.CGSizeValue.height;

    if (height > kMinimalTextViewHeight)
    {
      self.textViewHeight.constant = height;
      [self.delegate cellShouldChangeSize:self text:self.textView.text];
    }
    else
    {
      CGFloat const currentHeight = self.textViewHeight.constant;
      if (currentHeight > kMinimalTextViewHeight)
      {
        self.textViewHeight.constant = kMinimalTextViewHeight;
        [self.delegate cellShouldChangeSize:self text:self.textView.text];
      }
    }

    [self setNeedsLayout];
    [self layoutIfNeeded];

    return;
  }

  [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

- (CGFloat)cellHeight
{
  return self.textViewHeight.constant + 2 * kTopTextViewOffset;
}

- (void)textViewDidEndEditing:(UITextView *)textView
{
  [self.delegate cell:self didFinishEditingWithText:textView.text];
  [self unregisterObserver];
}

- (void)textViewDidBeginEditing:(UITextView *)textView
{
  [self registerObserver];
}

- (void)unregisterObserver
{
  [self.textView removeObserver:self forKeyPath:kTextViewContentSizeKeyPath context:kContext];
}

- (void)registerObserver
{
  [self.textView addObserver:self forKeyPath:kTextViewContentSizeKeyPath options:NSKeyValueObservingOptionNew context:kContext];
}

@end
