#import "MWMActionBarButton.h"
#import "MWMButton.h"
#import "MWMCircularProgress.h"
#import "SwiftBridge.h"

NSString * titleForButton(MWMActionBarButtonType type, BOOL isSelected)
{
  switch (type)
  {
  case MWMActionBarButtonTypeDownload: return L(@"download");
  case MWMActionBarButtonTypeCall: return L(@"placepage_call_button");
  case MWMActionBarButtonTypeBookmark: return L(isSelected ? @"delete" : @"save");
  case MWMActionBarButtonTypeTrack: return L(@"delete");
  case MWMActionBarButtonTypeNavigateTrack: return L(@"navigate_track");
  case MWMActionBarButtonTypeSaveTrackRecording: return L(@"save");
  case MWMActionBarButtonTypeDeleteTrackRecording: return L(@"delete");
  case MWMActionBarButtonTypeRouteFrom: return L(@"p2p_from_here");
  case MWMActionBarButtonTypeRouteTo: return L(@"p2p_to_here");
  case MWMActionBarButtonTypeMore: return L(@"placepage_more_button");
  case MWMActionBarButtonTypeRouteAddStop: return L(@"placepage_add_stop");
  case MWMActionBarButtonTypeRouteAddDestination: return L(@"route_add_destination");
  case MWMActionBarButtonTypeRouteReplaceStop: return L(@"placepage_replace_stop");
  case MWMActionBarButtonTypeRouteRemoveStop: return L(@"placepage_remove_stop");
  case MWMActionBarButtonTypeAvoidToll: return L(@"avoid_tolls");
  case MWMActionBarButtonTypeAvoidDirty: return L(@"avoid_unpaved");
  case MWMActionBarButtonTypeAvoidFerry: return L(@"avoid_ferry");
  }
}

typedef NS_ENUM(NSInteger, MWMBookmarksButtonState) {
  MWMBookmarksButtonStateSave,
  MWMBookmarksButtonStateDelete,
  MWMBookmarksButtonStateRecover,
};

@interface MWMActionBarButton () <MWMCircularProgressProtocol>

@property(nonatomic) MWMActionBarButtonType type;
@property(nonatomic) MWMCircularProgress * mapDownloadProgress;
@property(weak, nonatomic) IBOutlet MWMButton * button;
@property(weak, nonatomic) IBOutlet UILabel * label;
@property(weak, nonatomic) id<MWMActionBarButtonDelegate> delegate;

- (void)setBookmarkButtonState:(MWMBookmarksButtonState)state;

@end

@implementation MWMActionBarButton

- (void)configButton:(BOOL)isSelected primary:(BOOL)isPrimary
{
  self.label.text = titleForButton(self.type, isSelected);
  self.button.coloring = isPrimary ? MWMButtonColoringBlue : MWMButtonColoringBlack;
  [self.button.imageView setContentMode:UIViewContentModeScaleAspectFit];

  switch (self.type)
  {
  case MWMActionBarButtonTypeDownload:
  {
    if (self.mapDownloadProgress)
      return;

    self.mapDownloadProgress = [MWMCircularProgress downloaderProgressForParentView:self.button];
    self.mapDownloadProgress.delegate = self;

    MWMCircularProgressStateVec affectedStates =
        @[@(MWMCircularProgressStateNormal), @(MWMCircularProgressStateSelected)];

    [self.mapDownloadProgress setImageName:@"ic_download" forStates:affectedStates];
    [self.mapDownloadProgress setColoring:MWMButtonColoringBlue forStates:affectedStates];
    break;
  }
  case MWMActionBarButtonTypeCall:
    [self.button setImage:[UIImage imageNamed:@"ic_placepage_phone_number"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeBookmark:
    BOOL hasRecentlyDeletedBookmark = [MWMBookmarksManager.sharedManager hasRecentlyDeletedBookmark];
    MWMBookmarksButtonState state =
        isSelected ? MWMBookmarksButtonStateDelete
                   : (hasRecentlyDeletedBookmark ? MWMBookmarksButtonStateRecover : MWMBookmarksButtonStateSave);
    [self setBookmarkButtonState:state];
    break;
  case MWMActionBarButtonTypeNavigateTrack:
    self.button.accessibilityLabel = L(@"navigate_track");
    [self.button setImage:[UIImage imageNamed:@"ic_navigate_track"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeTrack:
    [self.button setImage:[[UIImage imageNamed:@"ic_route_manager_trash"]
                              imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate]
                 forState:UIControlStateNormal];
    self.button.coloring = MWMButtonColoringRed;
    break;
  case MWMActionBarButtonTypeSaveTrackRecording:
    [self.button setImage:[UIImage imageNamed:@"ic_placepage_save_track_recording"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeDeleteTrackRecording:
    [self.button setImage:[[UIImage imageNamed:@"ic_route_manager_trash"]
                              imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate]
                 forState:UIControlStateNormal];
    self.button.coloring = MWMButtonColoringRed;
    break;
  case MWMActionBarButtonTypeRouteFrom:
    [self.button setImage:[UIImage imageNamed:@"ic_route_from"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeRouteTo:
    [self.button setImage:[UIImage imageNamed:@"ic_route_to"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeRouteAddDestination:
    [self.button setImage:[UIImage imageNamed:@"ic_route_to"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeMore:
    [self.button setImage:[UIImage imageNamed:@"ic_placepage_more"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeRouteAddStop:
  case MWMActionBarButtonTypeRouteReplaceStop:
    [self.button setImage:[UIImage imageNamed:@"ic_add_route_point"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeRouteRemoveStop:
    [self.button setImage:[UIImage imageNamed:@"ic_remove_route_point"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeAvoidToll:
    [self.button setImage:[UIImage imageNamed:@"ic_avoid_tolls"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeAvoidDirty:
    [self.button setImage:[UIImage imageNamed:@"ic_avoid_dirty"] forState:UIControlStateNormal];
    break;
  case MWMActionBarButtonTypeAvoidFerry:
    [self.button setImage:[UIImage imageNamed:@"ic_avoid_ferry"] forState:UIControlStateNormal];
    break;
  }

  // Keep the title color consistent with the icon tint.
  switch (self.button.coloring)
  {
  case MWMButtonColoringBlue: self.label.styleName = @"PPActionBarTitleBlue"; break;
  case MWMButtonColoringRed: self.label.styleName = @"PPActionBarTitleRed"; break;
  default: self.label.styleName = @"PPActionBarTitle"; break;
  }
}

+ (MWMActionBarButton *)buttonWithDelegate:(id<MWMActionBarButtonDelegate>)delegate
                                buttonType:(MWMActionBarButtonType)type
                                isSelected:(BOOL)isSelected
                                 isPrimary:(BOOL)isPrimary
{
  MWMActionBarButton * button = [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  button.delegate = delegate;
  button.type = type;
  [button configButton:isSelected primary:isPrimary];
  return button;
}

- (void)progressButtonPressed:(MWMCircularProgress *)progress
{
  [self.delegate tapOnButtonWithType:self.type];
}

- (IBAction)tap
{
  [self.delegate tapOnButtonWithType:self.type];
}

- (void)setBookmarkButtonState:(MWMBookmarksButtonState)state
{
  NSString * imageName = state == MWMBookmarksButtonStateDelete ? @"ic_bookmarks_on" : @"ic_bookmarks_off";
  [self.button setImage:[UIImage imageNamed:imageName] forState:UIControlStateNormal];
  [self.button setImage:[UIImage imageNamed:@"ic_bookmarks_on"] forState:UIControlStateHighlighted];

  switch (state)
  {
  case MWMBookmarksButtonStateSave: self.label.text = L(@"save"); break;
  case MWMBookmarksButtonStateDelete: self.label.text = L(@"delete"); break;
  case MWMBookmarksButtonStateRecover: self.label.text = L(@"restore"); break;
  }
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
  return [self pointInside:point withEvent:event] ? self.button : nil;
}

@end
