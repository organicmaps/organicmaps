#import "MWMEditorViralAlert.h"

#include <array>

namespace
{
  array<NSString *, 3> const messages {{L(@"editor_done_dialog_1"), L(@"editor_done_dialog_2"), L(@"editor_done_dialog_3")}};
} // namespace

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
  int const index = rand() % messages.size();
  NSString * message = messages[index];
  alert.message.text = message;
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
