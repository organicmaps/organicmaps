//
//  MWMTextToSpeech.h
//  Maps
//
//  Created by vbykoianko on 10.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "std/string.hpp"
#include "std/vector.hpp"

FOUNDATION_EXPORT NSString * const MWMTEXTTOSPEECH_ENABLE;
FOUNDATION_EXPORT NSString * const MWMTEXTTOSPEECH_DISABLE;

@interface MWMTextToSpeech : NSObject

- (instancetype)init;
- (BOOL)isEnable;
- (void)enable;
- (void)disable;
- (void)playTurnNotifications;

@end
