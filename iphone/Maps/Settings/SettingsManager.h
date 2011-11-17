#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

class Framework;

/// Responsible for all settings dialogs
@interface SettingsManager : NSObject <UIActionSheetDelegate>
{
  Framework * m_framework;
  UINavigationController * m_navigationController;
}

// TODO: Refactor SettingsManager.Show: remove storage.
- (void) show:(UIViewController *)prevController withFramework:(Framework *)framework;
- (void) hide;

@end
