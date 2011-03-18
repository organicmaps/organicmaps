#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

namespace storage { class Storage; }

@class CountriesViewController;

/// Responsible for all settings dialogs
@interface SettingsManager : NSObject
{
  CountriesViewController * m_countriesController;
  UINavigationController * m_navController;
  UIViewController * m_prevController;
  storage::Storage * m_storage;
}

// TODO: Refactor SettingsManager.Show: remove storage.
- (void) Show: (UIViewController *)prevController WithStorage: (storage::Storage *)storage;
- (void) Hide;

@end
