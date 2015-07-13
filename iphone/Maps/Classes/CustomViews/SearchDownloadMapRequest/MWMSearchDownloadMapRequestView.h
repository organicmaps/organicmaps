//
//  MWMSearchDownloadMapRequestView.h
//  Maps
//
//  Created by Ilya Grechuhin on 09.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "UIKitCategories.h"

@interface MWMSearchDownloadMapRequestView : SolidTouchView

@property (nonatomic) BOOL hintHidden;

- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)init __attribute__((unavailable("init is not available")));

@end
