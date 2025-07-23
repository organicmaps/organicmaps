#import "MWMAutoupdateController.h"
#import "MWMCircularProgress.h"
#import "MWMStorage+UI.h"
#import "SwiftBridge.h"

#include "platform/downloader_defines.hpp"

#include <string>
#include <unordered_set>

namespace
{
NSString * RootId()
{
  return @(GetFramework().GetStorage().GetRootId().c_str());
}
enum class State
{
  Downloading,
  Waiting
};
}  // namespace

using namespace storage;

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
@property(weak, nonatomic) IBOutlet UILabel * progressLabel;
@property(weak, nonatomic) IBOutlet UILabel * legendLabel;

@property(weak, nonatomic) id<MWMCircularProgressProtocol> delegate;

@property(nonatomic) MWMCircularProgress * spinner;
@property(copy, nonatomic) NSString * updateSize;
@property(nonatomic) State state;

- (void)startSpinner;
- (void)stopSpinner;
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

- (void)setUpdateSize:(NSString *)updateSize
{
  _updateSize = updateSize;
  self.primaryButton.localizedText =
      [NSString stringWithFormat:L(@"whats_new_auto_update_button_size"), self.updateSize];
}

- (void)stateDownloading
{
  self.state = State::Downloading;
  self.primaryButton.hidden = YES;
  [self startSpinner];
  self.secondaryButton.localizedText = L(@"downloader_hide_screen");
}

- (void)stateWaiting
{
  self.state = State::Waiting;
  [self stopSpinner];
  self.primaryButton.hidden = NO;
  self.secondaryButton.localizedText = L(@"whats_new_auto_update_button_later");
}

- (void)startSpinner
{
  self.primaryButton.hidden = YES;
  self.spinnerView.hidden = NO;
  self.progressLabel.hidden = NO;
  self.legendLabel.hidden = NO;
  self.spinner = [MWMCircularProgress downloaderProgressForParentView:self.spinnerView];
  self.spinner.delegate = self.delegate;
  self.spinner.state = MWMCircularProgressStateSpinner;
}

- (void)stopSpinner
{
  self.primaryButton.hidden = NO;
  self.spinnerView.hidden = YES;
  self.progressLabel.hidden = YES;
  self.legendLabel.hidden = YES;
  self.spinner = nil;
}

- (void)setStatusForNodeName:(NSString *)nodeName rootAttributes:(NodeAttrs const &)nodeAttrs
{
  auto const progress = nodeAttrs.m_downloadingProgress;
  if (progress.m_bytesTotal > 0)
  {
    CGFloat const prog = kMaxProgress * static_cast<CGFloat>(progress.m_bytesDownloaded) / progress.m_bytesTotal;
    self.spinner.progress = prog;

    NSNumberFormatter * numberFormatter = [[NSNumberFormatter alloc] init];
    [numberFormatter setNumberStyle:NSNumberFormatterPercentStyle];
    [numberFormatter setMaximumFractionDigits:0];
    [numberFormatter setMultiplier:@100];
    NSString * percent = [numberFormatter stringFromNumber:@(prog)];
    NSString * downloadedSize = formattedSize(progress.m_bytesDownloaded);
    NSString * totalSize = formattedSize(progress.m_bytesTotal);
    self.progressLabel.text = [NSString stringWithFormat:L(@"downloader_percent"), percent, downloadedSize, totalSize];
  }
  else
  {
    self.progressLabel.text = @"";
  }

  BOOL const isApplying = nodeAttrs.m_status == storage::NodeStatus::Applying;
  NSString * format = L(isApplying ? @"downloader_applying" : @"downloader_process");
  self.legendLabel.text = [NSString stringWithFormat:format, nodeName];
}

@end

@interface MWMAutoupdateController () <MWMCircularProgressProtocol, MWMStorageObserver>
{
  std::unordered_set<CountryId> m_updatingCountries;
}

@property(nonatomic) Framework::DoAfterUpdate todo;
@property(nonatomic) MwmSize sizeInMB;
@property(nonatomic) NodeErrorCode errorCode;
@property(nonatomic) BOOL progressFinished;

@end

@implementation MWMAutoupdateController

+ (instancetype)instanceWithPurpose:(Framework::DoAfterUpdate)todo
{
  MWMAutoupdateController * controller = [[MWMAutoupdateController alloc] initWithNibName:[self className]
                                                                                   bundle:NSBundle.mainBundle];
  controller.todo = todo;
  auto view = static_cast<MWMAutoupdateView *>(controller.view);
  view.delegate = controller;
  [[MWMStorage sharedStorage] addObserver:controller];
  [controller updateSize];
  return controller;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  self.progressFinished = NO;
  MWMAutoupdateView * view = (MWMAutoupdateView *)self.view;
  if (self.todo == Framework::DoAfterUpdate::AutoupdateMaps)
  {
    [view stateDownloading];
    [[MWMStorage sharedStorage] updateNode:RootId()
                                  onCancel:^{
                                    [self updateSize];
                                    [view stateWaiting];
                                  }];
  }
  else
  {
    [view stateWaiting];
  }
}

- (void)dismiss
{
  [static_cast<MWMAutoupdateView *>(self.view) stopSpinner];
  [self dismissViewControllerAnimated:YES completion:^{ [[MWMStorage sharedStorage] removeObserver:self]; }];
}

- (void)updateSize
{
  auto containerView = static_cast<MWMAutoupdateView *>(self.view);
  auto const & s = GetFramework().GetStorage();
  storage::Storage::UpdateInfo updateInfo;
  s.GetUpdateInfo(s.GetRootId(), updateInfo);
  MwmSize const updateSizeInBytes = updateInfo.m_totalDownloadSizeInBytes;
  containerView.updateSize = formattedSize(updateSizeInBytes);
  _sizeInMB = updateSizeInBytes / MB;
}

- (IBAction)updateTap
{
  MWMAutoupdateView * view = (MWMAutoupdateView *)self.view;
  [view stateDownloading];
  [[MWMStorage sharedStorage] updateNode:RootId()
                                onCancel:^{
                                  [self updateSize];
                                  [view stateWaiting];
                                }];
}
- (IBAction)hideTap
{
  [self dismiss];
}

- (void)cancel
{
  auto view = static_cast<MWMAutoupdateView *>(self.view);
  UIAlertController * alertController = [UIAlertController alertControllerWithTitle:nil
                                                                            message:nil
                                                                     preferredStyle:UIAlertControllerStyleActionSheet];
  alertController.popoverPresentationController.sourceView = view.secondaryButton;
  alertController.popoverPresentationController.sourceRect = view.secondaryButton.bounds;
  auto cancelDownloadAction = [UIAlertAction actionWithTitle:L(@"cancel_download")
                                                       style:UIAlertActionStyleDestructive
                                                     handler:^(UIAlertAction * action) {
                                                       [[MWMStorage sharedStorage] cancelDownloadNode:RootId()];
                                                       [self dismiss];
                                                     }];
  [alertController addAction:cancelDownloadAction];
  auto cancelAction = [UIAlertAction actionWithTitle:L(@"cancel") style:UIAlertActionStyleCancel handler:nil];
  [alertController addAction:cancelAction];
  [self presentViewController:alertController animated:YES completion:nil];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [coordinator
      animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> _Nonnull context) {
        [static_cast<MWMAutoupdateView *>(self.view) updateForSize:size];
      }
                      completion:nil];
}

- (void)updateProcessStatus:(CountryId const &)countryId
{
  auto const & s = GetFramework().GetStorage();
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(s.GetRootId(), nodeAttrs);
  auto view = static_cast<MWMAutoupdateView *>(self.view);
  NSString * nodeName = @(s.GetNodeLocalName(countryId).c_str());
  [view setStatusForNodeName:nodeName rootAttributes:nodeAttrs];
  if (nodeAttrs.m_downloadingProgress.m_bytesDownloaded == nodeAttrs.m_downloadingProgress.m_bytesTotal)
    self.progressFinished = YES;
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(MWMCircularProgress *)progress
{
  [self cancel];
}

#pragma mark - MWMStorageObserver

- (void)processCountryEvent:(NSString *)countryId
{
  NodeStatuses nodeStatuses;
  GetFramework().GetStorage().GetNodeStatuses(countryId.UTF8String, nodeStatuses);
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
    case NodeStatus::OnDisk: m_updatingCountries.erase(countryId.UTF8String); break;
    default: m_updatingCountries.insert(countryId.UTF8String);
    }
  }

  if (self.progressFinished && m_updatingCountries.empty())
    [self dismiss];
  else
    [self updateProcessStatus:countryId.UTF8String];
}

- (void)processError
{
  [self updateSize];
  [static_cast<MWMAutoupdateView *>(self.view) stateWaiting];
  [[MWMStorage sharedStorage] cancelDownloadNode:RootId()];
}

- (void)processCountry:(NSString *)countryId downloadedBytes:(uint64_t)downloadedBytes totalBytes:(uint64_t)totalBytes
{
  if (m_updatingCountries.find(countryId.UTF8String) != m_updatingCountries.end())
    [self updateProcessStatus:countryId.UTF8String];
}

@end
