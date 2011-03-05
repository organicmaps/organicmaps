//
//  Suggest.mm
//  Sloynik
//
//  Created by Yury Melnichek on 11.09.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "SuggestDataSource.h"


@implementation SuggestDataSource

- (void)setText:(NSString *)newText
{
	[text_ release];
	text_ = newText;
	[text_ retain];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	static NSString * cellId = @"Id";
	UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:cellId];
	if (cell == nil)
	{
		cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cellId] autorelease];
	}
		
	
	cell.text = [NSString stringWithFormat:@"%@ %i", text_, indexPath.row];
	
	return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	if ([text_ length] > 20)
		return 20;
	else
		return [text_ length];
}

@end
