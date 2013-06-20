//
//  TwoButtonsView.h
//  Maps
//
//  Created by Kirill on 04/06/2013.
//  Copyright (c) 2013 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TwoButtonsView : UIView
-(id)initWithFrame:(CGRect)frame leftButtonSelector:(SEL)leftSel rightButtonSelector:(SEL)rightTitle leftButtonTitle:(NSString *)leftTitle rightButtontitle:(NSString *)rightTitle target:(id)target;
//-(void)changeWidth:(double)newWidth;
@end
