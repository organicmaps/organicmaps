#import <Foundation/NSObject.h>

/// Synchronizes device's Settings/App Preferences panel with
/// internal settings.ini file, also initializes app preferences
/// if Settings panel was not opened
@interface Preferences : NSObject

+ (void)setupByPreferences;

@end
