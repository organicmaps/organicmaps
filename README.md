# Tourism Map Tajikistan
This app is for Tajikistan tourists. It's based on open source app Organic map.

## Please Read all of this before continuing
Don't forget make changes to this file, when you make significant changes to navigation.  
version of Android Studio I used: android-studio-2024.1.1.13-mac_arm.dmg  
version of XCode I used: 16.1

## Navigation on Android
The first activity app starts is SplashActivity.java. There it navigates to MainActivity  
where all of the data about Tajikistan places are located,  
it has its own navigation system on Jetpack compose.  
There we navigateToMapToDownloadIfNotPresent() and navigateToAuthIfNotAuthed() being called.  
When map download is finished it will go to MainActivity.  
When you sign in or up, it will navigate to MainActivity  

## Navigation on iOS
The first screen to be shown is Map screen (see MapsAppDelegate.mm). I (Emin) couldn't change it.  
There's such logic in MapsAppDelegate.mm:   
```
if (Tajikistan is loaded) {
    if (token is nil) navigate to Auth (note: token is cleared when user signs out)
    else navigate to TourismMain (Home)
}
```
In Auth when user signs in or up, it navigates to TourismMain  
In TourismMain goes to auth if not authorized

## Features of their map

Organic Maps is the ultimate companion app for travellers, tourists, hikers, and cyclists:

- Detailed offline maps with places that don't exist on other maps, thanks to [OpenStreetMap](https://openstreetmap.org)
- Cycling routes, hiking trails, and walking paths
- Contour lines, elevation profiles, peaks, and slopes
- Turn-by-turn walking, cycling, and car navigation with voice guidance
- Fast offline search on the map
- Bookmarks and tracks import and export in KML, KMZ & GPX formats
- Dark Mode to protect your eyes
- Countries and regions don't take a lot of space
- Free and open-source

## Copyrights

Licensed under the Apache License, Version 2.0. See
[LICENSE](https://github.com/Ohpleaseman/tourism/blob/master/LICENSE),
[NOTICE](https://github.com/Ohpleaseman/tourism/blob/master/NOTICE)
for more information.
