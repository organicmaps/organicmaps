#import "MWMTextToSpeech.h"
#import "MWMTTSLanguageViewController.h"
#import "MWMTTSSettingsViewController.h"
#import "SelectableCell.h"
#import "UIColor+MapsMeColor.h"

static NSString * const kUnwingSegueIdentifier = @"UnwindToTTSSettings";

@implementation MWMTTSLanguageViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_tts_other_section_title");
  self.tableView.separatorColor = [UIColor blackDividers];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(SelectableCell *)sender
{
  if (![segue.identifier isEqualToString:kUnwingSegueIdentifier])
    return;
  MWMTTSSettingsViewController * dest = segue.destinationViewController;
  NSUInteger const row = [self.tableView indexPathForCell:sender].row;
  [dest setAdditionalTTSLanguage:[[MWMTextToSpeech tts] availableLanguages][row]];
}

#pragma mark - UITableViewDataSource && UITableViewDelegate

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [[MWMTextToSpeech tts] availableLanguages].size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  SelectableCell * cell = (SelectableCell *)[tableView dequeueReusableCellWithIdentifier:[SelectableCell className]];
  cell.titleLabel.text = @([[MWMTextToSpeech tts] availableLanguages][indexPath.row].second.c_str());
  return cell;
}

@end
