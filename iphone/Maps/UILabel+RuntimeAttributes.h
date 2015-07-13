//
//  UILabel+RuntimeAttributes.h
//  Maps
//
//  Created by v.mikhaylenko on 10.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MWMTextView.h"

@interface UILabel (RuntimeAttributes)
@property (nonatomic) NSString * localizedText;
@end

@interface MWMTextView (RuntimeAttributes)
@property (nonatomic) NSString * localizedPlaceholder;
@end

@interface UITextField (RuntimeAttributes)
@property (nonatomic) NSString * localizedPlaceholder;
@end
