#import "SettingsManager.h"
#import "CountriesViewController.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"

#include "Framework.h"

#include "../../../std/bind.hpp"

using namespace storage;

// Settings are always present globally
@implementation SettingsManager

/// Get right controller from the stack
- (UIViewController *)ControllerByIndex:(TIndex const &)index
{
  NSArray * controllers = m_navigationController.viewControllers;
  NSInteger count = [controllers count] - 1;
  if (index.m_region != TIndex::INVALID && count >= 3)
    return controllers[3];
  else if (index.m_country != TIndex::INVALID && count >= 2)
    return controllers[2];
  else if (index.m_group != TIndex::INVALID && count >= 1)
    return controllers[1];
  return nil;
}

- (void)OnCountryChange:(TIndex const &)index
{
  UIViewController * controller = [self ControllerByIndex:index];
  if (controller && [controller respondsToSelector:@selector(OnCountryChange:)])
    [(CountriesViewController *)controller OnCountryChange: index];
}

- (void)OnCountryDownload:(TIndex const &)index withProgress:(pair<int64_t, int64_t> const &)progress
{
  UIViewController * controller = [self ControllerByIndex:index];
  if (controller && [controller respondsToSelector:@selector(OnDownload:withProgress:)])
    [(CountriesViewController *)controller OnDownload:index withProgress:progress];
}

- (void)show:(UIViewController *)prevController
{
  NSString * header = NSLocalizedString(@"download_maps", @"Settings/Downloader - Main downloader window title");
  CountriesViewController * countriesController = [[CountriesViewController alloc] initWithIndex:TIndex() andHeader:header];

//  m_navigationController = [[UINavigationController alloc] initWithRootViewController:countriesController];
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
  m_navigationController = prevController.navigationController;
  [m_navigationController pushViewController:countriesController animated:YES];

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
- (void)hide
{
  GetFramework().Storage().Unsubscribe(m_slotID);
  [m_navigationController popToRootViewControllerAnimated:YES];
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
