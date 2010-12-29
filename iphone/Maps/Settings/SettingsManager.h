#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

namespace storage { class Storage; }

/// Responsible for all settings dialogs
@interface SettingsManager : NSObject
{
}

+ (void) Show: (UIViewController *)parentController WithStorage: (storage::Storage &)storage;
+ (void) Hide;

@end
