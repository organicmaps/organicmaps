//
//  MRDimmingView.h
//  MoPub
//
//  Created by Andrew He on 12/19/11.
//  Copyright (c) 2011 MoPub, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MRDimmingView : UIView {
    BOOL _dimmed;
    CGFloat _dimmingOpacity;
}

@property (nonatomic, assign) BOOL dimmed;
@property (nonatomic, assign) CGFloat dimmingOpacity;

@end
