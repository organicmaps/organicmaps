#import "MWMAutoupdateController.h"
#import "MWMCircularProgress.h"
#import "MWMCommon.h"
#import "MWMFrameworkListener.h"
#import "MWMStorage.h"
#import "Statistics.h"
#import "UIButton+RuntimeAttributes.h"

#include <unordered_set>

namespace
{
string RootId() { return GetFramework().GetStorage().GetRootId(); }
enum class State
{
  Downloading,
  Waiting
};
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
@property(nonatomic) State state;

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
  self.state = State::Downloading;
  self.primaryButton.hidden = YES;
  [self startSpinner];
  self.secondaryButton.localizedText = L(@"cancel");
  [MWMStorage updateNode:RootId()];
}

- (void)stateWaiting
{
  self.state = State::Waiting;
  [self stopSpinner];
  self.primaryButton.hidden = NO;
  self.secondaryButton.localizedText = L(@"whats_new_auto_update_button_later");
  NSString * pattern = [L(@"whats_new_auto_update_button_size") stringByReplacingOccurrencesOfString:@"%s"
                                                            withString:@"%@"];
  self.primaryButton.localizedText = [NSString stringWithFormat:pattern, self.updateSize];
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
@property(nonatomic) TMwmSize sizeInMB;
@property(nonatomic) NodeErrorCode errorCode;
@property(nonatomic) BOOL progressFinished;

@end

@implementation MWMAutoupdateController

std::unordered_set<TCountryId> updatingCountries;

+ (instancetype)instanceWithPurpose:(Framework::DoAfterUpdate)todo
{
  MWMAutoupdateController * controller = [[MWMAutoupdateController alloc] initWithNibName:[self className]
                                                                                   bundle:[NSBundle mainBundle]];
  controller.todo = todo;
  auto view = static_cast<MWMAutoupdateView *>(controller.view);
  view.delegate = controller;
  auto & f = GetFramework();
  auto const & s = f.GetStorage();
  storage::Storage::UpdateInfo updateInfo;
  s.GetUpdateInfo(s.GetRootId(), updateInfo);
  TMwmSize const updateSizeInBytes = updateInfo.m_totalUpdateSizeInBytes;
  view.updateSize = formattedSize(updateSizeInBytes);
  controller.sizeInMB = updateSizeInBytes / MB;
  [MWMFrameworkListener addObserver:controller];
  return controller;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  self.progressFinished = NO;
  [Statistics logEvent:kStatDownloaderOnStartScreenShow
        withParameters:@{kStatMapDataSize : @(self.sizeInMB)}];
  auto view = static_cast<MWMAutoupdateView *>(self.view);
  if (self.todo == Framework::DoAfterUpdate::AutoupdateMaps)
  {
    [view stateDownloading];
    [Statistics logEvent:kStatDownloaderOnStartScreenAutoDownload
          withParameters:@{kStatMapDataSize : @(self.sizeInMB)}];
  }
  else
  {
    [view stateWaiting];
  }
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
  [Statistics logEvent:kStatDownloaderOnStartScreenManualDownload
        withParameters:@{kStatMapDataSize : @(self.sizeInMB)}];
}

- (IBAction)cancelTap
{
  [MWMStorage cancelDownloadNode:RootId()];
  [self dismiss];
  auto view = static_cast<MWMAutoupdateView *>(self.view);
  [Statistics logEvent:view.state == State::Downloading ?
                                     kStatDownloaderOnStartScreenCancelDownload :
                                     kStatDownloaderOnStartScreenSelectLater
        withParameters:@{kStatMapDataSize : @(self.sizeInMB)}];
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
  [Statistics logEvent:kStatDownloaderOnStartScreenCancelDownload
        withParameters:@{kStatMapDataSize : @(self.sizeInMB)}];
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  NodeStatuses nodeStatuses;
  GetFramework().GetStorage().GetNodeStatuses(countryId, nodeStatuses);
  if (nodeStatuses.m_status == NodeStatus::Error)
  {
    self.errorCode = nodeStatuses.m_error;
    SEL const process = @selector(processError);
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:process object:nil];
    [self performSelector:process withObject:nil afterDelay:0.2];
  }

  if (!nodeStatuses.m_groupNode)
  {
    switch (nodeStatuses.m_status)
    {
    case NodeStatus::Error:
    case NodeStatus::OnDisk: updatingCountries.erase(countryId); break;
    default: updatingCountries.insert(countryId);
    }
  }
  
  if (self.progressFinished && updatingCountries.empty())
    [self dismiss];
}

- (void)processError
{
  [static_cast<MWMAutoupdateView *>(self.view) stateWaiting];
  [MWMStorage cancelDownloadNode:RootId()];
  auto errorType = ^NSString * (NodeErrorCode code)
  {
    switch (code)
    {
    case storage::NodeErrorCode::NoError:
      LOG(LWARNING, ("Incorrect error type"));
      return @"";
    case storage::NodeErrorCode::NoInetConnection:
      return @"no internet connection";
    case storage::NodeErrorCode::UnknownError:
      return  @"unknown error";
    case storage::NodeErrorCode::OutOfMemFailed:
      return @"not enough space";
    }
  }(self.errorCode);
  
  [Statistics logEvent:kStatDownloaderOnStartScreenError
        withParameters:@{kStatMapDataSize : @(self.sizeInMB), kStatType : errorType}];
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
  auto view = static_cast<MWMAutoupdateView *>(self.view);
  view.progress = kMaxProgress * static_cast<CGFloat>(p.first) / p.second;
  if (p.first == p.second)
    self.progressFinished = YES;
}

@end
