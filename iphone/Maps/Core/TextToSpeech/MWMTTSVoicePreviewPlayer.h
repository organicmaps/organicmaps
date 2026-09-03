#import "MWMTextToSpeech.h"

NS_ASSUME_NONNULL_BEGIN

// Plays a sample phrase in a given voice for the settings screens. Previewing never changes the
// voice the turn notifications are read in.
NS_SWIFT_NAME(TTSVoicePreviewPlayer)
@interface MWMTTSVoicePreviewPlayer : NSObject

// The voice being previewed, nil when nothing is playing.
@property(nonatomic, readonly, nullable) MWMTTSVoice * playingVoice;
// Called on the main queue when playback ends on its own. Not called by an explicit -stop.
@property(nonatomic, copy, nullable) void (^onFinished)(void);

- (void)play:(MWMTTSVoice *)voice;
- (void)stop;

@end

NS_ASSUME_NONNULL_END
