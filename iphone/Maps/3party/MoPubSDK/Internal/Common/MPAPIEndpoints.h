//
//  MPAPIEndpoints.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

#define MOPUB_BASE_HOSTNAME                 @"ads.mopub.com"

#define MOPUB_API_PATH_AD_REQUEST           @"/m/ad"
#define MOPUB_API_PATH_NATIVE_POSITIONING   @"/m/pos"
#define MOPUB_API_PATH_OPEN                 @"/m/open"
#define MOPUB_API_PATH_CONSENT_DIALOG       @"/m/gdpr_consent_dialog"
#define MOPUB_API_PATH_CONSENT_SYNC         @"/m/gdpr_sync"

@interface MPAPIEndpoints : NSObject

+ (void)setUsesHTTPS:(BOOL)usesHTTPS;
+ (NSString *)baseURL;
+ (NSString *)baseURLStringWithPath:(NSString *)path;

@end
