//
//  MWMDownloaderController.h
//  Maps
//
//  Created by v.mikhaylenko on 07.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMDownloaderController : UIViewController
- (instancetype)initWithViewController:(UIViewController *)viewController;
- (void)present;
- (void)close;
@end
