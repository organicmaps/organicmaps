//
//  ShareActionSheet.h
//  Maps
//
//  Created by Kirill on 05/06/2013.
//  Copyright (c) 2013 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface ShareActionSheet : NSObject
+(void)showShareActionSheetInView:(id)view withObject:(id)del;
+(void)resolveActionSheetChoice:(UIActionSheet *)as buttonIndex:(NSInteger)buttonIndex text:(NSString *)text
                           view:(id)view delegate:(id)del scale:(double)scale gX:(double)gX gY:(double)gY
                  andMyPosition:(BOOL)myPos;
@end
