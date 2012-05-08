#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

/// Responsible for all settings dialogs
@interface SettingsManager : NSObject <UIActionSheetDelegate>
{
  UINavigationController * m_navigationController;
}

- (void) show:(UIViewController *)prevController;
- (void) hide;

@end
