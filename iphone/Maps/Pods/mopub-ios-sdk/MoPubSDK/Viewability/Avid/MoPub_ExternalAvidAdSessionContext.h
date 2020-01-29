//
//  ExternalAvidAdSessionContext.h
//  AppVerificationLibrary
//
//  Created by Daria Sukhonosova on 18/04/16.
//  Copyright © 2016 Integral. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MoPub_ExternalAvidAdSessionContext : NSObject

@property(nonatomic, readonly) NSString *partnerVersion;
@property(nonatomic, readonly) BOOL isDeferred;

+ (instancetype)contextWithPartnerVersion:(NSString *)partnerVersion;
+ (instancetype)contextWithPartnerVersion:(NSString *)partnerVersion isDeferred:(BOOL)isDeferred;

- (instancetype)initWithPartnerVersion:(NSString *)partnerVersion;
- (instancetype)initWithPartnerVersion:(NSString *)partnerVersion isDeferred:(BOOL)isDeferred;

@end
