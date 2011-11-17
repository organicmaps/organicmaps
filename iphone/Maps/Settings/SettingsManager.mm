#import "SettingsManager.h"
#import "CountriesViewController.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"

#include "../../../map/framework.hpp"

#include "../../../std/bind.hpp"

using namespace storage;

// @TODO advertisement banner
// NSLocalizedString(@"MapsWithMe Pro", @"Banner title")
// NSLocalizedString(@"One step ahead! Cool search feature! Bla bla bla", @"Banner message")
// NSLocalizedString(@"Check MapsWithMe Pro", @"Banner Ok button - go to the appstore for paid version")
// NSLocalizedString(@"Remind me later", @"Banner postpone button - remind later about paid version")
// NSLocalizedString(@"Don't bother me", @"Banner cancel button - never remind user about paid version")

// @TODO Write Review dialog
// NSLocalizedString(@"Leave your review", @"Leave Review dialog title")
// NSLocalizedString(@"If you like MapsWithMe, please support us with your review. If you want to complain - please leave it on our support site", @"Leave Review dialog message")
// NSLocalizedString(@"Write a review", @"Leave Review dialog - Review button")
// NSLocalizedString(@"Complain", @"Leave Review dialog - Complain button (goes to support site)")
// NSLocalizedString(@"Not now", @"Leave Review dialog - Not now button (remond me later)")
// NSLocalizedString(@"Dismiss", @"Leave Review dialog - Dismiss forever button")

// @TODO Buttons in main maps view
// NSLocalizedString(@"Maps", @"View and button titles for accessibility")
// NSLocalizedString(@"Download Maps", @"View and button titles for accessibility")
// NSLocalizedString(@"Search", @"View and button titles for accessibility")
// NSLocalizedString(@"My Position", @"View and button titles for accessibility")
// NSLocalizedString(@"Travel Guide", @"View and button titles for accessibility")
// NSLocalizedString(@"Back", @"View and button titles for accessibility")
// NSLocalizedString(@"Zoom to the country", @"View and button titles for accessibility")

// Settings are always present globally
@implementation SettingsManager

- (void) dealloc
{
  [m_navigationController release];
  [super dealloc];
}


/// Get right controller from the stack
- (UIViewController *) ControllerByIndex:(TIndex const &)index
{
  NSArray * controllers = m_navigationController.viewControllers;
  if (index.m_region != TIndex::INVALID && [controllers count] >= 3)
    return [controllers objectAtIndex:2];
  else if (index.m_country != TIndex::INVALID && [controllers count] >= 2)
    return [controllers objectAtIndex:1];
  else if (index.m_group != TIndex::INVALID && [controllers count] >= 1)
    return [controllers objectAtIndex:0];
  return nil;
}

- (void) OnCountryChange: (TIndex const &)index
{
  UIViewController * controller = [self ControllerByIndex:index];
  if (controller && [controller respondsToSelector:@selector(OnCountryChange:)])
    [(CountriesViewController *)controller OnCountryChange: index];
}

- (void) OnCountryDownload: (TIndex const &) index withProgress: (pair<int64_t, int64_t> const &) progress
{
  UIViewController * controller = [self ControllerByIndex:index];
  if (controller && [controller respondsToSelector:@selector(OnDownload:withProgress:)])
    [(CountriesViewController *)controller OnDownload: index withProgress: progress];
}

- (void) show:(UIViewController *)prevController withFramework:(Framework *)framework
{
  m_framework = framework;

  CountriesViewController * countriesController = [[[CountriesViewController alloc]
      initWithStorage:framework->Storage() andIndex:TIndex() andHeader:NSLocalizedString(@"Download", @"Settings/Downloader - Main downloader window title")] autorelease];
  m_navigationController = [[UINavigationController alloc] initWithRootViewController:countriesController];

  // Subscribe to storage callbacks AND load country names after calling Storage::Subscribe()
  {
    // tricky boost::bind for objC class methods
		typedef void (*TChangeFunc)(id, SEL, TIndex const &);
		SEL changeSel = @selector(OnCountryChange:);
		TChangeFunc changeImpl = (TChangeFunc)[self methodForSelector:changeSel];

		typedef void (*TProgressFunc)(id, SEL, TIndex const &, pair<int64_t, int64_t> const &);
		SEL progressSel = @selector(OnCountryDownload:withProgress:);
		TProgressFunc progressImpl = (TProgressFunc)[self methodForSelector:progressSel];

		framework->Storage().Subscribe(bind(changeImpl, self, changeSel, _1), bind(progressImpl, self, progressSel, _1, _2));
  }
  // display controller only when countries are loaded
  [prevController presentModalViewController:m_navigationController animated:YES];

  // display upgrade/delete old maps dialog if necessary
  if (framework->NeedToDeleteOldMaps())
  {
    UIActionSheet * dialog = [[UIActionSheet alloc]
        initWithTitle:NSLocalizedString(@"MapsWithMe uses new, fresh and more compact maps. For example, USA, Germany, France, Canada and Russia are splitted to smaller States/Regions. But to use new maps you should delete all previously downloaded data and download new maps.", @"Downloader/Upgrade dialog message")
        delegate:self
        cancelButtonTitle:NSLocalizedString(@"Do nothing at the moment", @"Downloader/Upgrade Cancel button")
        destructiveButtonTitle:NSLocalizedString(@"Delete old and download new maps", @"Downloader/Upgrade OK button")
        otherButtonTitles:nil];
    [dialog showInView:m_navigationController.view];
    [dialog release];
  }
}

// Hides all opened settings windows
- (void) hide
{
  m_framework->Storage().Unsubscribe();

  [m_navigationController dismissModalViewControllerAnimated:YES];
  [m_navigationController release], m_navigationController = nil;

  m_framework = nil;
}

// Called from Upgrade/Delete old maps dialog
- (void) actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex == 0)
  { // delete old maps and show downloader
    m_framework->DeleteOldMaps();
  }
  else
  { // User don't want to upgrade at the moment - so be it.
    [self hide];
  }
}

@end
