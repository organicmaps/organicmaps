#import "SettingsManager.h"
#import "CountriesViewController.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"

#include "../../../storage/storage.hpp"

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

- (void) OnCountryDownload: (TIndex const &) index withProgress: (HttpProgressT const &) progress
{
  UIViewController * controller = [self ControllerByIndex:index];
  if (controller && [controller respondsToSelector:@selector(OnDownload:withProgress:)])
    [(CountriesViewController *)controller OnDownload: index withProgress: progress];
}

// Currently displays only countries to download
- (void) Show:(UIViewController *)prevController WithStorage:(Storage *)storage
{
  m_storage = storage;

  CountriesViewController * countriesController = [[[CountriesViewController alloc]
      initWithStorage:*m_storage andIndex:TIndex() andHeader:@"Download"] autorelease];
  m_navigationController = [[UINavigationController alloc] initWithRootViewController:countriesController];
  [prevController presentModalViewController:m_navigationController animated:YES];

  // Subscribe to storage callbacks.
  {
    // tricky boost::bind for objC class methods
		typedef void (*TChangeFunc)(id, SEL, TIndex const &);
		SEL changeSel = @selector(OnCountryChange:);
		TChangeFunc changeImpl = (TChangeFunc)[self methodForSelector:changeSel];

		typedef void (*TProgressFunc)(id, SEL, TIndex const &, HttpProgressT const &);
		SEL progressSel = @selector(OnCountryDownload:withProgress:);
		TProgressFunc progressImpl = (TProgressFunc)[self methodForSelector:progressSel];

		m_storage->Subscribe(bind(changeImpl, self, changeSel, _1), bind(progressImpl, self, progressSel, _1, _2));
  }
}

// Hides all opened settings windows
- (void) Hide
{
  m_storage->Unsubscribe();

  [m_navigationController dismissModalViewControllerAnimated:YES];
  [m_navigationController release], m_navigationController = nil;

  m_storage = nil;
}

@end
