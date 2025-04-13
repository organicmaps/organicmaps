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
- [FDroid only] Initial map download dialog should appear and suggest to
  download map of your area (a check box);
- The main map screen should start with cool animation to your current location.

1. GPS cold start

- Disable Wi-Fi, disable Cellular Data, enable Location;
- Get under a roof and away from the open sky and any windows;
- Restart device, start the app
- Wait until location is detected, or go outside to have the open sky and wait
for a few minutes so the device can find GPS satellites, without locking device
and switching to another apps.
- The screen should be automatically on all the time during the process.

This test-case should work with the same behavior regardless of
"Google Play Services" option in the app settings.

2. Location disabled by user

- Follow first 3 steps from (1) above but stop detecting location by pressing
the location "rotating radar" button;
- Check that location icon is crossed out;
- Switch to some other app and back;
- Check that location icon is crossed out;
- Kill the app and start it again;
- Check that location icon is crossed out;
- Tap on location button;
- Ensure that location search has started;
- Wait until location is found under the open sky.

This test-case should work with the same behavior regardless of
"Google Play Services" option in the app settings.

3. Google location dialog (positive case)

- Use Google flavor and enable Google Play Location in the app settings;
- Disable Location, but enable Cellular Data and/or Wi-Fi;
- Start the app;
- Check that location icon is crossed out immediately;
- "To continue, turn on device location, which uses Google's location service"
  Google dialog should appear;
- Tap "OK" in the dialog;
- Check that system location has been enabled;
- The app will start searching for location as in (1) case.

4. Google location dialog (negative case)

- Try the same steps as in (3), but tap "No thanks" button in the dialog.
- Check that location icon is crossed out immediately;
- "To continue, turn on..." Google dialog should appear;
- Tap "No thanks" button again;
- Check that location icon is still crossed out;
- Switch to some other app and back;
- Check that location icon is still crossed out and
  "To continue, turn on..." dialog doesn't appear automatically;
- Kill the app and start it again;
- Check that location icon is still crossed out and
  "To continue, turn on..." dialog doesn't appear automatically;
- Tap on location button - "To continue, turn on device..." dialog
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
- "Please enable Location Services" or "For better experiences..."
  dialog should appear immediately, depending in Google Play Service
  availability;
- Further taps on location button will lead to (4) or (5).

7. Pending (searching) location mode

- Disable Wi-Fi, disable cellular data, and enable location.
- Move your phone away from the open sky and any windows to make sure that GPS can't be acquired.
- If the location search hasn't already begun, press the location button to start it.
- The location icon MUST be "locating" while searching for a GPS signal, and location icon should be displayed in the system tray.
- Touch, drag or try to zoom in and zoom out the map - the icon MUST NOT change from "locating" mode.
- Press the location button to disable location search, its icon should change, and system location icon disappear after a few seconds.
- Press the location button again to start searching for location.