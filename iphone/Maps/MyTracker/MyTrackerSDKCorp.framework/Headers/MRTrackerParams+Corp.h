//
//  MRTrackerParams+Corp.h
//  myTrackerSDKCorp 1.3.2
//
//  Created by Igor Glotov on 24.03.15.
//  Copyright © 2015 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MyTrackerSDKCorp/MRTrackerParams.h>
#import <MyTrackerSDKCorp/MRCustomParamsProvider.h>

@interface MRTrackerParams (Corp)

- (MRCustomParamsProvider *)getCustomParams;

- (NSTimeInterval)launchTimeout;
- (void)setLaunchTimeout:(NSTimeInterval)launchTimeout;


@end
