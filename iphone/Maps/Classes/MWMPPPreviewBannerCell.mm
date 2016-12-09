#import "MWMPPPreviewBannerCell.h"
#import "Common.h"
#import "MapViewController.h"
#import "MWMPlacePageLayoutImpl.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

namespace
{
CGFloat const kDefaultBodyLeftOffset = 16;
CGFloat const kPreviewWithImageBodyLeftOffset = 47;
CGFloat const kOpenBodyLeftOffset = 60;
CGFloat const kPreviewImageSide = 20;
CGFloat const kOpenImageSide = 28;
CGFloat const kPreviewImageTopOffset = 8;
CGFloat const kOpenImageTopOffset = 12;
CGFloat const kParagraphSpacing = 5;
CGFloat const kLineSpacing = 5;
}  // namespace

@interface MWMPPPreviewBannerCell ()

@property(weak, nonatomic) IBOutlet UIImageView * icon;
@property(weak, nonatomic) IBOutlet UILabel * body;
@property(weak, nonatomic) IBOutlet UIButton * button;
@property(nonatomic) NSURL * adURL;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * imageWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * imageTopOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * bodyLeftOffset;

@end

@implementation MWMPPPreviewBannerCell

- (void)configWithTitle:(NSString *)title
                content:(NSString *)content
                  adURL:(NSURL *)adURL
               imageURL:(NSURL *)imageURL
{
  NSAssert(title.length, @"Title must be not empty!");
  self.adURL = adURL;
  auto full = [[NSMutableAttributedString alloc] initWithString:title attributes:
                                        @{NSForegroundColorAttributeName : [UIColor blackPrimaryText],
                                                     NSFontAttributeName : [UIFont medium16]}];
  [full appendAttributedString:[[NSAttributedString alloc] initWithString:@"\n"]];

  if (content.length)
  {
    auto attrContent = [[NSAttributedString alloc] initWithString:content attributes:
                                        @{NSForegroundColorAttributeName : [UIColor blackSecondaryText],
                                                     NSFontAttributeName : [UIFont regular13]}];

    [full appendAttributedString:attrContent];
  }

  auto paragraphStyle = [[NSMutableParagraphStyle alloc] init];
  paragraphStyle.paragraphSpacing = kParagraphSpacing;
  paragraphStyle.lineSpacing = kLineSpacing;

  [full addAttributes:@{NSParagraphStyleAttributeName : paragraphStyle} range:{0, full.length}];

  self.body.attributedText = full;

  self.icon.hidden = YES;
  self.bodyLeftOffset.constant = kDefaultBodyLeftOffset;
  self.button.hidden = !IPAD;

  if (!imageURL)
    return;

  [self downloadAssingImageWithURL:imageURL completion:^
  {
    if (IPAD)
      [self configImageInOpenState];
    else
      [self configImageInPreviewState];
  }];
}

- (void)downloadAssingImageWithURL:(NSURL *)URL completion:(TMWMVoidBlock)completion
{
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    UIImage * image = [UIImage imageWithData:[NSData dataWithContentsOfURL:URL]];
    dispatch_async(dispatch_get_main_queue(), ^{
      self.icon.image = image;
      if (image)
        self.icon.hidden = NO;
      completion();
    });
  });
}

- (void)configImageInPreviewState
{
  self.button.hidden = YES;
  if (self.icon.hidden)
    return;

  self.bodyLeftOffset.constant = kPreviewWithImageBodyLeftOffset;
  self.imageWidth.constant = self.imageHeight.constant = kPreviewImageSide;
  self.imageTopOffset.constant = kPreviewImageTopOffset;
  [self commitLayoutAnimated];
}

- (void)configImageInOpenState
{
  self.button.hidden = NO;
  if (self.icon.hidden)
    return;

  self.bodyLeftOffset.constant = kOpenBodyLeftOffset;
  self.imageWidth.constant = self.imageHeight.constant = kOpenImageSide;
  self.imageTopOffset.constant = kOpenImageTopOffset;
  [self commitLayoutAnimated];
}

- (void)commitLayoutAnimated
{
  [self setNeedsLayout];
  [UIView animateWithDuration:place_page_layout::kAnimationSpeed animations:^{ [self layoutIfNeeded]; }];
}

- (IBAction)tap
{
  if (self.adURL)
    [[MapViewController controller] openUrl:self.adURL];
}

@end
