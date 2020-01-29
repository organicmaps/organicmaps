//
// Created by Daria Sukhonosova on 11/05/16.
// Copyright (c) 2016 Integral. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MoPub_AbstractAvidAdSession.h"

@interface MoPub_AbstractAvidManagedAdSession : MoPub_AbstractAvidAdSession

- (void)injectJavaScriptResource:(NSString *)javaScriptResource;

@end
