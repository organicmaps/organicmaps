#import "MWMBookmarkCell.h"
#import "MWMPlacePageCellUpdateProtocol.h"
#import "MWMPlacePageProtocol.h"

namespace
{
void * kContext = &kContext;
// https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/KeyValueObserving/Articles/KVOBasics.html
NSString * const kTextViewContentSizeKeyPath = @"contentSize";
}  // namespace

@interface MWMBookmarkCell ()

@property(weak, nonatomic) IBOutlet UITextView * textView;
@property(weak, nonatomic) IBOutlet UIImageView * spinner;
@property(weak, nonatomic) IBOutlet UIButton * editButton;

@property(weak, nonatomic) IBOutlet UIImageView * gradientView;

@property(nonatomic) IBOutlet NSLayoutConstraint * textViewHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * moreButtonHeight;
@property(nonatomic) IBOutlet NSLayoutConstraint * textViewZeroHeight;

@property(weak, nonatomic) id<MWMPlacePageCellUpdateProtocol> updateCellDelegate;
@property(weak, nonatomic) id<MWMPlacePageButtonsProtocol> editBookmarkDelegate;

@property(copy, nonatomic) NSAttributedString * attributedHTML;

@property(nonatomic) BOOL isOpen;

@end

@implementation MWMBookmarkCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self registerObserver];
}

- (void)dealloc { [self unregisterObserver]; }
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

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context == kContext)
  {
    NSValue * s = change[@"new"];
    auto const height = s.CGSizeValue.height;
    auto const boundedHeight = self.textViewHeight.constant;

    if (height < boundedHeight || self.isOpen)
      [self stateOpen:YES];
    else
      [self stateOpen:NO];

    [self setNeedsLayout];
    [self.updateCellDelegate cellUpdated];
    return;
  }

  [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

- (void)configureWithText:(NSString *)text
       updateCellDelegate:(id<MWMPlacePageCellUpdateProtocol>)updateCellDelegate
     editBookmarkDelegate:(id<MWMPlacePageButtonsProtocol>)editBookmarkDelegate
                   isHTML:(BOOL)isHTML
{
  self.attributedHTML = nil;
  self.isOpen = NO;
  self.textViewHeight.active = NO;
  self.textViewZeroHeight.active = NO;
  self.updateCellDelegate = updateCellDelegate;
  self.editBookmarkDelegate = editBookmarkDelegate;

  if (!text.length)
    [self configWithEmptyDescription];
  else if (isHTML)
    [self configHTML:text];
  else
    [self configPlain:text];
}

- (void)configWithEmptyDescription
{
  [self stopSpinner];

  [self stateOpen:YES];
  self.textViewZeroHeight.active = YES;
}

- (void)configHTML:(NSString *)text
{
  if (self.attributedHTML)
  {
    [self stopSpinner];
    self.textViewZeroHeight.active = NO;
    self.textView.attributedText = self.attributedHTML;
    // In case when after setting attributed text into textView its content height is about 0 but
    // not 0 (e.g. 0.33).
    // When it happens we need to call sizeToFit to display bookmark description.
    if (self.textView.contentSize.height < 1)
      [self.textView sizeToFit];
  }
  else
  {
    self.textViewZeroHeight.active = YES;
    [self startSpinner];
    [self stateOpen:YES];

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
      NSDictionary<NSString *, id> * attr = @{
        NSForegroundColorAttributeName : [UIColor blackPrimaryText],
        NSFontAttributeName : [UIFont regular16]
      };
      NSError * error = nil;
      NSData * data = [text dataUsingEncoding:NSUnicodeStringEncoding];
      NSMutableAttributedString * str = [[NSMutableAttributedString alloc]
                initWithData:data
                     options:@{
                       NSDocumentTypeDocumentAttribute : NSHTMLTextDocumentType
                     }
          documentAttributes:nil
                       error:&error];
      if (error)
      {
        // If we failed while attempting to render html than just show plain text in bookmark.
        // Usually there is a problem only in iOS7.
        self.attributedHTML = [[NSAttributedString alloc] initWithString:text attributes:attr];
      }
      else
      {
        [str addAttributes:attr range:{0, str.length}];
        self.attributedHTML = str;
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        [self configHTML:nil];
      });
    });
  }
}

- (void)configPlain:(NSString *)text
{
  [self stopSpinner];
  self.textView.text = text;
}

- (void)stateOpen:(BOOL)isOpen
{
  self.moreButtonHeight.constant = isOpen ? 0 : 33;
  self.textViewHeight.active = !isOpen;
  self.gradientView.hidden = isOpen;
}

- (void)startSpinner
{
  self.editButton.hidden = YES;
  NSUInteger const animationImagesCount = 12;
  NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  NSString * postfix = [UIColor isNightMode] ? @"dark" : @"light";
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
  {
    UIImage * image =
        [UIImage imageNamed:[NSString stringWithFormat:@"Spinner_%@_%@", @(i + 1), postfix]];
    animationImages[i] = image;
  }
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

- (IBAction)moreTap
{
  self.isOpen = YES;
  [self stateOpen:YES];
  [self setNeedsLayout];
}

- (IBAction)editTap { [self.editBookmarkDelegate editBookmark]; }

@end
