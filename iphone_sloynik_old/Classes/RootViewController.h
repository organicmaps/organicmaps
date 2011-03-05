//
//  RootViewController.h
//  Sloynik
//
//  Created by Yury Melnichek on 07.09.09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "SuggestDataSource.h"

@interface RootViewController : UIViewController <UITableViewDelegate>
{
	UITextField * searchField;
	UITableView * suggestView;
	UISegmentedControl * languageSwitch;
	UIControl * menuButton;
	SuggestDataSource * suggestDataSource;
}
- (IBAction)onSearchFieldChanged;

@property (nonatomic, retain) IBOutlet UITextField * searchField;
@property (nonatomic, retain) IBOutlet UITableView * suggestView;
@property (nonatomic, retain) IBOutlet UISegmentedControl * languageSwitch;
@property (nonatomic, retain) IBOutlet UIControl * menuButton;

@end
