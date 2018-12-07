#import "MWMPlaceDescriptionCell.h"
#import "SwiftBridge.h"

@interface MWMPlaceDescriptionCell ()

@property(weak, nonatomic) IBOutlet UITextView * textView;
@property(copy, nonatomic) NSAttributedString * attributedHTML;
@property(copy, nonatomic) NSString * originalText;
@property(weak, nonatomic) id<MWMPlacePageButtonsProtocol> delegate;

@end

@implementation MWMPlaceDescriptionCell

- (void)configureWithDescription:(NSString *)text delegate:(id<MWMPlacePageButtonsProtocol>)delegate
{
  self.delegate = delegate;
  self.attributedHTML = nil;
  self.originalText = text;
  
  self.textView.textContainer.lineBreakMode = NSLineBreakByTruncatingTail;
  [self configHTML:text];
}

- (void)configHTML:(NSString *)text
{
  if (self.attributedHTML)
  {
    if (self.attributedHTML.length <= 500) {
      self.textView.attributedText = self.attributedHTML;
    } else {
      self.textView.attributedText = [self.attributedHTML attributedSubstringFromRange:NSMakeRange(0, 500)];
    }
  }
  else
  {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
      auto font = [UIFont regular16];
      auto color = [UIColor blackPrimaryText];
      auto str = [[NSMutableAttributedString alloc] initWithHtmlString:text baseFont:font];
      if (str)
      {
        [str addAttribute:NSForegroundColorAttributeName value:color range:NSMakeRange(0, str.length)];
        self.attributedHTML = str;
      }
      else
      {
        self.attributedHTML =
        [[NSAttributedString alloc] initWithString:text
                                        attributes:@{
                                                     NSFontAttributeName : font,
                                                     NSForegroundColorAttributeName : color
                                                     }];
      }
      
      dispatch_async(dispatch_get_main_queue(), ^{
        [self configHTML:nil];
      });
    });
  }
}

- (IBAction)moreButtonPressed:(UIButton *)sender {
  [self.delegate showPlaceDescription:self.originalText];
}

@end
