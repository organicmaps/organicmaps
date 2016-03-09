#import "MWMEditorViralAlert.h"

@interface MWMEditorViralAlert ()

@property (weak, nonatomic) IBOutlet UILabel * message;
@property (copy, nonatomic) TMWMVoidBlock share;

@end

@implementation MWMEditorViralAlert

+ (nonnull instancetype)alertWithShareBlock:(nonnull TMWMVoidBlock)share
{
  MWMEditorViralAlert * alert = [[[NSBundle mainBundle] loadNibNamed:[MWMEditorViralAlert className] owner:nil options:nil] firstObject];
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
