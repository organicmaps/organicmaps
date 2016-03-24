#import "Common.h"
#import "MWMDownloaderDialogHeader.h"
#import "MWMDownloadTransitMapAlert.h"
#import "Statistics.h"

static NSString * const kDownloaderDialogHeaderNibName = @"MWMDownloaderDialogHeader";

@interface MWMDownloaderDialogHeader ()

@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * size;
@property (weak, nonatomic) IBOutlet UIView * dividerView;
@property (weak, nonatomic) MWMDownloadTransitMapAlert * ownerAlert;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * sizeTrailing;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleLeading;

@end

@implementation MWMDownloaderDialogHeader

+ (instancetype)headerForOwnerAlert:(MWMDownloadTransitMapAlert *)alert title:(NSString *)title size:(NSString *)size;
{
  MWMDownloaderDialogHeader * header = [[[NSBundle mainBundle] loadNibNamed:kDownloaderDialogHeaderNibName owner:nil options:nil] firstObject];
  header.title.text = title;
  header.size.text = size;
  header.ownerAlert = alert;
  return header;
}

- (IBAction)headerButtonTap:(UIButton *)sender
{
  BOOL const currentState = sender.selected;
  [Statistics logEvent:kStatEventName(kStatDownloaderDialog, kStatExpand)
                   withParameters:@{kStatValue : currentState ? kStatOff : kStatOn}];
  sender.selected = !currentState;
  self.dividerView.hidden = currentState;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.expandImage.transform = sender.selected ? CGAffineTransformMakeRotation(M_PI) : CGAffineTransformIdentity;
  }];
  [self.ownerAlert showDownloadDetail:sender];
}

- (void)layoutSizeLabel
{
  if (self.expandImage.hidden)
    self.sizeTrailing.constant = self.titleLeading.constant;
  [self layoutIfNeeded];
}

@end
