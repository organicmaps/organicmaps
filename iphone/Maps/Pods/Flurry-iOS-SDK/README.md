# Flurry SDK

[![pod](https://img.shields.io/cocoapods/v/Flurry-iOS-SDK)](https://cocoapods.org/pods/Flurry-iOS-SDK)
[![platform](https://img.shields.io/cocoapods/p/Flurry-iOS-SDK)](https://cocoapods.org/pods/Flurry-iOS-SDK)
[![license](https://img.shields.io/github/license/flurry/flurry-ios-sdk)](https://github.com/flurry/Flurry-iOS-SDK)

## Table of Contents

- [Installation](#installation)
  - [iOS](#ios)
  - [watchOS](#watchos)
  - [tvOS](#tvos)
- [Requirements](#requirements)
- [Examples](#examples)
- [Suppport](#support)
- [License](#license)

## Installation

To install FlurrySDK from CocoaPods, please follow the instructions below. Remember to include `use_frameworks!` if your app target is in Swift.

### iOS

To enable Flurry Analytics:

```ruby
pod 'Flurry-iOS-SDK/FlurrySDK'
```

To enable Flurry Ad serving: 

```ruby
pod 'Flurry-iOS-SDK/FlurrySDK'
pod 'Flurry-iOS-SDK/FlurryAds'
```

To enable Flurry Config:

```ruby
pod 'Flurry-iOS-SDK/FlurryConfig'
```

To enable Flurry Messaging:

```ruby
pod 'Flurry-iOS-SDK/FlurryMessaging'
```

### watchOS

To use FlurrySDK for Apple Watch 1.x Extension:   

```ruby
target 'Your Apple Watch 1.x Extension Target' do 
  pod 'Flurry-iOS-SDK/FlurryWatchSDK'
end   
```

To use FlurrySDK for Apple Watch 2.x Extension:    

```ruby
target 'Your Apple Watch 2.x Extension Target' do 
  platform :watchos, '2.0'
  pod 'Flurry-iOS-SDK/FlurrySDK'
end   
```

### tvOS

To use FlurrySDK for tvOS apps:

```ruby
target 'Your tvOS Application' do
  platform :tvos, '9.0'
  pod 'Flurry-iOS-SDK/FlurrySDK'
end
```

To enable Flurry Messaging for tvOS:

```ruby
pod 'Flurry-iOS-SDK/FlurryMessaging'
```

## Requirements

* iOS 8.0+
* tvOS 9.0+
* watchOS 1.0+

## Examples

Listed below are brief examples of initializing and using Flurry in your project. For detailed instructions, please [check our documentation](https://developer.yahoo.com/flurry/docs/).

### Initialize Flurry

* iOS/tvOS

  To initialize Flurry, please import Flurry in your Application Delegate and start a Flurry session inside `applicationDidFinishLaunching`, as described below.

  ```swift
  func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
      let sessionBuilder = FlurrySessionBuilder()
          .withLogLevel(FlurryLogLevelAll)
          .withCrashReporting(true)
          .withAppVersion("1.0")
          .withIAPReportingEnabled(true)
      Flurry.startSession("Your API Key", with: sessionBuilder)
      return true
  }
  ```

* watchOS

  Please follow [our instructions here](https://developer.yahoo.com/flurry/docs/integrateflurry/watchos/).

### Log Events

Use this to log normal events and timed events in your app.

* iOS/tvOS

  ```swift
  // Normal events
  Flurry.logEvent("Event", withParameters: ["Key": "Value"])

  // Timed events
  Flurry.logEvent("Event", withParameters: ["Key": "Value"], timed: true)
  Flurry.endTimedEvent("Event", withParameters: ["Key": "Value"])
  ```

* watchOS

  ```swift
  FlurryWatch.logWatchEvent("Event", withParameters: ["Key": "Value"])
  ```

### Log Error (iOS/tvOS)

Use this to log exceptions and/or errors that occur in your app. Flurry will report the first 10 errors that occur in each session.

```swift
Flurry.logError("ERROR_NAME", message: "ERROR_MESSAGE", exception: e)
```

### Track User Demographics (iOS/tvOS)

After identifying the user, use this to log the user’s assigned ID, username, age and gender in your system.

```swift
Flurry.setUserID("USER_ID")
Flurry.setAge(20)
Flurry.setGender("f") // "f" for female and "m" for male
```

### Session Origins and Attributes (iOS/tvOS)

This allows you to specify session origin and deep link attached to each session, or add a custom parameterized session parameters. You can also add an SDK origin specified by origin name and origin version.

```swift
Flurry.addSessionOrigin("SESSION_ORIGIN")
Flurry.addSessionOrigin("SESSION_ORIGIN", withDeepLink: "DEEPLINK")
Flurry.sessionProperties(["key": "value"])
Flurry.addOrigin("ORIGIN_NAME", withVersion: "ORIGIN_VERSION")
Flurry.addOrigin("ORIGIN_NAME", withVersion: "ORIGIN_VERSION", withParameters: ["key": "value"])
```

## Support

* [Flurry Documentation](https://developer.yahoo.com/flurry/docs/)
* [FAQs for Flurry](https://developer.yahoo.com/flurry/docs/faq/)
* [Flurry Support](https://developer.yahoo.com/support/flurry/)

## License

Copyright 2019 Flurry by Verizon Media

This project is licensed under the terms of the [Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0) open source license. Please refer to [LICENSE](LICENSE) for the full terms. Your use of Flurry is governed by [Flurry Terms of Service](https://developer.yahoo.com/flurry/legal-privacy/terms-service/).
