#import "SettingsManager.h"
#import "CountriesViewController.h"

#include "../../../storage/storage.hpp"

using namespace storage;

static void OnCountryChange(TIndex const & index)
{

}

static void OnCountryDownloadProgress(TIndex const & index, TDownloadProgress const & progress)
{

}

static void OnUpdateCheck(int64_t size, char const * readme)
{

}

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

// Currently displays only countries to download
+ (void) Show: (UIViewController *)parentController WithStorage: (Storage &)storage
{
	g_storage = &storage;
	if (!g_navController)
  {
  	CountriesViewController * rootViewController = [[CountriesViewController alloc] initWithStorage:storage
    		andIndex:TIndex() andHeader:@"Download"];
  	g_navController = [[UINavigationController alloc] initWithRootViewController:rootViewController];
    
  	storage.Subscribe(&OnCountryChange, &OnCountryDownloadProgress, &OnUpdateCheck);
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
  	g_navController = nil;
  }
}

@end
