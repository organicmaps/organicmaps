//
//  UILabel+RuntimeAttributes.h
//  Maps
//
//  Created by v.mikhaylenko on 10.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UILabel (RuntimeAttributes)
@property (nonatomic, copy) NSString *localizedText;
@end

@interface UIButton (RuntimeAttributes)
@property (nonatomic, copy) NSString *localizedText;
@end
