#import "SettingsManager.h"
#import "CountriesViewController.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"

#include "Framework.h"

#include "../../../std/bind.hpp"

using namespace storage;

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

- (void) show:(UIViewController *)prevController
{
  CountriesViewController * countriesController = [[[CountriesViewController alloc]
      initWithIndex:TIndex() andHeader:NSLocalizedString(@"download_maps", @"Settings/Downloader - Main downloader window title")] autorelease];

  Framework & f = GetFramework();
  // Subscribe to storage callbacks AND load country names after calling Storage::Subscribe()
  {
    // tricky boost::bind for objC class methods
		typedef void (*TChangeFunc)(id, SEL, TIndex const &);
		SEL changeSel = @selector(OnCountryChange:);
		TChangeFunc changeImpl = (TChangeFunc)[self methodForSelector:changeSel];

		typedef void (*TProgressFunc)(id, SEL, TIndex const &, pair<int64_t, int64_t> const &);
		SEL progressSel = @selector(OnCountryDownload:withProgress:);
		TProgressFunc progressImpl = (TProgressFunc)[self methodForSelector:progressSel];

		m_slotID = f.Storage().Subscribe(bind(changeImpl, self, changeSel, _1),
                                     bind(progressImpl, self, progressSel, _1, _2));
  }
  // display controller only when countries are loaded
  [prevController.navigationController pushViewController:countriesController animated:YES];

  // We do force delete of old maps at startup from this moment.
  /*
  // display upgrade/delete old maps dialog if necessary
  if (f.NeedToDeleteOldMaps())
  {
    UIActionSheet * dialog = [[UIActionSheet alloc]
        initWithTitle:NSLocalizedString(@"new_map_data_format_upgrade_dialog", @"Downloader/Upgrade dialog title")
        delegate:self
        cancelButtonTitle:NSLocalizedString(@"cancel", @"Downloader/Upgrade Cancel button")
        destructiveButtonTitle:NSLocalizedString(@"delete_old_maps", @"Downloader/Upgrade OK button")
        otherButtonTitles:nil];
    [dialog showInView:m_navigationController.view];
    [dialog release];
  }
  */
}

// Hides all opened settings windows
- (void) hide
{
  GetFramework().Storage().Unsubscribe(m_slotID);

  [m_navigationController popToRootViewControllerAnimated:YES];
  [m_navigationController release], m_navigationController = nil;
}

/*
// Called from Upgrade/Delete old maps dialog
- (void) actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex == 0)
  { // delete old maps and show downloader
    GetFramework().DeleteOldMaps();
  }
  else
  { // User don't want to upgrade at the moment - so be it.
    [self hide];
  }
}
*/
@end
