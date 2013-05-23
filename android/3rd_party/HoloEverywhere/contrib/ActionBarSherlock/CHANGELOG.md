Change Log
===============================================================================

Version 4.3.1 *(2013-04-28)*
----------------------------

 * Fix: Importing library into Eclipse as a Maven module now works correctly.
 * Prevent dispatching touch events to both action bar and content view on API
   11 and up.
 * Add Hebrew and Brazillian translations to i18n module.


Version 4.3.0 *(2013-04-15)*
----------------------------

 * New: FEST module for testing ActionBarSherlock classes.
 * New: i18n module for including internationalized strings used by actionbar.
 * Removed dialog themes.
 * Fix: `SearchView` suggestions now work properly.
 * Fix: Prevent rare NPE when restoring state on pre-Honeycomb devices.
 * Fix: Correct behavior of `IcsColorDrawable`'s `setAlpha` method.
 * Fix: Handle cases where `Locale.ROOT` is not present on pre-Honeycomb.
 * Fix: Correct tab measurement edge-case on pre-Honeycomb.
 * Use custom Toast layout to ensure consistent look and feel.
 * Fix: Prevent monkey runners from uselessly crashing.
 * Fix: Do not hold on to old menu items after a call to `invalidateOptionsMenu`.
 * Fix: Ensure `ShareActionProvider` works when only one intent is available.
 * Fix: Scroll list navigation to the selected item when opened.


Version 4.2.0 *(2012-10-07)*
----------------------------

**Maven `artifactId` is now 'actionbarsherlock'.**

Note: The `.Dialog` themes are now deprecated. These will be removed in a future
version of the library.

 * Add `SearchView` widget for standard search interaction (API 8+ only)
 * Fix: `ShareActionProvider` in the split action bar no longer fills the entire
   screen.
 * Fix: `ShareActionProvider` now does file I/O on a background thread.
 * Fix: Automatically correct `ColorDrawable` not respecting bounds when used as
   a stacked background.
 * Fix: Ensure fragments collection is present before dispatching events.
 * Fix: XML-defined `onClick` searches the correct context for the declared
   method.
 * Fix: Ensure action mode start/finish callbacks are invoked on the activity
   for the native action bar.
 * Fix: Allow tab callbacks to have a fragment transaction instance for any
   `FragmentActivity`.
 * Fix: Ensure `CollapsibleActionView` callbacks are dispatched in both native
   and compatbility action bars.
 * Fix: Remove `.ForceOverflow` themes. These never should have been included.


Version 4.1.0 *(2012-05-17)*
----------------------------

 * Fix: Altered technique used for menu event dispatching through the fragment
   manager for greater control.
 * Fix: Do not dispatch menu creation event if the activity has been destroyed.
 * Fix: Correct potential `NullPointerException` when expanding an action item.
 * Fix: Correct potential `NullPointerException` when the hardware menu key was
   pressed in an activity that is forcing the overflow menu.
 * Fix: Do not set a listener on the native action bar tab wrapper unless a
   compatibility listener has been set.
 * Fix: Ensure the compatibility animation framework is always available on
   views even if they were previously detached from the view hierarchy.


Version 4.0.2 *(2012-04-15)*
----------------------------

 * Upgrade to r7 support library.
 * Fix: Do not trigger menu creation after `onCreate` if activity is finishing.
 * Fix: Prevent overflow from displaying if there are no overflow action items.
 * Fix: Long-pressing menu key no longer triggers overflow.
 * Fix: Use proper tab state-list drawable to mimic ICS.
 * Fix: Ensure dispatching menu creation and preparation to fragments can
   properly return `false` when appropriate to avoid rendering artifacts.
 * Fix: Properly save and fetch action mode tag on ICS.
 * Fix: Add missing density-specific resources for certain asssets and remove
   unused assets.


Version 4.0.1 *(2012-03-25)*
----------------------------

 * Add `ShareActionProvider` widget for use as action items.
 * Re-add 'Styled' sample to provide a more comprehensive theming example.
 * Fix: Do not dispatch options item selection to fragments if the activity
   handles the callback.
 * Fix: Prevent menu key from opening the overflow menu when an action mode is
   currently displayed.
 * Fix: Ensure fragment transaction instance is not `null` on initial tab
   selection callback.
 * Fix: Displaying an action mode while using stacked tab navigation no longer
   throws an exception.
 * Fix: Using expandable action item callbacks no longer results in a possible
   exception on older devices.


Version 4.0.0 *(2012-03-07)*
----------------------------

Complete rewrite of the library to backport the Android 4.0 action bar.

 * The minimum supported version of Android is now 2.1 (API 7).
 * New base activities are provided (e.g., `SherlockActivity` and
   `SherlockFragmentActivity`) which extend from the native activities.
 * The support library sources are no longer included in the library. You must
   include `android-support-v4.jar` in your project separately.
 * Theming now mirrors that of the native action bar through the use of multiple
   styles rather than through `ab`- and `am`-prefixed attributes in the theme.
 * The action bar can be statically attached to an activity view without the
   requirement of using one of the provided base activities.


Version 3.5.1 *(2012-01-03)*
----------------------------

 * Fix: `NullPointerException` in `FragmentManager` can no longer occur when an
   attempt is being made to save to a `Bundle` that has not yet been created.
 * Fix: Pre-3.0 action item submenu dialogs now properly dismiss themselves when
   an item of theirs is selected.


Version 3.5.0 *(2011-12-18)*
----------------------------

 *  Library now uses the `r6` version of the compatibility library for its base.
    Ice Cream Sandwich-specific implementations are currently disabled, however,
    but will be added in a future version of the library.

    `MenuCompat`, `MenuItemCompat`, and `ActivityCompat` have be added back in
    to ease transition to this library but all their methods and the classes
    themselves have been deprecated.
 *  Rewritten menu and action item support from Ice Cream Sandwich.

    * Removed the need for the custom `Window.FEATURE_ACTION_ITEM_TEXT` flag.
      You should now use the `showAsAction` attribute and/or the
      `setShowAsAction(int)` method on each `MenuItem` to control whether or
      not text is shown
    * Action item dividers are now added automatically only when necessary
      to distinguish possible confusion between action items.
    * Fix: Action views now properly size themselves within the bounded space
      of the menu.

 *  Fix: List navigation no longer becomes unusable on certain device
    configurations.
 *  Fix: `SubMenu`'s `findItem(int)` method now properly returns the support
    version of `MenuItem`.
 *  Fix: Invisible sub-menu items are no longer shown on the pre-3.0 popup list.


Version 3.4.2 *(2001-11-09)*
----------------------------

 * Fix: Stacked action bar now properly sets the tab bar background based on
   the theme.


Version 3.4.1 *(2011-11-09)*
----------------------------

 * The `makeFragmentName` method in `FragmentPagerAdapter` has been changed to
   `public` scope to allow for easier access to your fragments that it is
   managing.
 * Action bar will now animate when calling `show()` or `hide()`.
 * `SherlockPreferenceActivity` now provides full fragment and loader support.
 * Examples for the plugins are now in their own sample application.
 * Fix: Home icon no longer erroneously clipped when it exceeds the size of the
   action bar.
 * Fix: Tabs will now scroll horizontally to mimic the native action bar
   behavior.
 * Fix: Plugins now properly DO NOT inline their `R.java` integer constants.
 * Fix: Tabs below the action bar are now styled with a default background so
   that they do not incorrectly inherit an applied background unless explicity
   declared.


Version 3.4.0 *(2011-10-30)*
----------------------------

 * Library now uses the `r4` version of the compatibility library for its base.
   Ice Cream Sandwich-specific implementations are currently disabled, however,
   but will be added in a future version of the library.
 * Context menu callbacks now use the support version of `MenuItem` to maintain
   consistency.
 * Added preference plugin which provides an action bar enhanced preference
   screen.
 * Fix: `abHomeLayout` theme attribute is now honored.
 * Fix: `onPrepareOptionsMenu` is now properly dispatched upon menu
   invalidation.


Version 3.3.1 *(2011-10-20)*
----------------------------

ADT 14 is now required. Maven 3 is required if building from the command line.

 * XML-defined `onClick` attributes will now check for an `onClick` method that
   takes an `android.support.v4.view.MenuItem` instance.
 * Tabs on medium screens in landscape now display inline rather than below the
   action bar to mirror how Android 4.0 behaves with the same configuration.
 * Fix: Menu inflater properly checks activity context for `onClick` method
   declared in the XML.
 * Fix: Dialog fragment properly saves its `showDialog` state when not being
   used as a popup.
 * Fix: Return `-1` when in tab navigation but no tab is selected. This brings
   the library in line with the post-3.0 behavior.
 * Fix: Removing a menu group no longer throws an `IndexOutOfBoundsException`.
 * Fix: `getSelectedTab` and `getTabAt` no longer throw `NullPointerException`s
   on post-3.0 when no tab was selected or no tab existed at the specified
   position, respectively.
 * Fix: `findFragmentById` now properly returns fragments attached to
   `android.R.id.content` when run on pre-3.0 devices.


Version 3.3.0 *(2011-10-11)*
----------------------------

 * Tabs are now displayed below the action bar on all medium-screen devices and
   portrait large-screen devices.
 * Fix: Dialog fragments no longer throw an `IllegalStateException` when being
   used as a regular fragment (i.e., not as a popup). See
   [StackOverflow](http://stackoverflow.com/questions/5637894/dialogfragments-with-devices-api-level-11/7560686#7560686)
   for more information.
 * Fix: Popping a fragment off of the back stack now properly assigns its parent
   activity.
 * Fix: An activity result no longer causes a `NullPointerException` when the
   target fragment no longer exists.
 * Fix: Action item dividers are now properly initially hidden when their
   associated action items are as well.


Version 3.2.3 *(2011-09-16)*
----------------------------

 * Fix: Fragments in a `ViewPager` that contributed items to the options menu
   were caught in a race condition causing inconsistent results when a new page
   was selected. This regression was introduced in version 3.2.2.


Version 3.2.2 *(2011-09-15)*
----------------------------

 * Fix: Side-effects related to using `FragmentMapActivity` due to how it was
   referencing resources from the main library.
 * Fix: Fragments adjacent to the currently selected fragment in a `ViewPager`
   no longer receive context menu events.
 * Fix: Eliminate exception when inflating context menus on 3.0+ when using
   `getMenuInflater()`.
 * Fix: `ViewPager` now determines whether or not an activity menu invalidation
   is required independently of whether or not fragments were created or
   destroyed. This should fix an edge case where an activity with a `ViewPager`
   containing only two fragments would not get its menu properly invalidated.


Version 3.2.1 *(2011-09-12)*
----------------------------

 * Fix: Action mode API incorrectly using the native `Menu` and `MenuItem`
   classes causing an easy pitfall for `ClassCastExceptions`.
 * Fix: Large action bar backgrounds increasing the size beyond that alloted in
   the theme.


Version 3.2.0 *(2011-09-05)*
----------------------------

 *  Added support for `MapView` and the Google APIs through the use of
    `FragmentMapActivity`. If you are using a map within a fragment you must
    ensure it is always attached to an activity which extends from this new base
    class.

    Since supporting maps requires compiling against the Google APIs, this
    functionality is implemented in the form of a plugin which is to be used
    alongside the normal library. You can choose to add it as an additional
    library project or by including it as a `.jar`. Maven users may simply
    include the additional dependency (artifactId: `plugin-maps`).
 *  Fix: Fragments adjacent to the currently selected fragment in a `ViewPager`
    no longer contribute to the activity menu.
 *  `ActionBar.Tab` has been changed from an interface to an abstract class to
    mirror its native counterpart.


Version 3.1.3 *(2011-08-14)*
----------------------------

 * Renamed all resources to be prefixed with `abs__` to avoid conflicts when
   including in your project.
 * Fix: Action bar background being set on two views causing artifacts to remain
   on screen when the action bar was hidden.
 * Fix: Incorrect sub-menu item being selected by default when the sub-menu was
   triggered from the native options menu on pre-3.0.
 * Fix: `MenuItem.setVisible` now properly updates the associated action item and
   native menu item visible state.
 * Fix: Adding items to a menu now honors its ordering and category.
 * Fix: Fragment options item selected callback now uses the proper version of
   `MenuItem`.


Version 3.1.2 *(2011-08-07)*
----------------------------

 * Fix: `MenuItem.getMenuInfo()` was throwing runtime exception. Will now just
   return `null`.
 * Fix: Dragging over a `WebView` contained in a `ViewPager` would not register.
 * Fix: Inflation of context menu incorrectly being handled by the custom menu
   inflater for the library.


Version 3.1.1 *(2011-07-31)*
----------------------------

 * Fix: `MenuItem.getSubMenu` now returns a support instance rather than a
   native instance.
 * Fix: Fragment methods `onAttach` and `onInflate` incorrectly regressed to use
   `Activity` instead of a `FragmentActivity` in their method signatures.
 * Fix: Retained fragments not being re-attached on pre-3.0 when attached to
   `android.R.id.content` upon activity recreation.
 * Fix: `onPrepareOptionsMenu` not dispatched to fragments. This still will only
   occur if the activity method returns true (which is the default).
 * Fix: `Menu.findItem` not returning `null` when the item was not found on
   Android 3.0+.


Version 3.1.0 *(2011-07-22)*
----------------------------

Due to shortcomings in the Android theming system, a small change must be made
in how this library handles themes. If you were using a custom style for
`actionBarStyle` you must now specify its attributes in the root of the theme
and prefix them with 'ab'.

You can see an example of this in the `SherlockCustom` theme in
`samples/demos/res/values/styles.xml`.

 * Library now uses the `r3` version of the compatibility library for its base.
 * `actionBarStyle` is no longer a valid theme attribute (see note above).
 * Added the demo project included with the new compatibility library under
   `samples/demos/` and merged in the old 'featuredemo'.
 * Dividers are now shown on pre-3.0 devices between all action items.
 * `Window.FEATURE_ACTION_BAR_OVERLAY` is now honored on pre-3.0 devices.
 * Inflation of XML menu resources will now honor `android:actionLayout` and
   `android:actionViewClass` attributes.
 * Buttons for displaying the determinate and indeterminate progress bars have
   been added to the feature toggle demo.
 * Added support for indeterminate progress bar. Due to the `final` modifier on
   the native type, you must use `setIndeterminateProgressBarVisibility(Boolean)`
   and pass `Boolean.TRUE` or `Boolean.FALSE`.
 * Fix: `MenuBuilder#removeItem(int)` and `MenuBuilder#findItem(int)` throwing
   `IndexOutOfBoundsException`s when the item was not found.
 * Fix: Theme attributes for home item data (e.g., icon, logo) will not be
   overwritten by the special `MenuItem` instance for home.
 * Fix: Native strings can now be specified for an XML menu `<item>` in
   `android:title` and `android:titleCondensed`.
 * `Window.FEATURE_ENABLE_ACTION_BAR_WATSON_TEXT` is now
   `Window.FEATURE_ACTION_BAR_ITEM_TEXT`.
 * `Widget.Sherlock.Spinner.DropDown.ActionBar` and
   `Widget.Sherlock.Light.Spinner.DropDown.ActionBar` styles are now
   `Widget.Sherlock.Spinner` and `Widget.Sherlock.Light.Spinner`, respectively.
 * `Widget.Sherlock.ActionBarView_TabXXX` styles are now
   `Widget.Sherlock.ActionBar.TabXXX`.


Version 3.0.3 *(2011-07-17)*
----------------------------

This version is a hotfix for incompatibilities introduced with the SDKs for
3.1 r2 and 3.2 r1. Due to unavoidable changes in the underlying SDK, the library
must now be compiled against API level 13.

 * `actionModeStyle` and `actionModePopupWindowStyle` are no longer valid theme
   attributes.


Version 3.0.2 *(2011-06-23)*
----------------------------

 * Sub-menus for action items are now shown in a list dialog.
 * Moved certain classes to the `com.actionbarsherlock.internal` package which
   were not meant for public consumption. Despite being given `public` scope in
   this new package, these classes should **NOT** be used under any circumstances
   as their API can be considered highly volatile and is subject to change often
   and without warning.


Version 3.0.1 *(2011-06-08)*
----------------------------

 * Fix: `onOptionsItemSelected()` not being called in fragments if the activity
   version returns `false`.
 * Fix: `onCreateOptionsMenu()` not being called in fragments on Android 3.0+.
 * New: Enable action item text display on pre-Android 3.0 by calling
   `requestWindowFeature` with `Window.FEATURE_ENABLE_ACTION_BAR_WATSON_TEXT`.
 * Fix: `setCustomView()` no longer automatically enables the custom view on
   pre-3.0. You must call `setDisplayShowCustomEnabled()` in order to display
   the view.


Version 3.0.0 *(2011-06-05)*
----------------------------

The API has been rewritten to mimic that of the native action bar. As a result,
usage now only requires changing a few imports to use the support versions
of classes and calling `getSupportActionBar()`. See the README for more info.

The rewrite necessitated tight interaction with the
[compatibility library](http://android-developers.blogspot.com/2011/03/fragments-for-all.html)
to the point where its sources are now included. You are no longer required to
have the standalone `.jar` file.

Also included is a default custom action bar for use by default on pre-3.0
devices. This custom implementation is based off of Johan Nilsson's
[Android-ActionBar](https://github.com/johannilsson/android-actionbar) and the
[work that I have done](https://github.com/johannilsson/android-actionbar/pull/25)
on it.

More details are available at http://actionbarsherlock.com


Version 2.1.1 *(2011-03-21)*
----------------------------

**No changes to library code.**

 * Moved library to the root of the repository.
 * Added `samples/dependencies.py` script to automatically download the needed
   dependencies for the sample projects.


Version 2.1.0 *(2011-03-21)*
----------------------------

**WARNING**: The
[Android Compatibility Library (v4)](http://android-developers.blogspot.com/2011/03/fragments-for-all.html)
is now required.

 * Added `ActionBarSherlock.Activity`, `ActionBarSherlock.FragmentActivity`,
   and `ActionBarSherlock.ListActivity` for extension by implementing
   activities, the latter of which is deprecated. This affords a much tighter
   integration and allows for the use of other new features listed below.
 * New API method: `layout(Fragment)` will use the fragment argument as the
   content to the activity.
 * New API method: `menu(int)` allows for the inflation of menu XMLs from a
   resource. For the non-native implementation, the XML can be inflated to a
   custom Menu which can then be applied appropriately to the third-party
   action bar. Sub-menus are also supported. Third-party action bar handlers
   should implement `ActionBarSherlock.HasMenu` for this functionality. *This
   feature requires that activities extend from one of the provided activity
   base classes.*
 * New API method: `homeAsUp(boolean)`. This mimics the native method
   `setDisplayHomeAsUpEnalbed` on the native action bar. Third-party action bar
   handlers should implement `ActionBarSherlock.HasHomeAsUp` for this
   functionality.
 * New API method: `useLogo(boolean)` will trigger the action bar to hide the
   application icon/home button and title and show a larger logo representing
   the application. Third-party action bar handlers should implement
   `ActionBarSherlock.HasLogo` for this functionality.
 * New API method: `listNavigation(SpinnerAdapter, OnNavigationListener)`. Tells
   the action bar to use drop-down style navigation with the specified list of
   items and callback listener. Third-party action bar handlers should
   implement `ActionBarSherlock.HasListNavigation` for this functionality.
 * Javadocs are now available at
   [jakewharton.github.com/ActionBarSherlock](http://jakewharton.github.com/ActionBarSherlock/).
 * A standalone JAR is now available via the
   [GitHub downloads page](https://github.com/JakeWharton/ActionBarSherlock/downloads)
   or in my
   [personal maven repository](http://r.jakewharton.com/maven/)
   as `com.jakewharton:android-actionbarsherlock:2.1.0`.


Version 2.0.1 *(2011-03-11)*
----------------------------

 * Use `Class.forName()` for detection of native action bar. This provides
   compatability all the way back to Android 1.5.


Version 2.0.0 *(2011-03-09)*
----------------------------
Complete rewrite!

 * New and better API.
 * More sane logic and attachment to activity.
 * Extensible via generics. Implement any ActionBar or roll your own with
   minimal effort.
 * Now a library project for easy inclusion in applications.


Version 1.0.0 *(2011-03-07)*
----------------------------
Initial release.
