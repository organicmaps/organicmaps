//
//  MRADFullscreenOrientationSettings.h
//  MRAdMan
//
//  Created by Пучка Илья on 24.03.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, MRADImageOrientation) {
    MRADImageOrientationPortrait,
    MRADImageOrientationLandscape
};

@interface MRADBannerImage : NSObject <NSCopying>

- (instancetype)initWithImage:(UIImage *)image source:(NSString *)url;

@property (nonatomic, copy, readonly) NSString *url;
@property (nonatomic, readonly) CGFloat width;
@property (nonatomic, readonly) CGFloat height;
@property (nonatomic, readonly) MRADImageOrientation orientation;

@property (nonatomic, copy, readonly) NSData *imageData;

@property (nonatomic, copy, readonly) NSDictionary *jsonDict;

@end
