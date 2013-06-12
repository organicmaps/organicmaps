# MapsWithMe Android API: Getting Started

## Introduction
MapsWithMe Android API (hereinafter referred to as *"API Library"* or just *"library"*)
provides interface for client application to perform next tasks:

* Show one or set of the points on offline map of [MapsWithMe Application][linkMwm]
* Send arbitrary [PendingIntent][linkPIntent] with point data when
  user asks for more information by pressing "More Info" button in MapsWithMe Application

Thus, you can provide two way communication between your appication and MapsWithMe,
using MapsWithMe to show points of interest (POI) and providing more information in your app.

Please refer to [sample application][linkSample] for demo.

## Prerequisites
It is supposed that you are familiar with Android Development, and you have Android SDK and Eclipse (or another IDE of your choice) installed.
You should be familiar with concept of [Intents][linkIntents], [library projects][linkLibProj], and [PendingIntents][linkPIntent] (recommended) as well.
Your application must target at least *android sdk version 7*.
## Integration
First step is to clone [repository][linkRepo] or download it as an archive.

When your are done you find two folders: *lib* and *sample-app-capitals*. First one is a library project that you should add to your project.
You don't need any additional permissions in your AndroidManifest.xml to use API library, so you can write real code straight away.

##Classes Overview and HOW TO
Core classes you will work with are:

* [com.mapswithme.maps.api.MapsWithMeApi][linkApiClass] - static class with methods such as `showPointOnMap(Activity, double, double, String)` etc.
* [com.mapswithme.maps.api.MWMPoint][linkPointClass] - model of POI, includes lat, lon, name, and id data.
* [com.mapswithme.maps.api.MWMResponse][linkRespClass] -

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

We support PendingIntent interaction (just like Android system
NotificationManager does). You need to specify ID for each point to
diftiguish it leter, and PentingIntent that MapsWithMe send back to
your application:

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
      // if defined your activity as "SingleTop"- you should use onNewIntent
      handleIntent(intent);
    }

    void handleIntent(Intent intent)
    {
      final MWMResponse mwmResponse = MWMResponse.extractFromIntent(this, intent);
      // Here is your point that user selected
      final MWMPoint point = mwmResponse.getPoint();
      // Now, for instance you can do some work depending on point id
      processUserInteraction(point.getId());
    }

## Docs [TODO add link to javadoc]

## Sample Code [TODO add link to sample code]

## FAQ [TODO to be defined]

## Support
If you have any questions please email to [api@mapswith.me][linkSupport].

[linkMwm]: http://mapswith.me/ "MapsWithMe"
[linkPIntent]: http://developer.android.com/reference/android/app/PendingIntent.html "PendingIntent"
[linkSample]: http://example.com "Sample Application"
[linkRepo]: http://example.com "GitHub Repository"
[linkLibProj]: http://developer.android.com/tools/projects/index.html#LibraryProjects "Android Library Project"
[linkIntents]: http://developer.android.com/guide/components/intents-filters.html "Intents and Intent Filters"
[linkSupport]: mailto:api@mapswith.me "MapsWithMe Support Contact"
[linkApiClass]: http://example.com "MapsWithMeApi.java"
[linkPointClass]: http://example.com "MWMPoint.java"
[linkRespClass]: http://example.com "MWMResponse.java"
