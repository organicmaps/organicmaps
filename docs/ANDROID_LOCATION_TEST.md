# Android Location Test Cases

This document describes manual testing flows to validate location
implementation on Android.

0. Initial start

- Remove the app with all data and install it again;
- Enable Wi-Fi and Location (normal behavior);
- Start the app and ensure that system dialog about location permissions
  appears on the top of splash screen;
- Grant "Precise" or "Approximately" (doesn't matter) permissions
  for "Only this time".
- [fdroid only] Initial map download dialog should appear and suggest to
  download map of your area (a check box);
- The main map screen should start with cool animation to your current location.

1. GPS cold start

- Disable Wi-Fi, disable Cellular Data, enable Location;
- Get under a roof and away from the open sky and any windows;
- Restart device, start the app, and wait 1 minute until
  "Continue detecting your current location?" appears;
- Don't tap any buttons in the dialog - the search for a location
  should continue in the background until "Stop" is explicitly taped;
- Go out to have the open sky and wait 1-5 minutes GPS to find satellites
  without locking device and switching to another apps.
-  "Continue detecting your current location?" should disappear
  automatically when location is found;
- The screen should be automatically on all the time during the process.

This test-case should work with the same behavior regardless of
 "Google Play Services"  option in the app settings.

2. Location disabled by user

- Follow instructions from (1) until the dialog appears,
  but press "Stop" button in the dialog;
- Check that location icon is crossed out;
- Switch to some other app and back;
- Check that location icon is crossed out;
- Kill the app and start it again;
- Check that location icon is crossed out;
- Tap on location icon;
- Ensure that location search has started;
- Wait until location is found under the open sky.

This test-case should work with the same behavior regardless of
 "Google Play Services"  option in the app settings.

3. Google location dialog (positive case)

- Use Google flavor and enable Google Play Location in the app settings;
- Disable Wi-Fi, disable Cellular Data, disable Location;
- Start the app and initiate location search by tapping on location button;
- "For better experiences..." Google dialog should appear immediately;
- Tap "Enable" in the dialog and it should enable Location in the system;
- Location should start working as in (1) case.

4. Google location dialog (negative case)

- Try the same steps as in (3), but tap "No thanks" button in the dialog.
- Check that location icon is crossed out immediately;
- "For better experiences..." Google dialog should appear;
- Tap "No thanks" button again;
- Check that location icon is crossed out immediately;
- Switch to some other app and back;
- Check that location icon is still crossed out and
  "For better experiences..." dialog doesn't appear automatically;
- Kill the app and start it again;
- Check that location icon is still crossed out and
  "For better experiences..." dialog doesn't appear automatically;
- Tap on location button - "For better experiences..." dialog
  should re-appear again.

5. OM location dialog (negative case)

- Use non-Google flavor or disable Google Play Location in the app settings;
- Disable Wi-Fi, disable Cellular Data, disable Location;
- Start the app and initiate location search by tapping on location button;
- "Please enable Location Services" information dialog should appear;
- Check that location icon is crossed out immediately;
- Switch to some other app and back;
- Check that location icon is still crossed out and
  "Please enable Location Services" dialog doesn't appear automatically;
- Kill the app and start it again;
- Check that location icon is still crossed out and
  "Please enable Location Services" dialog doesn't appear automatically;
- Tap on location button - "Please enable Location Services" dialog
  should re-appear again.

6. Location disabled when app is running

- Disable Wi-Fi, disable Cellular Data, enable Location;
- Get location acquired in the app (the blue location arrow);
- Disable system Location by swiping down from Android's menu bar
  to disable location via "Quick settings" panel WITHOUT switching
  to Settings or any other apps;
- "Please enable Location Services" or  "For better experiences..."
  dialog should appear immediately, depending in Google Play Service
  availability;
- Further taps on location button will lead to (4) or (5).
