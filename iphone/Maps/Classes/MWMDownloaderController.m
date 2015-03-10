//
//  MWMDownloaderController.m
//  Maps
//
//  Created by v.mikhaylenko on 07.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDownloaderController.h"

@interface MWMDownloaderController ()
@property (nonatomic, weak) UIViewController *ownerViewController;
@end

@implementation MWMDownloaderController

- (instancetype)initWithViewController:(UIViewController *)viewController {
  self = [super init];
  if (self) {
    self.ownerViewController = viewController;
  }
  return self;
}

- (void)present {
  
}

- (void)close {
  
}

@end
