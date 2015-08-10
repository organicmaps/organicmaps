//
//  MWMTextToSpeech.h
//  Maps
//
//  Created by Vladimir Byko-Ianko on 10.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "std/string.hpp"
#include "std/vector.hpp"


@interface  MWMTextToSpeech : NSObject

- (instancetype)init;
- (bool)isEnabled;
- (void)enable:(bool)enalbed;
- (void)speakNotifications: (vector<string> const &)turnNotifications;

@end
