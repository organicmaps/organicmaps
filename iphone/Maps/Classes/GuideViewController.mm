//
//  GuideViewController.mm
//  Maps
//
//  Created by Yury Melnichek on 15.03.11.
//  Copyright 2011 MapsWithMe. All rights reserved.
//

#import "GuideViewController.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "ArticleVC.h"
#include "global.hpp"

@implementation GuideViewController

@synthesize activityIndicator;
@synthesize loadingLabel;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
      // Custom initialization
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];

    // Release any cached data, images, etc that aren't in use.
}

- (IBAction)OnMapClicked:(id)sender
{
  UISegmentedControl * pSegmentedControl = (UISegmentedControl *)sender;
  int const selectedIndex = pSegmentedControl.selectedSegmentIndex;

  if (selectedIndex != 1)
  {
    LOG(LINFO, (selectedIndex));
    [UIView transitionFromView:self.view
                        toView:[MapsAppDelegate theApp].mapViewController.view
                      duration:0
                       options:UIViewAnimationOptionTransitionNone
                    completion:nil];
    [pSegmentedControl setSelectedSegmentIndex:1];
  }
}

- (void)onEmptySearch
{
  // Do nothing. Don't hide the results view.
}

- (void)willShowArticleVC:(ArticleVC *) viewController
{
  [super willShowArticleVC:viewController];
  viewController.articleFormat = @
  "<html>"
  "  <head>"
  "    <meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>"
  "    <meta name='viewport' content='width=device-width; initial-scale=1.0; "
  "                                   maximum-scale=1.0; user-scalable=0;'/>"
  "    <style type='text/css'>"
  "      img, object { border:none; max-width:280px; height:auto; }"
  "      div { clear:both; }"
  "      div.thumbcaption div.magnify { display:none; }"
  "      div.thumbinner { padding:6px; margin:6px 0 0 0; border:1px solid #777;"
  "                       -webkit-border-radius:6px;"
  "                       font-size:12px; display:table; }"
  "      div#content h2 button { display:none; }"
  "    </style>"
  "  </head>"
  "  <body style='-webkit-text-size-adjust:%d%%'>"
  "    %@"
  "  </body>"
  "</html>";
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  // Do any additional setup after loading the view from its nib.
  if (!GetSloynikEngine())
  {
    CGRect frame = self.view.frame;
    self.activityIndicator = [[[UIActivityIndicatorView alloc]
                               initWithFrame:CGRectMake(frame.origin.x + frame.size.width / 2 - 15,
                                                        80, 30, 30)] autorelease];
    
    [self.activityIndicator setActivityIndicatorViewStyle:UIActivityIndicatorViewStyleGray];
    [self.view addSubview:self.activityIndicator];
    
    self.loadingLabel = [[[UILabel alloc]
                          initWithFrame:CGRectMake(frame.origin.x + frame.size.width / 2 - 30,
                                                   110, 60, 30)] autorelease];
    self.loadingLabel.text = @"Indexing...";
    self.loadingLabel.textAlignment = UITextAlignmentCenter;
    self.loadingLabel.userInteractionEnabled = NO;
    self.loadingLabel.numberOfLines = 1;
    self.loadingLabel.baselineAdjustment = UIBaselineAdjustmentAlignCenters;
    self.loadingLabel.adjustsFontSizeToFitWidth = YES;
    [self.view addSubview:self.loadingLabel];

    self.searchBar.hidden = YES;
    self.resultsView.hidden = YES;

    [self.activityIndicator startAnimating];
  }
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return YES;
}

- (void)OnSloynikEngineInitialized
{
  if (self.activityIndicator)
  {
    [self.activityIndicator removeFromSuperview];
    [self.loadingLabel removeFromSuperview];
    self.activityIndicator = nil;
    self.searchBar.hidden = NO;
    self.resultsView.hidden = NO;
    [self.resultsView reloadData];
  }  
}

@end
