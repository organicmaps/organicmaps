/*******************************************************************************

 Copyright (c) 2013, MapsWithMe GmbH
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************/

#import "MasterViewController.h"
#import "CityDetailViewController.h"
#import "City.h"

#import "MapsWithMeAPI.h"

@implementation MasterViewController

- (void)showAllCitiesOnTheMap:(id)sender
{
  size_t const capitalsCount = sizeof(CAPITALS)/sizeof(CAPITALS[0]);

  NSMutableArray * array = [[NSMutableArray alloc] initWithCapacity:capitalsCount];

  for (size_t i = 0; i < capitalsCount; ++i)
  {
    NSString * pinId = [[[NSString alloc] initWithFormat:@"%ld", i] autorelease];
    // Note that url is empty - it means "More details" button for a pin in MapsWithMe will lead back to this example app
    MWMPin * pin = [[[MWMPin alloc] initWithLat:CAPITALS[i].lat lon:CAPITALS[i].lon title:CAPITALS[i].name and:pinId] autorelease];
    [array addObject:pin];
  }

  [MWMApi showPins:array];

  [array release];
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
  self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
  if (self)
  {
    self.title = NSLocalizedString(@"World Capitals", nil);
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
    {
      self.clearsSelectionOnViewWillAppear = NO;
      self.contentSizeForViewInPopover = CGSizeMake(320.0, 600.0);
    }
  }
  return self;
}

- (void)dealloc
{
  self.detailViewController = nil;
  [super dealloc];
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  UIBarButtonItem * showMapButton = [[[UIBarButtonItem alloc] initWithTitle:@"Show All" style:UIBarButtonItemStyleDone target:self action:@selector(showAllCitiesOnTheMap:)] autorelease];
  self.navigationItem.rightBarButtonItem = showMapButton;
}

#pragma mark - Table View

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return sizeof(CAPITALS)/sizeof(CAPITALS[0]);
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  UILabel * label = [[[UILabel alloc] initWithFrame:CGRectMake(0, 0, 240, tableView.rowHeight)] autorelease];
  label.text = [MWMApi isApiSupported] ? @"MapsWithMe is installed" : @"MapsWithMe is not installed";
  label.textAlignment = UITextAlignmentCenter;
  label.backgroundColor = [UIColor clearColor];
  return label;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return tableView.rowHeight / 2;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  static NSString * cellId = @"MasterCell";

  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:cellId];
  if (cell == nil)
  {
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cellId] autorelease];
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone)
      cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  }

  cell.textLabel.text = CAPITALS[indexPath.row].name;
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
    self.detailViewController.cityIndex = indexPath.row;
  else
  {
    if (!self.detailViewController)
      self.detailViewController = [[[CityDetailViewController alloc] initWithNibName:@"CityDetailViewController" bundle:nil] autorelease];
    self.detailViewController.cityIndex = indexPath.row;
    [self.navigationController pushViewController:self.detailViewController animated:YES];
  }
}

@end
