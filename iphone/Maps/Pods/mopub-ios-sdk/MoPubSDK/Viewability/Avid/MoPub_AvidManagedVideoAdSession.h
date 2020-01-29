//
//  AvidManagedVideoAdSession.h
//  AppVerificationLibrary
//
//  Created by Daria Sukhonosova on 05/04/16.
//  Copyright © 2016 Integral. All rights reserved.
//

#import "MoPub_AbstractAvidManagedAdSession.h"
#import "MoPub_AvidVideoPlaybackListener.h"

@interface MoPub_AvidManagedVideoAdSession : MoPub_AbstractAvidManagedAdSession

@property(nonatomic, readonly) id<MoPub_AvidVideoPlaybackListener> avidVideoPlaybackListener;

@end
