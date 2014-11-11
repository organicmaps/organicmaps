//
//  MRADSectionIcon.h
//  MRAdService
//
//  Created by Aleksandr Karimov on 26.12.13.
//  Copyright (c) 2013 MailRu. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MRADSectionIcon : NSObject <NSCoding>

@property (nonatomic, readonly, copy) NSString *value;
@property (nonatomic, readonly, copy) NSString *icon;
@property (nonatomic, readonly, copy) NSString *iconHD;
@property (nonatomic, readonly, copy) NSString *style;

@property (nonatomic, copy, readonly) NSDictionary *jsonDict;

@end
