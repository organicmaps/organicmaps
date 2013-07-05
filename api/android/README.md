# MapsWithMe Android API: Getting Started

## Introduction
MapsWithMe Android API (hereinafter referred to as *"API Library"* or just *"library"*)
provides interface for client application to perform next tasks:

* Show one or more points on offline map of [MapsWithMe Application][linkMwm]
* Come back to the client application after selecting specific point on the map, by sending [PendingIntent][linkPIntent] with point data when user asks for more information by pressing "More Info" button in MapsWithMe Application
* Map screen brending : your application's icon and name (or custom title) will be placed at the top.

Thus, you can provide **two way communication between your appication and MapsWithMe**,
using MapsWithMe to show points of interest (POI) and providing more information in your app.

Please refer to [sample application][linkSampleSource] for demo.

## Prerequisites

It is supposed that you are familiar with Android Development, and you have Android SDK and Eclipse (or another IDE of your choice) installed.
You should be familiar with concept of [Intents][linkIntents], [library projects][linkLibProj], and [PendingIntents][linkPIntent] (recommended) as well.
Your application must target at least *android sdk version 7*.

## Integration
First step is to clone [repository][linkRepo] or download it as an archive.

When your are done you find two folders: *lib* and *sample-app-capitals*. First one is a library project that you should add to your project.
You don't need any additional permissions in your AndroidManifest.xml to use API library, so you can write real code straight away, calling for different `MapsWithMeApi` methods (more details below). 

##Classes Overview and HOW TO
Core classes you will work with are:

* [com.mapswithme.maps.api.MapsWithMeApi][linkApiClass] - static class with methods such as `showPointOnMap(Activity, double, double, String)` etc.
* [com.mapswithme.maps.api.MWMPoint][linkPointClass] - model of POI, includes lat, lon, name, and id data.
* [com.mapswithme.maps.api.MWMResponse][linkRespClass] - helps you to extract response from MapsWithMe by applying `MWMResponse.extractFromIntent(Intent)` to Intent. Contains MWMPoint data.

### Show Points on the Map

The simplest usage:

    public class MyPerfectActivity extends Activity {
    ...

      void showSomethingOnTheMap(SomeDomainObject arg)
      {
        // Do some work, create lat, lon, and name for point
        final double lat = ...;
        final double lon = ...;
        final String name = ...;
        // Ask MapsWithMe to show the point
        MapsWithMeApi.showPointOnMap(this, lat, lon, name);
      }
    ...

    }

For multiple points use [MWMPoint][linkPointClass] class:

    void showMultiplePoints(List<SomeDomainObject> list)
    {
      // Convert objects to MMWPoints
      final MWMPoint[] points = new MWMPoint[list.length];
      for (int i = 0; i < list.size; i++)
      {
        // Get lat, lon, and name from object and assign it to new MMWPoint
        points[i] = new MWMPoint(lat, lon, name);
      }
      // Show all point on the map, you could also provide some title
      MapsWithMeApi.showPointsOnMap(this, "Look at my points, my points are amazing!", points);
    }


### Ask MapsWithMe to Call my App

We support PendingIntent interaction (just like Android native
NotificationManager does). You should specify ID for each point to
diftiguish it leter, and PentingIntent that MapsWithMe will send back to
your application when user press "More Info" button :

    // Here is how to pass points with ID ant PendingIntent
    void showMultiplePointsWithPendingIntent(List<SomeDomainObject> list, PendingIntent pendingIntent)
    {
      // Convert objects to MMWPoints
      final MWMPoint[] points = new MWMPoint[list.length];
      for (int i = 0; i < list.size; i++)
      {
        //                                      ||
        //                                      ||
        //                                      \/
        //         Now you should specify string ID for each point
        points[i] = new MWMPoint(lat, lon, name, id);
      }
      // Show all points on the map, you could also provide some title
      MapsWithMeApi.showPointsOnMap(this, "This title says that user should choose some point", pendingIntent, points);
    }

    //Code below shows general way to extract response data
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // Handle intent you specified with PandingIntent
        // Now it has additional information (MWMPoint).
        handleIntent(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
      super.onNewIntent(intent);
      // if defined your activity as "SingleTop"- you should use onNewIntent callback
      handleIntent(intent);
    }

    void handleIntent(Intent intent)
    {
      // Apply MWMResponse extraction method to intent
      final MWMResponse mwmResponse = MWMResponse.extractFromIntent(this, intent);
      // Here is your point that user selected
      final MWMPoint point = mwmResponse.getPoint();
      // Now, for instance you can do some work depending on point id
      processUserInteraction(point.getId());
    }

## FAQ

#### How should I detect if user has MapsWithMe installed?
`MapsWithMeApi.isMapsWithMeInstalled(Context)` will return `true` if user has *Lite* or *Pro* version that supports API call installed.

#### Which versions of MapsWithMe support API calls?
Both *Lite* and *Pro* versions since 2.4.0 are support API calls.

#### What will happen if I call for `MapsWithMeApi.showPoint()` but MapsWithMe application is not installed?
Nothing serious. API library will show simple dialog with gentle offer to download MapsWithMe. You can see how it looks like below. ![Please install us](site/images/dlg.png)

#### If user has both *Lite* and *Pro* versions which one will be called?
MapsWithMe Pro will serve your request in the case if both *Lite* and *Pro* versions installed. 

## Sample Code and Application

* [Sample Application at Google Play][linkSampleGooglePlay]
* [Sample Application Source Code][linkSampleSource]

## Support
If you have any questions please email to [api@mapswith.me][linkSupport].

-------------------------------------------------------------------------------
## API Code License
Copyright (c) 2013, MapsWithMe GmbH
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

[linkMwm]: http://mapswith.me/ "MapsWithMe"
[linkPIntent]: http://developer.android.com/reference/android/app/PendingIntent.html "PendingIntent"
[linkRepo]: https://github.com/mapswithme/api-android "GitHub Repository"
[linkLibProj]: http://developer.android.com/tools/projects/index.html#LibraryProjects "Android Library Project"
[linkIntents]: http://developer.android.com/guide/components/intents-filters.html "Intents and Intent Filters"
[linkSupport]: mailto:api@mapswith.me "MapsWithMe Support Contact"
[linkApiClass]: lib/src/com/mapswithme/maps/api/MapsWithMeApi.java "MapsWithMeApi.java"
[linkPointClass]: lib/src/com/mapswithme/maps/api/MWMPoint.java "MWMPoint.java"
[linkRespClass]: lib/src/com/mapswithme/maps/api/MWMResponse.java  "MWMResponse.java"
[linkSampleSource]: sample-app-capitals "Api Source Code"
[linkSampleGooglePlay]: http://example.com "Api Demo .apk"
