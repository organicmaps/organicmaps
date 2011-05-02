#import "SettingsManager.h"
#import "CountriesViewController.h"
#import "MapViewController.h"

#include "../../../storage/storage.hpp"

#include <boost/bind.hpp>

using namespace storage;

// Settings are always present globally
@implementation SettingsManager

// Destructor
- (void) dealloc
{
  [m_prevController release];
  m_prevController = nil;
  [m_navController release];
  m_navController = nil;
  [super dealloc];
}

/// Get right controller from the stack
- (UIViewController *) ControllerByIndex:(TIndex const &)index
{
  NSArray * controllers = [m_navController viewControllers];
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
	if (m_navController)
  {
    UIViewController * controller = [self ControllerByIndex:index];
    if (controller && [controller respondsToSelector:@selector(OnCountryChange:)])
      [(CountriesViewController *)controller OnCountryChange: index];
  }
}

- (void) OnCountryDownload: (TIndex const &) index withProgress: (HttpProgressT const &) progress
{
	if (m_navController)
  {
    UIViewController * controller = [self ControllerByIndex:index];
    if (controller && [controller respondsToSelector:@selector(OnDownload:withProgress:)])
      [(CountriesViewController *)controller OnDownload: index withProgress: progress];
  }
}

- (void) OnUpdateCheck: (TUpdateResult) result withText: (string const &) text
{
}

// Currently displays only countries to download
- (void) Show:(UIViewController *)prevController WithStorage:(Storage *)storage
{
  m_storage = storage;
  m_prevController = [prevController retain];

  if (!m_navController)
  {
    CountriesViewController * countriesController = [[CountriesViewController alloc]
                               initWithStorage:*m_storage andIndex:TIndex() andHeader:@"Download"];
    m_navController = [[UINavigationController alloc]
                       initWithRootViewController:countriesController];
    [countriesController release];
  }

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

  // Transition views.
  [m_prevController presentModalViewController:m_navController animated:YES];
  // This has bugs when device orientation is changed.
  // [UIView transitionFromView:m_prevController.view
  //                     toView:m_navController.view
  //                   duration:1
  //                    options:UIViewAnimationOptionTransitionCurlUp
  //                 completion:nil];
}

// Hides all opened settings windows
- (void) Hide
{
  if (!m_prevController)
    return;

  m_storage->Unsubscribe();

  // Transition views.
  [m_prevController dismissModalViewControllerAnimated:YES];
  // This has bugs when device orientation is changed.
  // [UIView transitionFromView:m_navController.view
  //                     toView:m_prevController.view
  //                   duration:1
  //                    options:UIViewAnimationOptionTransitionCurlDown
  //                 completion:nil];

  m_storage = nil;
  [m_prevController release];
  m_prevController = nil;
  [m_navController release];
  m_navController = nil;

}

@end
