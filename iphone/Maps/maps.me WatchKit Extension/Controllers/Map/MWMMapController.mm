#import "MWMMapController.h"
#import "MWMWatchEventInfo.h"
#import "MWMWatchLocationTracker.h"
#import "MWMFrameworkUtils.h"
#import "Macros.h"
#import "Common.h"

static NSTimeInterval const kDrawUpdateInterval = 0.2;

static NSString * const kCategoriesControllerIdentifier = @"CategoriesController";

extern NSString * const kNoMapControllerIdentifier;

static int const kZoomModifierUp = 2;
static int const kZoomModifierDefault = 0;
static int const kZoomModifierDown = -2;

@interface MWMMapController()

@property (weak, nonatomic) IBOutlet WKInterfaceGroup * mapGroup;

@property (weak, nonatomic) NSTimer * drawTimer;
@property (nonatomic) BOOL needUpdateMap;
@property (nonatomic) BOOL initSoftwareRenderer;
@property (nonatomic) int zoomModifier;

@end

@implementation MWMMapController

- (void)awakeWithContext:(id)context
{
  [super awakeWithContext:context];
}

- (void)willActivate
{
  [super willActivate];
  if (!self.haveLocation)
    return;
  [self stubAnimation];
  [self configure];
}

- (void)didDeactivate
{
  [super didDeactivate];
  self.drawTimer = nil;
  self.initSoftwareRenderer = NO;
}

- (void)stubAnimation
{
  [self.mapGroup setBackgroundImageNamed:@"spinner_"];
  [self.mapGroup startAnimating];
}

- (void)configure
{
  if ([[[NSUserDefaults alloc] initWithSuiteName:kApplicationGroupIdentifier()] boolForKey:kHaveAppleWatch])
  {
    [self checkHasMapAndShowUI];
    [self configureMenu];
  }
  else
  {
    NSMutableDictionary * migrationRequest = [NSMutableDictionary dictionary];
    migrationRequest.watchEventInfoRequest = MWMWatchEventInfoRequestMoveWritableDir;
    [WKInterfaceController openParentApplication:migrationRequest reply:^(NSDictionary *replyInfo, NSError *error)
    {
      if (error)
        NSLog(@"WatchKit Extension, MWMMapController, MWMWatchEventInfoRequestMoveWritableDir request failed with error: %@", error);
      else
        [self configure];
    }];
  }
}

- (void)configureMenu
{
  [self clearAllMenuItems];
  [self addMenuItemWithImageNamed:@"ic_search" title:L(@"nearby") action:@selector(searchTap)];
  if ([MWMWatchLocationTracker sharedLocationTracker].hasDestination)
    [self addMenuItemWithImageNamed:@"ic_close" title:L(@"clear_pin") action:@selector(finishRoute)];
}

- (void)checkHasMapAndShowUI
{
  if ([MWMFrameworkUtils hasMWM])
  {
    self.initSoftwareRenderer = YES;
    [self updateMapAtZoomDefault];
    [self forceUpdateMap];
    self.drawTimer = [NSTimer scheduledTimerWithTimeInterval:kDrawUpdateInterval target:self selector:@selector(updateMap) userInfo:nil repeats:YES];
  }
  else
    [self pushControllerWithName:kNoMapControllerIdentifier context:nil];
}

- (void)searchTap
{
  [self pushControllerWithName:kCategoriesControllerIdentifier context:nil];
}

- (void)forceUpdateMap
{
  self.needUpdateMap = YES;
  [self updateMap];
}

- (void)updateMap
{
  if (!self.needUpdateMap)
    return;
  self.needUpdateMap = NO;
  [self updateUIToDestination];
  UIImage * frame = [MWMFrameworkUtils getFrame:self.contentFrame.size
                                       withScreenScale:[WKInterfaceDevice currentDevice].screenScale
                                       andZoomModifier:self.zoomModifier];
  [self.mapGroup setBackgroundImage:frame];
}

- (void)updateUIToDestination
{
  MWMWatchLocationTracker *tracker = [MWMWatchLocationTracker sharedLocationTracker];
  if (tracker.hasDestination)
    [self setTitle:[tracker distanceToPoint:tracker.destinationPosition]];
  else
    [self setTitle:@""];
}

- (void)pushControllerWithName:(NSString *)name context:(id)context
{
  self.drawTimer = nil;
  [super pushControllerWithName:name context:context];
}

#pragma mark - MWMWatchLocationTrackerDelegate

- (void)onChangeLocation:(CLLocation *)location
{
  self.needUpdateMap = YES;
}

#pragma mark - Properties

- (void)setDrawTimer:(NSTimer *)drawTimer
{
  [_drawTimer invalidate];
  _drawTimer = drawTimer;
}

- (void)setInitSoftwareRenderer:(BOOL)initSoftwareRenderer
{
  if (!initSoftwareRenderer && _initSoftwareRenderer)
    [MWMFrameworkUtils releaseSoftwareRenderer];
  else if (initSoftwareRenderer && !_initSoftwareRenderer)
    [MWMFrameworkUtils initSoftwareRenderer:[WKInterfaceDevice currentDevice].screenScale];
  _initSoftwareRenderer = initSoftwareRenderer;
}

- (void)setZoomModifier:(int)zoomModifier
{
  if (_zoomModifier != zoomModifier)
  {
    _zoomModifier = zoomModifier;
    [self forceUpdateMap];
  }
}

#pragma mark - Interface actions

- (void)finishRoute
{
  [[MWMWatchLocationTracker sharedLocationTracker] clearDestination];
  [self updateUIToDestination];
  [self configureMenu];
  [self stubAnimation];
  [self forceUpdateMap];
}

- (IBAction)updateMapAtZoomUp
{
  self.zoomModifier = kZoomModifierUp;
}

- (IBAction)updateMapAtZoomDefault
{
  self.zoomModifier = kZoomModifierDefault;
}

- (IBAction)updateMapAtZoomDown
{
  self.zoomModifier = kZoomModifierDown;
}

@end
