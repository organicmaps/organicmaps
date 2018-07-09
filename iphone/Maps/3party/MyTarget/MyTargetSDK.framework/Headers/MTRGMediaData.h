//
//  MTRGMediaData.h
//  myTargetSDK 4.6.15
//
//  Created by Timur Voloshin on 05.19.16.
//  Copyright © 2016 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>


@interface MTRGMediaData : NSObject

@property(nonatomic, readonly, copy) NSString *url;
@property(nonatomic) id data;
@property(nonatomic) CGSize size;

- (instancetype)initWithUrl:(NSString *)url;

@end
