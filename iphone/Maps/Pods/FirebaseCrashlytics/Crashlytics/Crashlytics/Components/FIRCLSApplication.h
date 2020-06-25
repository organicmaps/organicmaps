// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#import <Foundation/Foundation.h>
#if CLS_TARGET_OS_HAS_UIKIT
#import <UIKit/UIKit.h>
#endif

__BEGIN_DECLS

#define FIRCLSApplicationActivityDefault \
  (NSActivitySuddenTerminationDisabled | NSActivityAutomaticTerminationDisabled)

/**
 * Type to indicate application installation source
 */
typedef NS_ENUM(NSInteger, FIRCLSApplicationInstallationSourceType) {
  FIRCLSApplicationInstallationSourceTypeDeveloperInstall = 1,
  // 2 and 3 are reserved for legacy values.
  FIRCLSApplicationInstallationSourceTypeAppStore = 4
};

/**
 * Returns the application bundle identifier with occurences of "/" replaced by "_"
 */
NSString* FIRCLSApplicationGetBundleIdentifier(void);

/**
 * Returns the SDK's bundle ID
 */
NSString* FIRCLSApplicationGetSDKBundleID(void);

/**
 * Returns the platform identifier, either: ios, mac, or tvos.
 * Catalyst apps are treated as mac.
 */
NSString* FIRCLSApplicationGetPlatform(void);

/**
 * Returns the user-facing app name
 */
NSString* FIRCLSApplicationGetName(void);

/**
 * Returns the build number
 */
NSString* FIRCLSApplicationGetBundleVersion(void);

/**
 * Returns the human-readable build version
 */
NSString* FIRCLSApplicationGetShortBundleVersion(void);

/**
 * Returns a number to indicate how the app has been installed: Developer / App Store
 */
FIRCLSApplicationInstallationSourceType FIRCLSApplicationInstallationSource(void);

BOOL FIRCLSApplicationIsExtension(void);
NSString* FIRCLSApplicationExtensionPointIdentifier(void);

#if CLS_TARGET_OS_HAS_UIKIT
UIApplication* FIRCLSApplicationSharedInstance(void);
#else
id FIRCLSApplicationSharedInstance(void);
#endif

void FIRCLSApplicationOpenURL(NSURL* url,
                              NSExtensionContext* extensionContext,
                              void (^completionBlock)(BOOL success));

id<NSObject> FIRCLSApplicationBeginActivity(NSActivityOptions options, NSString* reason);
void FIRCLSApplicationEndActivity(id<NSObject> activity);

void FIRCLSApplicationActivity(NSActivityOptions options, NSString* reason, void (^block)(void));

__END_DECLS
