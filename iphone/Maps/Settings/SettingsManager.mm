#import "SettingsManager.h"
#import "CountriesViewController.h"

#include "../../../storage/storage.hpp"

using namespace storage;

// Settings are always present globally
//static SettingsManager gInstance;

@implementation SettingsManager

	CountriesViewController * m_countriesViewController = nil;

// Destructor
- (void) dealloc
{
	if (m_countriesViewController)
  	[m_countriesViewController release];
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
	if (!m_countriesViewController)
  	m_countriesViewController = [[CountriesViewController alloc] initWithStorage:storage];

  [parentController presentModalViewController:m_countriesViewController animated:YES];
}

// Hides all opened settings windows
+ (void) Hide
{
	if (m_countriesViewController)
  {
		[[m_countriesViewController parentViewController] dismissModalViewControllerAnimated:YES];
  	m_countriesViewController = nil;
  }
}

@end
