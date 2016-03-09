#import "MWMEditorVirusAlert.h"

@interface MWMEditorVirusAlert ()

@property (weak, nonatomic) IBOutlet UILabel * message;
@property (copy, nonatomic) TMWMVoidBlock share;

@end

@implementation MWMEditorVirusAlert

+ (nonnull instancetype)alertWithShareBlock:(nonnull TMWMVoidBlock)share
{
  MWMEditorVirusAlert * alert = [[[NSBundle mainBundle] loadNibNamed:[MWMEditorVirusAlert className] owner:nil options:nil] firstObject];
  NSAssert(share, @"Share block can't be nil!");
  alert.share = share;
  return alert;
}

- (IBAction)shareTap
{
  //TODO(Vlad): Determine which message we have to show and log it.
  self.share();
  [self close];
}

- (IBAction)cancelTap
{
  //TODO(Vlad): Determine which message we have to show and log it.
  [self close];
}

@end
