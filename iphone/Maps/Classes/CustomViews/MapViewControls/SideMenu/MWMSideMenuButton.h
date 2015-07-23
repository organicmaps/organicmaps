//
//  MWMSideMenuButton.h
//  Maps
//
//  Created by Ilya Grechuhin on 24.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMSideMenuButtonDelegate.h"
#import "MWMSideMenuDelegate.h"
#import "MWMSideMenuDownloadBadge.h"

@interface MWMSideMenuButton : UIView <MWMSideMenuDownloadBadgeOwner>

@property (nonatomic, readonly) CGRect frameWithSpacing;
@property (weak, nonatomic) id<MWMSideMenuInformationDisplayProtocol, MWMSideMenuButtonTapProtocol, MWMSideMenuButtonLayoutProtocol> delegate;
@property (weak, nonatomic) MWMSideMenuDownloadBadge * downloadBadge;

- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)init __attribute__((unavailable("init is not available")));

- (void)setup;
- (void)setHidden:(BOOL)hidden animated:(BOOL)animated;

@end
