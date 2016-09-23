//
// Created by Timur on 5/23/16.
// Copyright (c) 2016 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MTRGTracer : NSObject

+ (BOOL)enabled;

+ (void)setEnabled:(BOOL)enabled;

@end

extern void mtrg_tracer_i(NSString *, ...);

extern void mtrg_tracer_d(NSString *, ...);

extern void mtrg_tracer_e(NSString *, ...);