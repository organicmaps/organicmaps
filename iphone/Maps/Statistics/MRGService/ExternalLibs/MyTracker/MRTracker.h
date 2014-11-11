//
// Created by Igor Glotov on 22/07/14.
// Copyright (c) 2014 Mailru Group. All rights reserved.
// MyTracker, version 1.0.8

#import <Foundation/Foundation.h>

@class MRTrackerParams;


@interface MRTracker : NSObject

- (id)initWithAppId:(NSString *)id params:(MRTrackerParams *)params host:(NSString *)host;

+ (BOOL)enableLogging;

+ (void)setEnableLogging:(BOOL)enable;
@end