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

@implementation GuideViewController


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
    [[MapsAppDelegate theApp].mapViewController Invalidate];
  }
}

- (void)onEmptySearch
{
  // Do nothing. Don't hide the results view.
}

- (void)willShowArticleVC:(ArticleVC *) articleVC
{
  [super willShowArticleVC:articleVC];
  articleVC.articleFormat = @
  "<html>"
  "  <head>"
  "    <meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>"
  "    <script type='text/javascript'>"
  "      function tg(id) {"
  "        if (document.getElementById('section-'+id).style.display == 'block') {"
  "          document.getElementById('section-'+id).style.display = 'none';"
  "          document.getElementById('button-'+id).innerHTML = 'Show';"
  "        } else {"
  "          document.getElementById('section-'+id).style.display = 'block';"
  "          document.getElementById('button-'+id).innerHTML = 'Hide';"
  "        }"
  "      }"
  "    </script>"
  "    <style type='text/css'>"
  "      img, object { border:none; max-width:280px; height:auto; }"
  "      div.loadHide { display:none; }"
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

@end
