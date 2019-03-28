<p align="center">
  <img height="75" src="repository_assets/AF_color_medium.png" />
</p>

-----------
[![Version](https://img.shields.io/cocoapods/v/AppsFlyerFramework.svg?style=flat)](http://cocoapods.org/pods/AppsFlyerFramework)

[AppsFlyer](https://www.appsflyer.com/) helps mobile marketers measure and improve their performance through amazing tools, really big data and over 2,000 integrations.



- Supports iOS 8+

Installation
------------

### CocoaPods

Just add `pod 'AppsFlyerFramework'` into your [Podfile](https://guides.cocoapods.org/syntax/podfile.html).

Then run

```zsh
$ pod install
```

Finally, import the framework:

```swift
// Swift
import AppsFlyerLib
```

```objc
// ObjC
#import <AppsFlyerTracker/AppsFlyerTracker.h>
```

### Carthage

Just add the following into your [Cartfile](https://github.com/Carthage/Carthage/blob/master/Documentation/Artifacts.md#cartfile):
```
binary "https://raw.githubusercontent.com/AppsFlyerSDK/AppsFlyerFramework/master/AppsFlyerTracker.json"
```

Then run

```zsh
$ carthage bootstrap
```

**Note:**
Old URI referencing `Carthage.json` is deprecated. If you use it please update your Cartfile to the new one to ease dependency management.



Changelog
------------

You can find the release changelog [here](https://support.appsflyer.com/hc/en-us/articles/115001224823-AppsFlyer-iOS-SDK-Release-Notes).

---

In order for us to provide optimal support, we would kindly ask you to submit any issues to support@appsflyer.com

*When submitting an issue please specify your AppsFlyer sign-up (account) email, your app ID, production steps, logs, code snippets and any additional relevant information.*

----------
