//
//  MRAbstractDataProvider.h
//  myTrackerSDKCorp 1.4.3
//
//  Created by Igor Glotov on 23.07.14.
//  Copyright © 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MRJsonBuilder;

@interface MRAbstractDataProvider : NSObject

- (void)collectData;

- (void)putDataToBuilder:(MRJsonBuilder *)builder;
@end
