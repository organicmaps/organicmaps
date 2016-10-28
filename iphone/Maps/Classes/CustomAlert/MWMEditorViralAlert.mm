#import "Common.h"
#import "MWMActivityViewController.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMEditorViralAlert.h"
#import "Statistics.h"

#include "indexer/osm_editor.hpp"

#include "std/array.hpp"

namespace
{
  array<NSString *, 2> const kMessages {{L(@"editor_done_dialog_1"), L(@"editor_done_dialog_2")}};
} // namespace

@interface MWMEditorViralAlert ()

@property (weak, nonatomic) IBOutlet UILabel * message;
@property (weak, nonatomic) IBOutlet UIButton * shareButton;
@property (nonatomic) int indexOfMessage;

@end

@implementation MWMEditorViralAlert

+ (nonnull instancetype)alert
{
  MWMEditorViralAlert * alert = [[[NSBundle mainBundle] loadNibNamed:[MWMEditorViralAlert className] owner:nil options:nil] firstObject];
  alert.indexOfMessage = rand() % kMessages.size();
  NSString * message = kMessages[alert.indexOfMessage];
  if (alert.indexOfMessage == 1)
  {
    int const ratingValue = (rand() % 1000) + 1000;
    message = [NSString stringWithFormat:message, ratingValue];
  }
  alert.message.text = message;
  NSMutableDictionary <NSString *, NSString *> * info = [@{kStatValue : alert.statMessage} mutableCopy];
  if (NSString * un = osm_auth_ios::OSMUserName())
    [info setObject:un forKey:kStatOSMUserName];

  [Statistics logEvent:kStatEditorSecondTimeShareShow withParameters:info];
  return alert;
}

- (IBAction)shareTap
{
  [Statistics logEvent:kStatEditorSecondTimeShareClick withParameters:@{kStatValue : self.statMessage}];
  [self close:^{
    MWMActivityViewController * shareVC = [MWMActivityViewController shareControllerForEditorViral];
    [shareVC presentInParentViewController:self.alertController.ownerViewController
                                anchorView:self.shareButton];
  }];
}

- (IBAction)cancelTap
{
  [self close:nil];
}

- (NSString *)statMessage
{
  return self.indexOfMessage ? @"change" : @"rating";
}

@end
