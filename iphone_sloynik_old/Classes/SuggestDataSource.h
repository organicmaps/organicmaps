//
//  Suggest.h
//  Sloynik
//
//  Created by Yury Melnichek on 11.09.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SuggestDataSource : NSObject <UITableViewDataSource>
{
	NSString * text_;
}

- (void)setText:(NSString *)text;

@end
