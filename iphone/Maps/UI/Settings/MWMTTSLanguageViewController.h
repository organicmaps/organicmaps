#import "MWMTableViewController.h"

// Base class for the language and voice pickers. Adds per-row voice preview: a leading play/stop
// button on each voice cell that speaks a random sample phrase, like the iOS VoiceOver voice picker.
@interface MWMTTSPreviewTableViewController : MWMTableViewController

@end

// Lists all languages that have an installed TTS voice. A language with a single voice is selected
// directly (and can be previewed in place); a language with several voices pushes
// MWMTTSVoiceViewController to pick one.
@interface MWMTTSLanguageViewController : MWMTTSPreviewTableViewController

@end

// Lists the voices available for a given language and lets the user pick or preview one. Selecting a
// voice stores the language + voice and returns to the TTS settings screen.
@interface MWMTTSVoiceViewController : MWMTTSPreviewTableViewController

@property(nonatomic, copy) NSString * languageBcp47;
@property(nonatomic, copy) NSString * languageName;

@end
