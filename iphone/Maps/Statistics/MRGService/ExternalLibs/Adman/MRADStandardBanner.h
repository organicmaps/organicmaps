//
//  MRADStandardBanner.h
//  MRAdMan
//
//  Created by Пучка Илья on 24.03.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import "MRADBanner.h"

/*
 Стандартный баннер
*/
@interface MRADStandardBanner : MRADBanner

@property (nonatomic, copy, readonly) NSString *imageLink;
@property (nonatomic, readonly) CGFloat imageWidth;
@property (nonatomic, readonly) CGFloat imageHeight;
@property (nonatomic, copy, readonly) NSString *ageRestrictions;
@property (nonatomic, copy, readonly) NSString *disclaimer;

@end
