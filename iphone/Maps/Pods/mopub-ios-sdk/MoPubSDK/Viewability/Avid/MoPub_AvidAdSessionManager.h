//
//  AvidAdSessionManager.h
//  AppVerificationLibrary
//
//  Created by Daria Sukhonosova on 05/04/16.
//  Copyright © 2016 Integral. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MoPub_AvidDisplayAdSession.h"
#import "MoPub_AvidManagedDisplayAdSession.h"
#import "MoPub_AvidVideoAdSession.h"
#import "MoPub_AvidManagedVideoAdSession.h"
#import "MoPub_ExternalAvidAdSessionContext.h"

@interface MoPub_AvidAdSessionManager : NSObject

+ (NSString *)version;
+ (NSString *)releaseDate;

+ (MoPub_AvidVideoAdSession *)startAvidVideoAdSessionWithContext:(MoPub_ExternalAvidAdSessionContext *)avidAdSessionContext;
+ (MoPub_AvidDisplayAdSession *)startAvidDisplayAdSessionWithContext:(MoPub_ExternalAvidAdSessionContext *)avidAdSessionContext;
+ (MoPub_AvidManagedVideoAdSession *)startAvidManagedVideoAdSessionWithContext:(MoPub_ExternalAvidAdSessionContext *)avidAdSessionContext;
+ (MoPub_AvidManagedDisplayAdSession *)startAvidManagedDisplayAdSessionWithContext:(MoPub_ExternalAvidAdSessionContext *)avidAdSessionContext;

@end
