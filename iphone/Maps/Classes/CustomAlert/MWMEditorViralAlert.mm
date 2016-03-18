#import "MWMEditorViralAlert.h"
#import "Statistics.h"

#include "indexer/osm_editor.hpp"

#include <array>

namespace
{
  array<NSString *, 3> const kMessages {{L(@"editor_done_dialog_1"), L(@"editor_done_dialog_2"), L(@"editor_done_dialog_3")}};
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
  int const index = rand() % kMessages.size();
  NSString * message = kMessages[index];
  if (index == 1)
  {
    auto const edits = osm::Editor::Instance().GetStats().m_edits.size();
    int const ratingValue = (rand() % 1000) + 1000;
    int const rating = ceil(ratingValue / (ceil(edits / 10)));
    message = [NSString stringWithFormat:message, edits, rating];
  }
  alert.message.text = message;
  [[Statistics instance] logEvent:kStatEditorSecondTimeShareShow withParameters:@{kStatValue : message}];
  return alert;
}

- (IBAction)shareTap
{
  [[Statistics instance] logEvent:kStatEditorSecondTimeShareClick withParameters:@{kStatValue : self.message.text}];
  self.share();
  [self close];
}

- (IBAction)cancelTap
{
  [self close];
}

@end
