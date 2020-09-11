# MoPub iOS SDK

Thanks for taking a look at MoPub! We take pride in having an easy-to-use, flexible monetization solution that works across multiple platforms.

Sign up for an account at [http://app.mopub.com/](http://app.mopub.com/).

## Need Help?

You can find integration documentation on our [developer help site](https://developers.mopub.com/publishers/ios/get-started/). Additional documentation can be found [here](https://www.mopub.com/resources/docs).

To file an issue with our team, email [support@mopub.com](mailto:support@mopub.com).

## New Pull Requests?

Thank you for submitting pull requests to the MoPub iOS GitHub repository. Our team regularly monitors and investigates all submissions for inclusion in our official SDK releases. Please note that MoPub does not directly merge these pull requests at this time. Please reach out to your account team or [support@mopub.com](mailto:support@mopub.com) if you have further questions.

## Disclosure
MoPub SDK 4.16 and above integrates technology from our partners Integral Ad Science, Inc. (“IAS”) and Moat, Inc. (“Moat”) in order to support viewability measurement and other proprietary reporting that [IAS](https://integralads.com/capabilities/viewability/) and [Moat](https://moat.com/analytics) provide to their advertiser and publisher clients. You have the option to remove or disable this technology by following the opt-out instructions [below](#disableViewability).

If you do not remove or disable IAS's and/or Moat’s technology in accordance with these instructions, you agree that IAS's [privacy policy](https://integralads.com/privacy-policy/) and [license](https://integralads.com/sdk-license-agreement) and Moat’s [privacy policy](https://moat.com/privacy),  [terms](https://moat.com/terms), and [license](https://moat.com/sdklicense.txt), respectively, apply to your integration of these partners' technologies into your application.

## Installation

The MoPub SDK supports multiple methods for installing the library in a project.

The current version of the SDK is 5.12.0

### Installation with CocoaPods

[CocoaPods](https://cocoapods.org/) is a dependency manager for Swift and Objective-C Cocoa projects, which automates and simplifies the process of using 3rd-party libraries like the MoPub SDK in your projects. You can install it with the following command:

```
$ gem install cocoapods
```

**Podfile**
To integrate MoPub SDK into your Xcode project using CocoaPods, specify it in your Podfile:

```
source 'https://github.com/CocoaPods/Specs.git'
platform :ios, '9.0'
use_frameworks!

target 'MyApp' do
  pod 'mopub-ios-sdk', '~> 5.9'
end
```

Then, run the following command:

```
$ pod install
```

### Manual Integration with Dynamic Framework

MoPub provides a prepackaged archive of the dynamic framework:

- **[MoPub SDK Framework.zip](https://github.com/mopub/mopub-ios-sdk/releases/download/5.12.0/mopub-framework-5.12.0.zip)**

  Includes everything you need to serve HTML, MRAID, and Native MoPub advertisements.  Third party ad networks are not included.

Add the dynamic framework to the target's Embedded Binaries section of the General tab.

### Manual Integration with Source Code

MoPub provides two prepackaged archives of source code:

- **[MoPub Base SDK.zip](https://github.com/mopub/mopub-ios-sdk/releases/download/5.12.0/mopub-base-5.12.0.zip)**

  Includes everything you need to serve HTML, MRAID, and Native MoPub advertisements.  Third party ad networks are not included.

- **[MoPub Base SDK Excluding Native.zip](https://github.com/mopub/mopub-ios-sdk/releases/download/5.12.0/mopub-nonnative-5.12.0.zip)**

  Includes everything you need to serve HTML and MRAID advertisements.  Third party ad networks and Native MoPub advertisements are not included.

## Integrate

Integration instructions are available on the [wiki](https://github.com/mopub/mopub-ios-sdk/wiki/Getting-Started).

## New in this Version

Please view the [changelog](https://github.com/mopub/mopub-ios-sdk/blob/master/CHANGELOG.md) for details.

- **Features**
  - Location setters for all formats are marked deprecated and will be removed in a future release.
  - Added Mintegral as a supported network.
  - A new field `appVersion` has been added to `MPImpressionData`.
  - Update the Sample app to 64-bit architectures only.
  - Added ad load history to the Saved Ads section in the Sample app.

- **Bug Fixes**
  - Fixed potential multithreading crash in `MPAdServerURLBuilder`.

See the [Getting Started Guide](https://github.com/mopub/mopub-ios-sdk/wiki/Getting-Started#app-transport-security-settings) for instructions on setting up ATS in your app.

## Upgrading to SDK 5.0

Please see the [Getting Started Guide](https://developers.mopub.com/docs/ios/getting-started/) for instructions on upgrading from SDK 4.X to SDK 5.0.

For GDPR-specific upgrading instructions, also see the [GDPR Integration Guide](https://developers.mopub.com/docs/publisher/gdpr).

### <a name="disableViewability"></a>Disabling Viewability Measurement
There are a few options for opting out of viewability measurement:
##### Opting Out in a Manual Integration
Before dragging the MoPubSDK folder into your Xcode project, simply delete the “Moat” folder to opt out of Moat or the “Avid” folder to opt out of IAS in MoPubSDK/Viewability/. If you would like to opt out of both, delete both folders.
##### Opting Out in a CocoaPods Integration
Including `pod 'mopub-ios-sdk'` in your Podfile will include both IAS and Moat SDKs, as well as the MoPub SDK. In order to opt out:
- `pod 'mopub-ios-sdk/Avid'` will include the IAS SDK, but not the Moat SDK, as well as the MoPub SDK.
- `pod 'mopub-ios-sdk/Moat'` will include the Moat SDK, but not the IAS SDK, as well as the MoPub SDK.
- `pod 'mopub-ios-sdk/Core'` will only include the MoPub SDK, with viewability measurement totally disabled.

Make sure to run `pod update` once your Podfile is set up to your preferences.
##### Software Disable
If you would like to opt out of viewability measurement but do not want to modify the MoPub SDK, a function is provided for your convenience. As soon as possible after calling `- (void)initializeSdkWithConfiguration:completion:`, call `[[MoPub sharedInstance] disableViewability:(vendors)]`. In place of “(vendors)”, `MPViewabilityOptionIAS` will disable IAS but leave Moat enabled, `MPViewabilityOptionMoat` will disable Moat but leave IAS enabled, and `MPViewabilityOptionAll` will disable all viewability measurement.

## Requirements

- iOS 9.0 and up
- Xcode 11.0 and up

## License

We have launched a new license as of version 3.2.0. To view the full license, visit [http://www.mopub.com/legal/sdk-license-agreement/](http://www.mopub.com/legal/sdk-license-agreement/)
