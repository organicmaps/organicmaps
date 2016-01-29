//
//  MRTrackerParams+Corp.h
//  myTrackerSDKCorp 1.4.0
//
//  Created by Igor Glotov on 24.03.15.
//  Copyright © 2015 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MyTrackerSDKCorp/MRTrackerParams.h>
#import <MyTrackerSDKCorp/MRCustomParamsProvider.h>

@interface MRTrackerParams (Corp)

- (MRCustomParamsProvider *)getCustomParams;

@end
