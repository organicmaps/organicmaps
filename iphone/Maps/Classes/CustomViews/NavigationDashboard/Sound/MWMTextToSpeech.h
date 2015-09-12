#import <Foundation/Foundation.h>

#include "std/string.hpp"
#include "std/vector.hpp"

@interface MWMTextToSpeech : NSObject

- (instancetype)init;
- (BOOL)isEnable;
- (void)enable;
- (void)disable;
- (void)playTurnNotifications;

@end
