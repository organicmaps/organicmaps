//
//  MTRGTracer.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Igor Glotov on 23.07.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MTRGTracer : NSObject

//Property is deprecated, use [MTRGManager setLoggingEnabled:YES];
@property (nonatomic) BOOL enableLogging __attribute__((deprecated));

+(MTRGTracer *) sharedTracer;

@end

extern void mtrg_tracer_i(NSString *, ...);
extern void mtrg_tracer_d(NSString *, ...);
extern void mtrg_tracer_e(NSString *, ...);

