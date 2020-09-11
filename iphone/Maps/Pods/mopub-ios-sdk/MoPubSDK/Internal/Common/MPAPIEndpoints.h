//
//  MPAPIEndpoints.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

#define MOPUB_API_PATH_AD_REQUEST           @"/m/ad"
#define MOPUB_API_PATH_NATIVE_POSITIONING   @"/m/pos"
#define MOPUB_API_PATH_OPEN                 @"/m/open"
#define MOPUB_API_PATH_CONSENT_DIALOG       @"/m/gdpr_consent_dialog"
#define MOPUB_API_PATH_CONSENT_SYNC         @"/m/gdpr_sync"

@interface MPAPIEndpoints : NSObject

@property (nonatomic, copy, class) NSString * baseHostname;
@property (nonatomic, copy, readonly, class) NSString * baseURL;

+ (void)setUsesHTTPS:(BOOL)usesHTTPS;
+ (NSURLComponents *)baseURLComponentsWithPath:(NSString *)path;

@end
