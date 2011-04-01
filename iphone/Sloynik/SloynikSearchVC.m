//
//  SloynikSearchVC.m
//  Sloynik
//
//  Created by Yury Melnichek on 01.04.11.
//  Copyright 2011 -. All rights reserved.
//

#import "SloynikSearchVC.h"


@implementation SloynikSearchVC

@synthesize menuButton;


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)dealloc
{
  [super dealloc];
  self.menuButton = nil;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];

    // Release any cached data, images, etc that aren't in use.
}

// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView
{
  int const buttonW = 40;
  int const toolbarH = 44;
  int const marginW = 5;
  CGRect frame = [[UIScreen mainScreen] applicationFrame];
  int const frameW = frame.size.width;
  int const frameH = frame.size.height;

  CGRect searchBarFrame = CGRectMake(0, 0, frameW - buttonW - 2 * marginW, toolbarH);
  CGRect navBarFrame = CGRectMake(frameW - buttonW - 2 * marginW, 0,
                                  buttonW + 2 * marginW, toolbarH);
  CGRect resultsFrame = CGRectMake(0, toolbarH, frameW, frameH - toolbarH);

  self.searchBar = [[[UISearchBar alloc] initWithFrame:searchBarFrame] autorelease];
  self.searchBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  [self.searchBar becomeFirstResponder];

  self.menuButton = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"menu-20.png"]
                                                     style:UIBarButtonItemStylePlain
                                                    target:self
                                                    action:@selector(menuButtonPressed)];

  UINavigationItem * navItem = [[[UINavigationItem alloc] initWithTitle:@""] autorelease];
  navItem.rightBarButtonItem = self.menuButton;

  UINavigationBar * navBar = [[[UINavigationBar alloc] initWithFrame:navBarFrame] autorelease];
  navBar.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
  [navBar pushNavigationItem:navItem animated:NO];

  self.resultsView = [[[UITableView alloc] initWithFrame:resultsFrame] autorelease];
  self.resultsView.rowHeight = 40;
  self.resultsView.autoresizingMask = (UIViewAutoresizingFlexibleWidth |
                                       UIViewAutoresizingFlexibleHeight);

  UIView * mainView = [[[UIView alloc] initWithFrame:frame] autorelease];
  [mainView addSubview:self.searchBar];
  [mainView addSubview:navBar];
  [mainView addSubview:self.resultsView];
  self.view = mainView;
}

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
}
*/

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  // Return YES for supported orientations
  return YES;
}

- (void)menuButtonPressed
{
}

@end
