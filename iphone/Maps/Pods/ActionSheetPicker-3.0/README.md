[![Version](http://img.shields.io/cocoapods/v/ActionSheetPicker-3.0.svg)](http://cocoadocs.org/docsets/ActionSheetPicker-3.0)
[![Carthage compatible](https://img.shields.io/badge/Carthage-compatible-4BC51D.svg?style=flat)](https://github.com/Carthage/Carthage)
[![Build Status](https://travis-ci.org/skywinder/ActionSheetPicker-3.0.svg?branch=master)](https://travis-ci.org/skywinder/ActionSheetPicker-3.0)
[![Issues](http://img.shields.io/github/issues/skywinder/ActionSheetPicker-3.0.svg)](https://github.com/skywinder/ActionSheetPicker-3.0/issues?state=open)
[![License](https://img.shields.io/cocoapods/l/ActionSheetPicker-3.0.svg)](http://cocoadocs.org/docsets/ActionSheetPicker-3.0)
[![Platform](https://img.shields.io/cocoapods/p/ActionSheetPicker-3.0.svg)](http://cocoadocs.org/docsets/ActionSheetPicker-3.0)

ActionSheetPicker-3.0
==================

- [Overview](#overview)
	- [Benefits](#benefits)
- [QuickStart](#quickstart)
	- [Basic Usage](#basic-usage)
- [Installation](#installation)
- [Example Projects](#example-projects)
- [Screenshots](#screenshots)
- [Apps using this library](#apps-using-this-library)
- [Maintainer and Contributor](#maintainer-and-contributor)
- [Credits](#credits)
- [Contributing](#contributing)

Please welcome: **ActionSheetPicker-3.0**!

`pod 'ActionSheetPicker-3.0', '~> 2.3.0'` (**iOS 5.1.1-9.x** compatible!)

##ActionSheetPicker = UIPickerView + UIActionSheet ##

![Animation](Screenshots/example.gif)

Well, that's how it started. Now, the following is more accurate:

 * _**iPhone/iPod** ActionSheetPicker = ActionSheetPicker = A Picker + UIActionSheet_
 * _**iPad** ActionSheetPicker = A Picker + UIPopoverController_


## Overview ##
Easily present an ActionSheet with a PickerView, allowing user to select from a number of immutable options. 

### Benefits ##

 * Spawn pickers with convenience function - delegate or reference
   not required. Just provide a target/action callback.
 * Add buttons to UIToolbar for quick selection (see ActionSheetDatePicker below)
 * Delegate protocol available for more control
 * Universal (iPhone/iPod/iPad)

## QuickStart

There are 4 distinct picker view options: `ActionSheetStringPicker`, `ActionSheetDistancePicker`, `ActionSheetDatePicker`, and `ActionSheetCustomPicker`. We'll focus here on how to use the `ActionSheetStringPicker` since it's most likely the one you want to use.

### Basic Usage ##

**For detailed info about customisations, please look  [BASIC USAGE](https://github.com/skywinder/ActionSheetPicker-3.0/blob/master/BASIC-USAGE.md)**

- Custom buttons view
- Custom buttons callbacks
- Action by clicking outside of the picker
- Background color and blur effect
- Other customisations

**For detailed examples, please check [Example Projects](#example-projects) in this repo.**

#### `Swift:`

```swift
 ActionSheetMultipleStringPicker.show(withTitle: "Multiple String Picker", rows: [
            ["One", "Two", "A lot"],
            ["Many", "Many more", "Infinite"]
            ], initialSelection: [2, 2], doneBlock: {
                picker, indexes, values in
                
                print("values = \(values)")
                print("indexes = \(indexes)")
                print("picker = \(picker)")
                return
        }, cancel: { ActionMultipleStringCancelBlock in return }, origin: sender)
```

#### `Objective-C:`

```obj-c
// Inside a IBAction method:

// Create an array of strings you want to show in the picker:
NSArray *colors = [NSArray arrayWithObjects:@"Red", @"Green", @"Blue", @"Orange", nil];

[ActionSheetStringPicker showPickerWithTitle:@"Select a Color"
                                        rows:colors
                            initialSelection:0
                                   doneBlock:^(ActionSheetStringPicker *picker, NSInteger selectedIndex, id selectedValue) {
                                      NSLog(@"Picker: %@, Index: %@, value: %@", 
                                      picker, selectedIndex, selectedValue);
                                    }
                                 cancelBlock:^(ActionSheetStringPicker *picker) {
                                      NSLog(@"Block Picker Canceled");
                                    }
                                      origin:sender];
// You can also use self.view if you don't have a sender
```

 
 
##Installation##

### CocoaPods

[CocoaPods](http://cocoapods.org) is a dependency manager for Cocoa projects.

You can install it with the following command:

```bash
$ gem install cocoapods
```

To integrate ActionSheetPicker-3.0 into your Xcode project using CocoaPods, specify it in your `Podfile`:

```ruby
source 'https://github.com/CocoaPods/Specs.git'
use_frameworks!

pod 'ActionSheetPicker-3.0'
```

Then, run the following command:

```bash
$ pod install
```

### Import to project

To import pod you should add string:

- For `Obj-c` projects:

```obj-c
   #import "ActionSheetPicker.h"
```
- For `Swift` projects:

```swift
  import ActionSheetPicker_3_0
```
### Carthage

Carthage is a decentralized dependency manager that automates the process of adding frameworks to your Cocoa application.

You can install Carthage with [Homebrew](http://brew.sh/) using the following command:

```bash
$ brew update
$ brew install carthage
```

To integrate ActionSheetPicker-3.0 into your Xcode project using Carthage, specify it in your `Cartfile`:

```ogdl
github "skywinder/ActionSheetPicker-3.0"
```

### Manually

If you prefer not to use either of the aforementioned dependency managers, you can integrate ActionSheetPicker-3.0 into your project manually.

The "old school" way is manually add to your project all from [Pickers](/Pickers) folder.

### Embedded Framework

- Add ActionSheetPicker-3.0 as a [submodule](http://git-scm.com/docs/git-submodule) by opening the Terminal, `cd`-ing into your top-level project directory, and entering the following command:

```bash
$ git submodule add https://github.com/skywinder/ActionSheetPicker-3.0.git
```

- Open the `ActionSheetPicker-3.0` folder, and drag `CoreActionSheetPicker.xcodeproj` into the file navigator of your app project.
- In Xcode, navigate to the target configuration window by clicking on the blue project icon, and selecting the application target under the "Targets" heading in the sidebar.
- Ensure that the deployment target of CoreActionSheetPicker.framework matches that of the application target.
- In the tab bar at the top of that window, open the "Build Phases" panel.
- Expand the "Target Dependencies" group, and add `CoreActionSheetPicker.framework`.
- Click on the `+` button at the top left of the panel and select "New Copy Files Phase". Rename this new phase to "Copy Frameworks", set the "Destination" to "Frameworks", and add `CoreActionSheetPicker.framework`.

## Example Projects##

`open ActionSheetPicker-3.0.xcworkspace`

Here is 4 projects:

- **CoreActionSheetPicker** - all picker files combined in one Framework. (available since `iOS 8`)
- **ActionSheetPicker** - modern and descriptive Obj-C project with many examples.
- **Swift-Example** - example, written on Swift. (only with basic 3 Pickers examples, for all examples please run `ActionSheetPicker` project)
- **ActionSheetPicker-iOS6-7** -  iOS 6 and 7 comparable project. or to run only this project `open Example-for-and-6/ActionSheetPicker.xcodeproj`

## Screenshots

![ActionSheetPicker](https://raw.githubusercontent.com/skywinder/ActionSheetPicker-3.0/master/Screenshots/string.png "ActionSheetPicker")
![ActionSheetDatePicker](https://raw.githubusercontent.com/skywinder/ActionSheetPicker-3.0/master/Screenshots/date.png "ActionSheetDatePicker")
![ActionSheetDatePicker](https://raw.githubusercontent.com/Jack-s/ActionSheetPicker-3.0/master/Screenshots/time.png "ActionSheetDatePicker")
![CustomButtons](https://raw.githubusercontent.com/skywinder/ActionSheetPicker-3.0/master/Screenshots/custom.png "CustomButtons")
![iPad Support](https://raw.githubusercontent.com/skywinder/ActionSheetPicker-3.0/master/Screenshots/ipad.png "iPad Support")


## [Apps using this library](https://github.com/skywinder/ActionSheetPicker-3.0/wiki/Apps-using-ActionSheetPicker-3.0) 

If you've used this project in a live app, please let me know! Nothing makes me happier than seeing someone else take my work and go wild with it.

*If you are using `ActionSheetPicker-3.0` in your app or know of an app that uses it, please add it to [this] (https://github.com/skywinder/ActionSheetPicker-3.0/wiki/Apps-using-ActionSheetPicker-3.0) list.*

## Maintainer and Contributor

- [Petr Korolev](http://github.com/skywinder) (update to iOS 7 and iOS 8, implementing new pickers, community support)

## Credits

- ActionSheetPicker was originally created by [Tim Cinel](http://github.com/TimCinel) ([@TimCinel](http://twitter.com/TimCinel)) Since the [Tim's repo](https://github.com/TimCinel/ActionSheetPicker) is not support iOS 7+, I forked from his repo and implement iOS 7-8 support, and also bunch of UI fixes, crash-fixes and different customisation abilities.

- And most of all, thanks to ActionSheetPicker-3.0's [growing list of contributors](https://github.com/skywinder/ActionSheetPicker-3.0/graphs/contributors).

## Contributing

1. Create an issue to discuss about your idea
2. Fork it (https://github.com/skywinder/ActionSheetPicker-3.0/fork)
3. Create your feature branch (`git checkout -b my-new-feature`)
4. Commit your changes (`git commit -am 'Add some feature'`)
5. Push to the branch (`git push origin my-new-feature`)
6. Create a new Pull Request

**Bug reports, feature requests, patches, well-wishes, and rap demo tapes are always welcome.**

[![Analytics](https://ga-beacon.appspot.com/UA-52127948-3/ActionSheetPicker-3.0/readme)](https://ga-beacon.appspot.com/UA-52127948-3/ActionSheetPicker-3.0/readme)
