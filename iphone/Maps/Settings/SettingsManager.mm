#import "SettingsManager.h"
#import "CountriesViewController.h"

#include "../../../storage/storage.hpp"

#include <boost/bind.hpp>

using namespace storage;

// Settings are always present globally
@implementation SettingsManager

	storage::Storage * g_storage = 0;
  UINavigationController * g_navController = nil;

// Destructor
- (void) dealloc
{
	if (g_navController)
  	[g_navController release];
  [super dealloc];
}

// Acts as Close dialog button
- (void) OnMapButtonClick: (id)selector
{
	[SettingsManager Hide];
}


+ (void) OnCountryChange: (TIndex const &) index
{
	if (g_navController)
  	[(CountriesViewController *)g_navController.topViewController OnCountryChange: index];
}

+ (void) OnCountryDownload: (TIndex const &) index withProgress: (TDownloadProgress const &) progress
{
	if (g_navController)
  	[(CountriesViewController *)g_navController.topViewController OnDownload: index withProgress: progress];
}

+ (void) OnUpdateCheck: (TUpdateResult) result withText: (string const &) text
{

}


// Currently displays only countries to download
+ (void) Show: (UIViewController *)parentController WithStorage: (Storage &)storage
{
	g_storage = &storage;
	if (!g_navController)
  {
  	CountriesViewController * rootViewController = [[CountriesViewController alloc] initWithStorage:storage
    		andIndex:TIndex() andHeader:@"Download"];
  	g_navController = [[UINavigationController alloc] initWithRootViewController:rootViewController];
    
    // tricky boost::bind for objC class methods
		typedef void (*TChangeFunc)(id, SEL, TIndex const &);
		SEL changeSel = @selector(OnCountryChange:);
		TChangeFunc changeImpl = (TChangeFunc)[self methodForSelector:changeSel];

		typedef void (*TProgressFunc)(id, SEL, TIndex const &, TDownloadProgress const &);
		SEL progressSel = @selector(OnCountryDownload:withProgress:);
		TProgressFunc progressImpl = (TProgressFunc)[self methodForSelector:progressSel];

		typedef void (*TUpdateFunc)(id, SEL, bool, string const &);
		SEL updateSel = @selector(OnUpdateCheck:);
		TUpdateFunc updateImpl = (TUpdateFunc)[self methodForSelector:updateSel];

		storage.Subscribe(boost::bind(changeImpl, self, changeSel, _1),
    		boost::bind(progressImpl, self, progressSel, _1, _2),
        boost::bind(updateImpl, self, updateSel, _1, _2));
  }

  [parentController presentModalViewController:g_navController animated:YES];
}

// Hides all opened settings windows
+ (void) Hide
{
	if (g_navController)
  {
    g_storage->Unsubscribe();
		[[g_navController parentViewController] dismissModalViewControllerAnimated:YES];
  	[g_navController release];
    g_navController = nil;
  }
}

@end
