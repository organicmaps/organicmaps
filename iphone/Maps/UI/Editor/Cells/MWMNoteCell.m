#import "MWMNoteCell.h"
#import "MWMTextView.h"

static CGFloat const kTopTextViewOffset = 12.;
static NSString *const kTextViewContentSizeKeyPath = @"contentSize";
static CGFloat const kMinimalTextViewHeight = 104.;
static void *kContext = &kContext;

@interface MWMNoteCell () <UITextViewDelegate>

@property(nonatomic) IBOutlet MWMTextView *textView;
@property(nonatomic) IBOutlet NSLayoutConstraint *textViewHeight;
@property(weak, nonatomic) id<MWMNoteCellDelegate> delegate;

@end

@implementation MWMNoteCell

- (void)configWithDelegate:(id<MWMNoteCellDelegate>)delegate
                  noteText:(NSString *)text
               placeholder:(NSString *)placeholder {
  self.delegate = delegate;
  self.textView.text = text;
  self.textView.keyboardAppearance = [UIColor isNightMode] ? UIKeyboardAppearanceDark : UIKeyboardAppearanceDefault;
  self.textView.placeholder = placeholder;
}

- (void)updateTextViewForHeight:(CGFloat)height {
  id<MWMNoteCellDelegate> delegate = self.delegate;
  if (height > kMinimalTextViewHeight) {
    [delegate cell:self didChangeSizeAndText:self.textView.text];
  }

  [self setNeedsLayout];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
  if (context == kContext) {
    NSValue *s = change[NSKeyValueChangeNewKey];
    CGFloat height = s.CGSizeValue.height;
    [self updateTextViewForHeight:height];
    return;
  }

  [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

- (CGFloat)cellHeight {
  return self.textViewHeight.constant + 2 * kTopTextViewOffset;
}

- (CGFloat)textViewContentHeight {
  return self.textView.contentSize.height;
}

+ (CGFloat)minimalHeight {
  return kMinimalTextViewHeight;
}

- (void)textViewDidEndEditing:(UITextView *)textView {
  [self.delegate cell:self didFinishEditingWithText:textView.text];
  [self unregisterObserver];
}

- (void)textViewDidBeginEditing:(UITextView *)textView {
  [self registerObserver];
}

- (void)textViewDidChange:(UITextView *)textView {
  [textView sizeToFit];
}

- (void)unregisterObserver {
  [self.textView removeObserver:self forKeyPath:kTextViewContentSizeKeyPath context:kContext];
}

- (void)registerObserver {
  [self.textView addObserver:self
                  forKeyPath:kTextViewContentSizeKeyPath
                     options:NSKeyValueObservingOptionNew
                     context:kContext];
}

@end
