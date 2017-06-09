//
//  MTRGPromoCardViewProtocol.h
//  myTargetSDK 4.6.15
//
//  Created by Andrey Seredkin on 20.10.16.
//  Copyright © 2016 Mail.ru. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MTRGMediaAdView;

@protocol MTRGPromoCardViewProtocol <NSObject>

@required

@property(nonatomic, readonly) UILabel *titleLabel;
@property(nonatomic, readonly) UILabel *descriptionLabel;
@property(nonatomic, readonly) UILabel *ctaButtonLabel;
@property(nonatomic, readonly) MTRGMediaAdView *mediaAdView;

- (CGFloat)heightWithCardWidth:(CGFloat)width;

@end
