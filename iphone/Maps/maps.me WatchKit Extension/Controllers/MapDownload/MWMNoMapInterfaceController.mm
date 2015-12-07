#import "MWMNoMapInterfaceController.h"
#import "MWMWatchLocationTracker.h"
#import "MWMWatchEventInfo.h"
#import "MWMWatchNotification.h"
#import "MWMFrameworkUtils.h"
#import "Macros.h"
#import "Common.h"

#include "geometry/mercator.hpp"

extern NSString * const kNoMapControllerIdentifier = @"NoMapController";
static NSUInteger const kMaxPercent = 100;

static NSTimeInterval const kUpdateUIInterval = 1.0;

static NSUInteger const kProgressImagesCount = 20;
static NSUInteger const kProgressImagesStep = kMaxPercent / kProgressImagesCount;

@interface MWMNoMapInterfaceController ()

@property (weak, nonatomic) IBOutlet WKInterfaceLabel * statePercent;
@property (weak, nonatomic) IBOutlet WKInterfaceGroup * stateImage;
@property (weak, nonatomic) IBOutlet WKInterfaceLabel * stateDescription;

@property (weak, nonatomic) NSTimer * updateUITimer;
@property (nonatomic) MWMWatchNotification * notificationCenter;
@property (nonatomic) NSUInteger progress;
@property (nonatomic) BOOL needProgressUIUpdate;
@property (nonatomic) NSUInteger progressImageID;
@property (nonatomic) BOOL needProgressImageUIUpdate;

@end

@implementation MWMNoMapInterfaceController

- (void)awakeWithContext:(id)context
{
  [super awakeWithContext:context];
  [self.stateDescription setText:L(@"download_map_iphone")];
}

- (void)willActivate
{
  [super willActivate];
  if (self.haveLocation)
    [self configInitialUI];
}

- (void)didDeactivate
{
  [super didDeactivate];
  [self willLeaveController];
}

- (void)configInitialUI
{
  self.progress = NSUIntegerMax;
  self.updateUITimer = [NSTimer scheduledTimerWithTimeInterval:kUpdateUIInterval target:self selector:@selector(updateUI) userInfo:nil repeats:YES];
  // Handle retain cycle: self -> notificationCenter -> block -> self
  __weak MWMNoMapInterfaceController * weakSelf = self;
  self.notificationCenter = [[MWMWatchNotification alloc] init];
  [self.notificationCenter listenForMessageWithIdentifier:kDownloadingProgressUpdateNotificationId listener:^(NSNumber *progress)
  {
    __strong MWMNoMapInterfaceController * self = weakSelf;
    [self configUIForDownload];
    self.progress = MIN(kMaxPercent, MAX(0, kMaxPercent * progress.floatValue));
  }];
}

- (void)configUIForDownload
{
  dispatch_async(dispatch_get_main_queue(),
  ^{
    [self.statePercent setHidden:NO];
    [self.stateDescription setText:[MWMFrameworkUtils currentCountryName]];
  });
}

- (void)updateUI
{
  if (self.progress < kMaxPercent)
  {
    if (self.needProgressUIUpdate)
    {
      [self.statePercent setText:[NSString stringWithFormat:@"%@%%", @(self.progress)]];
      self.needProgressUIUpdate = NO;
    }

    if (self.needProgressImageUIUpdate)
    {
      // Workaround for Apple bug. If you try to load @"progress_1" it will load @"progress_10" instead.
      // Case @"progress_%@.png" doesn't fix it.
      NSString *imageName;
      if (self.progressImageID < 10)
        imageName = [NSString stringWithFormat:@"progress_0%@", @(self.progressImageID)];
      else
        imageName = [NSString stringWithFormat:@"progress_%@", @(self.progressImageID)];

      [self.stateImage setBackgroundImageNamed:imageName];
      self.needProgressImageUIUpdate = NO;
    }
  } else if (self.progress == kMaxPercent)
    [MWMFrameworkUtils resetFramework];
  if ([MWMFrameworkUtils hasMWM])
    [self popController];
}

- (void)willLeaveController
{
  [self.updateUITimer invalidate];
  [self.notificationCenter stopListeningForMessageWithIdentifier:kDownloadingProgressUpdateNotificationId];
  [MWMFrameworkUtils resetFramework];
}

- (void)popController
{
  [self willLeaveController];
  [super popController];
}

- (void)pushControllerWithName:(NSString *)name context:(id)context
{
  [self willLeaveController];
  [super pushControllerWithName:name context:context];
}

#pragma mark - Properties

- (void)setProgress:(NSUInteger)progress
{
  dispatch_async(dispatch_get_main_queue(),
  ^{
    self.needProgressUIUpdate |= (progress != _progress);
    _progress = progress;
    self.progressImageID = progress / kProgressImagesStep;
  });
}

- (void)setProgressImageID:(NSUInteger)progressImageID
{
  self.needProgressImageUIUpdate |= (_progressImageID != progressImageID);
  _progressImageID = progressImageID;
}

@end
