#import "SettingsManager.h"
#import "CountriesViewController.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"

#include "../../../storage/storage.hpp"

#include <boost/bind.hpp>

using namespace storage;

// Settings are always present globally
@implementation SettingsManager

- (void) dealloc
{
  [super dealloc];
}

/// Get right controller from the stack
- (UIViewController *) ControllerByIndex:(TIndex const &)index
{
  NSArray * controllers = [[MapsAppDelegate theApp].m_navigationController viewControllers];
  if (index.m_region != TIndex::INVALID && [controllers count] >= 4)
    return [controllers objectAtIndex:3];
  else if (index.m_country != TIndex::INVALID && [controllers count] >= 3)
    return [controllers objectAtIndex:2];
  else if (index.m_group != TIndex::INVALID && [controllers count] >= 2)
    return [controllers objectAtIndex:1];
  return nil;
}

- (void) OnCountryChange: (TIndex const &)index
{
  UIViewController * controller = [self ControllerByIndex:index];
  if (controller && [controller respondsToSelector:@selector(OnCountryChange:)])
    [(CountriesViewController *)controller OnCountryChange: index];
}

- (void) OnCountryDownload: (TIndex const &) index withProgress: (HttpProgressT const &) progress
{
  UIViewController * controller = [self ControllerByIndex:index];
  if (controller && [controller respondsToSelector:@selector(OnDownload:withProgress:)])
    [(CountriesViewController *)controller OnDownload: index withProgress: progress];
}

- (void) OnUpdateCheck: (TUpdateResult) result withText: (string const &) text
{
}

// Currently displays only countries to download
- (void) Show:(UIViewController *)prevController WithStorage:(Storage *)storage
{
  m_storage = storage;
  CountriesViewController * countriesController = [[[CountriesViewController alloc]
      initWithStorage:*m_storage andIndex:TIndex() andHeader:@"Download"] autorelease];
  [[MapsAppDelegate theApp].m_navigationController pushViewController:countriesController animated:YES];

  // Subscribe to storage callbacks.
  {
    // tricky boost::bind for objC class methods
		typedef void (*TChangeFunc)(id, SEL, TIndex const &);
		SEL changeSel = @selector(OnCountryChange:);
		TChangeFunc changeImpl = (TChangeFunc)[self methodForSelector:changeSel];

		typedef void (*TProgressFunc)(id, SEL, TIndex const &, HttpProgressT const &);
		SEL progressSel = @selector(OnCountryDownload:withProgress:);
		TProgressFunc progressImpl = (TProgressFunc)[self methodForSelector:progressSel];

		typedef void (*TUpdateFunc)(id, SEL, bool, string const &);
		SEL updateSel = @selector(OnUpdateCheck:);
		TUpdateFunc updateImpl = (TUpdateFunc)[self methodForSelector:updateSel];

		m_storage->Subscribe(boost::bind(changeImpl, self, changeSel, _1),
    		boost::bind(progressImpl, self, progressSel, _1, _2),
        boost::bind(updateImpl, self, updateSel, _1, _2));
  }
}

// Hides all opened settings windows
- (void) Hide
{
  m_storage->Unsubscribe();

  [[MapsAppDelegate theApp].m_navigationController popToRootViewControllerAnimated:YES];

  m_storage = nil;
}

@end
