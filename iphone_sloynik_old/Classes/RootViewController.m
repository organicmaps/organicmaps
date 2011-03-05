//
//  RootViewController.m
//  Sloynik
//
//  Created by Yury Melnichek on 07.09.09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "RootViewController.h"
#import "PerfCount.h"

@implementation RootViewController

@synthesize searchField;
@synthesize suggestView;
@synthesize languageSwitch;
@synthesize menuButton;

- (void)viewDidLoad
{
	LogTimeCounter("StartTime", "RootViewController::viewDidLoad_begin");
    [super viewDidLoad];

	[searchField becomeFirstResponder];
	CGRect searchFieldFrame = searchField.frame;
	searchFieldFrame.size.height = 33;
	searchField.frame = searchFieldFrame;
	searchField.returnKeyType = UIReturnKeySearch;
	
	CGRect languageSwitchFrame = languageSwitch.frame;
	languageSwitchFrame.size.height = 33;
	languageSwitch.frame = languageSwitchFrame;
	
	CGRect menuButtonFrame = menuButton.frame;
	menuButtonFrame.size.height = 33;
	menuButton.frame = menuButtonFrame;

	suggestDataSource = [[SuggestDataSource alloc] init];
	suggestView.dataSource = suggestDataSource;
	suggestView.delegate = self;
	suggestView.rowHeight = 43;
	
	LogTimeCounter("StartTime", "RootViewController::viewDidLoad_end");
}

/*
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
}
*/
- (void)viewDidAppear:(BOOL)animated
{
	LogTimeCounter("StartTime", "RootViewController::viewDidAppear_begin");
    [super viewDidAppear:animated];
	LogTimeCounter("StartTime", "RootViewController::viewDidAppear_end");
}
/*
- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
}
*/
/*
- (void)viewDidDisappear:(BOOL)animated {
	[super viewDidDisappear:animated];
}
*/

/*
 // Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations.
	return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
 */

- (void)didReceiveMemoryWarning
{
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload
{
	// Release anything that can be recreated in viewDidLoad or on demand.
	// e.g. self.myOutlet = nil;
}

- (void)dealloc
{
	[suggestDataSource release];
	[searchField release];
	[suggestView release];
    [super dealloc];
}

#pragma mark UITableViewDelegate

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
	[searchField resignFirstResponder];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	[searchField resignFirstResponder];
    // Navigation logic may go here. Create and push another view controller.
	// AnotherViewController *anotherViewController = [[AnotherViewController alloc] initWithNibName:@"AnotherView" bundle:nil];
	// [self.navigationController pushViewController:anotherViewController];
	// [anotherViewController release];
}


/*
 // Override to support conditional editing of the table view.
 - (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
 // Return NO if you do not want the specified item to be editable.
 return YES;
 }
 */


/*
 // Override to support editing the table view.
 - (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
 
 if (editingStyle == UITableViewCellEditingStyleDelete) {
 // Delete the row from the data source
 [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:YES];
 }   
 else if (editingStyle == UITableViewCellEditingStyleInsert) {
 // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
 }   
 }
 */


/*
 // Override to support rearranging the table view.
 - (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
 }
 */


/*
 // Override to support conditional rearranging of the table view.
 - (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
 // Return NO if you do not want the item to be re-orderable.
 return YES;
 }
 */

#pragma mark View events

- (IBAction)onSearchFieldChanged
{
	[suggestDataSource setText:searchField.text];
	[suggestView reloadData];
	// [suggestView setList:[suggest getSuggestions:suggestionCount starting:0]];
}


@end

