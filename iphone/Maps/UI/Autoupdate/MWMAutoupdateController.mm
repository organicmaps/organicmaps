#import "MWMAutoupdateController.h"
#import "MWMCircularProgress.h"
#import "MWMCommon.h"
#import "MWMFrameworkListener.h"
#import "MWMStorage.h"
#import "UIButton+RuntimeAttributes.h"

#include <vector>

namespace
{
  string RootId() { return GetFramework().GetStorage().GetRootId(); }
}  // namespace

@interface MWMAutoupdateView : UIView

@property(weak, nonatomic) IBOutlet UIImageView * image;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;

@property(weak, nonatomic) IBOutlet UILabel * title;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;

@property(weak, nonatomic) IBOutlet UILabel * text;

@property(weak, nonatomic) IBOutlet UIButton * primaryButton;
@property(weak, nonatomic) IBOutlet UIButton * secondaryButton;
@property(weak, nonatomic) IBOutlet UIView * spinnerView;

@property(weak, nonatomic) id<MWMCircularProgressProtocol> delegate;

@property(nonatomic) MWMCircularProgress * spinner;
@property(copy, nonatomic) NSString * updateSize;

- (void)startSpinner;
- (void)stopSpinner;
- (void)setProgress:(CGFloat)progress;
- (void)updateForSize:(CGSize)size;

@end

@implementation MWMAutoupdateView

- (void)setFrame:(CGRect)frame
{
  [self updateForSize:frame.size];
  super.frame = frame;
}

- (void)updateForSize:(CGSize)size
{
  BOOL const hideImage = (self.imageHeight.multiplier * size.height <= self.imageMinHeight.constant);
  self.titleImageOffset.priority = hideImage ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.image.hidden = hideImage;
  [self layoutIfNeeded];
}

- (void)stateDownloading
{
  self.primaryButton.hidden = YES;
  [self startSpinner];
  self.secondaryButton.localizedText = L(@"cancel");
  [MWMStorage updateNode:RootId()];
}

- (void)stateWaiting
{
  [self stopSpinner];
  self.primaryButton.hidden = NO;
  self.secondaryButton.localizedText = L(@"whats_new_auto_update_button_later");
  NSString * pattern = [L(@"whats_new_auto_update_button_size") stringByReplacingOccurrencesOfString:@"%s"
                                                            withString:@"%@"];
  self.primaryButton.localizedText = [NSString stringWithFormat:pattern, self.updateSize];
  [MWMStorage cancelDownloadNode:RootId()];
}

- (void)startSpinner
{
  self.primaryButton.hidden = YES;
  self.spinnerView.hidden = NO;
  self.spinner = [MWMCircularProgress downloaderProgressForParentView:self.spinnerView];
  self.spinner.delegate = self.delegate;
  [self.spinner setInvertColor:YES];
  self.spinner.state = MWMCircularProgressStateSpinner;
}

- (void)stopSpinner
{
  self.primaryButton.hidden = NO;
  self.spinnerView.hidden = YES;
  self.spinner = nil;
}

- (void)setProgress:(CGFloat)progress
{
  self.spinner.progress = progress;
}

@end

@interface MWMAutoupdateController () <MWMCircularProgressProtocol, MWMFrameworkStorageObserver>

@property(nonatomic) Framework::DoAfterUpdate todo;

@end

@implementation MWMAutoupdateController

+ (instancetype)instanceWithPurpose:(Framework::DoAfterUpdate)todo
{
  MWMAutoupdateController * controller = [[MWMAutoupdateController alloc] initWithNibName:[self className]
                                                                                   bundle:[NSBundle mainBundle]];
  controller.todo = todo;
  auto view = static_cast<MWMAutoupdateView *>(controller.view);
  view.delegate = controller;
  auto & f = GetFramework();
  auto const & s = f.GetStorage();
  NodeAttrs attrs;
  s.GetNodeAttrs(s.GetRootId(), attrs);
  TMwmSize const countrySizeInBytes = attrs.m_localMwmSize;
  view.updateSize = formattedSize(countrySizeInBytes);
  [MWMFrameworkListener addObserver:controller];
  return controller;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  auto view = static_cast<MWMAutoupdateView *>(self.view);
  if (self.todo == Framework::DoAfterUpdate::AutoupdateMaps)
    [view stateDownloading];
  else
    [view stateWaiting];
}

- (void)dismiss
{
  [static_cast<MWMAutoupdateView *>(self.view) stopSpinner];
  [self dismissViewControllerAnimated:YES completion:^{
    [MWMFrameworkListener removeObserver:self];
  }];
}

- (IBAction)updateTap
{
  [static_cast<MWMAutoupdateView *>(self.view) stateDownloading];
  [MWMStorage updateNode:RootId()];
}

- (IBAction)cancelTap
{
  [MWMStorage cancelDownloadNode:RootId()];
  [self dismiss];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext>  _Nonnull context) {
    [static_cast<MWMAutoupdateView *>(self.view) updateForSize:size];
  } completion:nil];
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(MWMCircularProgress *)progress
{
  [MWMStorage cancelDownloadNode:RootId()];
  [static_cast<MWMAutoupdateView *>(self.view) stateWaiting];
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  NodeStatuses nodeStatuses;
  GetFramework().GetStorage().GetNodeStatuses(countryId, nodeStatuses);
  if (nodeStatuses.m_status == NodeStatus::Error)
    [static_cast<MWMAutoupdateView *>(self.view) stateWaiting];
}

- (void)processCountry:(TCountryId const &)countryId
              progress:(MapFilesDownloader::TProgress const &)progress
{
  auto const & s = GetFramework().GetStorage();
  storage::TCountriesVec downloaded;
  storage::TCountriesVec _;
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(RootId(), nodeAttrs);
  auto const p = nodeAttrs.m_downloadingProgress;
  static_cast<MWMAutoupdateView *>(self.view).progress = static_cast<CGFloat>(p.first) / p.second;
  if (p.first == p.second)
    [self dismiss];
}

@end
