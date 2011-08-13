#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

namespace storage { class Storage; }

/// Responsible for all settings dialogs
@interface SettingsManager : NSObject
{
  storage::Storage * m_storage;
}

// TODO: Refactor SettingsManager.Show: remove storage.
- (void) Show: (UIViewController *)prevController WithStorage: (storage::Storage *)storage;
- (void) Hide;

@end
