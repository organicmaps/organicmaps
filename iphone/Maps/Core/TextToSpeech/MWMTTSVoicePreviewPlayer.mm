#import "MWMTTSVoicePreviewPlayer.h"
#import <AVFoundation/AVFoundation.h>
#import "TTSTester.h"

@interface MWMTTSVoicePreviewPlayer ()

@property(nonatomic, nullable) MWMTTSVoice * playingVoice;

@end

@implementation MWMTTSVoicePreviewPlayer
{
  TTSTester * _tester;
}

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    _tester = [[TTSTester alloc] init];
    // Nothing calls -viewWillDisappear when the app is backgrounded, so a preview would otherwise
    // keep speaking over the lock screen.
    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(stop)
                                               name:UIApplicationWillResignActiveNotification
                                             object:nil];
  }
  return self;
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self];
  [self stop];
}

- (void)play:(MWMTTSVoice *)voice
{
  AVSpeechSynthesisVoice * speechVoice = [AVSpeechSynthesisVoice voiceWithIdentifier:voice.identifier];
  NSString * phrase = [[_tester getTestStrings:speechVoice.language] firstObject];
  // Fall back to the voice's own name for a language with no sample phrases, e.g. Cantonese.
  if (phrase.length == 0)
    phrase = speechVoice.name;
  if (phrase.length == 0)
    return;

  self.playingVoice = voice;
  __weak __typeof(self) weakSelf = self;
  [[MWMTextToSpeech tts] speakPreview:phrase
                      voiceIdentifier:voice.identifier
                           completion:^{
                             __typeof(self) self = weakSelf;
                             self.playingVoice = nil;
                             if (self.onFinished)
                               self.onFinished();
                           }];
}

- (void)stop
{
  if (!self.playingVoice)
    return;
  self.playingVoice = nil;
  [[MWMTextToSpeech tts] stopPreview];
}

@end
