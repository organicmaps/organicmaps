//
//  MWMSideMenuDownloadBadge.h
//  Maps
//
//  Created by Ilya Grechuhin on 14.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMSideMenuDownloadBadge;
@protocol MWMSideMenuDownloadBadgeOwner <NSObject>

@property (weak, nonatomic) MWMSideMenuDownloadBadge * downloadBadge;

@end

@interface MWMSideMenuDownloadBadge : UIView

@property (nonatomic) NSUInteger outOfDateCount;

- (void)showAnimatedAfterDelay:(NSTimeInterval)delay;

@end
