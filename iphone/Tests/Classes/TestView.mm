//
//  TestView.mm
//  Tests
//
//  Created by Siarhei Rachytski on 1/26/11.
//  Copyright 2011 Credo-Dialogue. All rights reserved.
//

#import "TestView.h"


@implementation TestView


- (id)initWithFrame:(CGRect)frame {
    
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code.
    }
    return self;
}


// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
	
	CGContextRef context = UIGraphicsGetCurrentContext();
	
	CGContextSetLineWidth(context, 2.0);
		
	CGContextSetStrokeColorWithColor(context, [UIColor blueColor].CGColor);
	CGFloat dashArray[] = {2, 6, 4, 2};
	
	CGContextSetLineDash(context, 3, dashArray, 4);	
	
	CGContextMoveToPoint(context, 
											 100 + rand() % 200, 
											 100 + rand() % 300);
	
	for (int i = 0; i < 100; ++i)
	  CGContextAddLineToPoint(context, 
														100 + rand() % 200, 
														100 + rand() % 300);
	
	CGContextStrokePath(context);
}

- (void)dealloc {
    [super dealloc];
}


@end
