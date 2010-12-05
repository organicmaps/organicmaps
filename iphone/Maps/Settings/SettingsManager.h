#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

namespace mapinfo { class Storage; }

/// Responsible for all settings dialogs
@interface SettingsManager : NSObject
{
}

+ (void) Show: (UIViewController *)parentController WithStorage: (mapinfo::Storage &)storage;
+ (void) Hide;

@end
