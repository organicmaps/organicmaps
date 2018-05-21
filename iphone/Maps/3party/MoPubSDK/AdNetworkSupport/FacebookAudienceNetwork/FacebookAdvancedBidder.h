//
//  FacebookAdvancedBidder.h
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#if __has_include(<MoPub/MoPub.h>)
#import <MoPub/MoPub.h>
#else
#import "MPAdvancedBidder.h"
#endif


@interface FacebookAdvancedBidder : NSObject<MPAdvancedBidder>
@property (nonatomic, copy, readonly) NSString * creativeNetworkName;
@property (nonatomic, copy, readonly) NSString * token;
@end
