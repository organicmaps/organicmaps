java
package com.example.myapp.utils;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.util.Log;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MapLinkHandler {

    private static final String TAG = "MapLinkHandler";

    // Regex patterns for extracting coordinates and optionally a query/label

    // Google Maps patterns
    // 1. Matches q=label@lat,lon (e.g., https://maps.google.com/maps?z=16&q=Mezza9%401.3067198,103.83282)
    private static final Pattern GOOGLE_MAPS_Q_COORD_LABEL_PATTERN = Pattern.compile("q=([^@]+)@([\\d.-]+),([\\d.-]+)");
    // 2. Matches q=lat,lon (e.g., https://maps.google.com/maps?q=55.751809,37.6130029)
    private static final Pattern GOOGLE_MAPS_Q_ONLY_COORD_PATTERN = Pattern.compile("q=([\\d.-]+),([\\d.-]+)");
    // 3. General pattern: Matches @lat,lon,zoom or @lat,lon (e.g., https://www.google.com/maps/place/Falafel+M.+Sahyoun/@33.8904447,35.5044618,16z)
    // This pattern is robust and covers the previously separate "/place/.../@lat,lon" cases.
    private static final Pattern GOOGLE_MAPS_GENERAL_COORD_PATTERN = Pattern.compile("@([\\d.-]+),([\\d.-]+)(?:,[\\d.]+z?)?"); // Catches @lat,lon and optional zoom (e.g., ,16z or ,16)

    // OpenStreetMap patterns
    // Matches #map=zoom/lat/lon (e.g., https://www.openstreetmap.org/#map=16/33.89041/35.50664)
    private static final Pattern OSM_COORD_PATTERN = Pattern.compile("#map=\\d+/([\\d.-]+)/([\\d.-]+)");

    // Apple Maps patterns
    // Matches ll=lat,lon in query parameters (e.g., https://maps.apple.com/place?ll=53.991561,-111.289525)
    private static final Pattern APPLE_MAPS_COORD_PATTERN = Pattern.compile("ll=([\\d.-]+),([\\d.-]+)");

    /**
     * Attempts to open a map URL using a geo: URI, or falls back to the original URL.
     * It presents a chooser to the user to select their preferred map application,
     * allowing Organic Maps (if installed and configured) to be an option.
     *
     * @param context The context to start the activity.
     * @param url The map URL to open.
     */
    public static void openMapLink(Context context, String url) {
        Uri parsedUri = Uri.parse(url);
        Uri geoUri = null;
        String label = null;
        double latitude = 0;
        double longitude = 0;
        boolean coordinatesFound = false;

        // 1. Check if the URL is already a geo: URI
        if (parsedUri.getScheme() != null && parsedUri.getScheme().equals("geo")) {
            geoUri = parsedUri;
            coordinatesFound = true; // No further coordinate parsing needed if already geo:
        } else {
            // 2. Attempt to parse common web map links to extract coordinates and construct a geo: URI.

            // Google Maps links: Handles various Google Maps domains including short URLs
            if (url.contains("google.com/maps") || url.contains("goo.gl/maps") || url.contains("maps.app.goo.gl")) {
                Matcher matcher;

                // Priority 1: Try to find a label along with coordinates (q=label@lat,lon)
                matcher = GOOGLE_MAPS_Q_COORD_LABEL_PATTERN.matcher(url);
                if (matcher.find()) {
                    label = Uri.decode(matcher.group(1)); // Decode URL-encoded label
                    try {
                        latitude = Double.parseDouble(matcher.group(2));
                        longitude = Double.parseDouble(matcher.group(3));
                        coordinatesFound = true;
                    } catch (NumberFormatException e) {
                        Log.e(TAG, "NumberFormatException: Failed to parse Google Maps Q coord/label from URL: " + url + " - " + e.getMessage());
                    }
                }

                // Priority 2: If no label found, try q=lat,lon
                if (!coordinatesFound) {
                    matcher = GOOGLE_MAPS_Q_ONLY_COORD_PATTERN.matcher(url);
                    if (matcher.find()) {
                        try {
                            latitude = Double.parseDouble(matcher.group(1));
                            longitude = Double.parseDouble(matcher.group(2));
                            coordinatesFound = true;
                        } catch (NumberFormatException e) {
                            Log.e(TAG, "NumberFormatException: Failed to parse Google Maps Q only coord from URL: " + url + " - " + e.getMessage());
                        }
                    }
                }

                // Priority 3: As a last resort for Google Maps, try the general @lat,lon pattern
                if (!coordinatesFound) {
                    matcher = GOOGLE_MAPS_GENERAL_COORD_PATTERN.matcher(url);
                    if (matcher.find()) {
                        try {
                            latitude = Double.parseDouble(matcher.group(1));
                            longitude = Double.parseDouble(matcher.group(2));
                            coordinatesFound = true;
                        } catch (NumberFormatException e) {
                            Log.e(TAG, "NumberFormatException: Failed to parse Google Maps general coord from URL: " + url + " - " + e.getMessage());
                        }
                    }
                }
            }
            // OpenStreetMap links: Handles www.openstreetmap.org, openstreetmap.org, and short osm.org/go
            else if (url.contains("openstreetmap.org") || url.contains("osm.org/go")) {
                Matcher matcher = OSM_COORD_PATTERN.matcher(url);
                if (matcher.find()) {
                    try {
                        latitude = Double.parseDouble(matcher.group(1));
                        longitude = Double.parseDouble(matcher.group(2));
                        coordinatesFound = true;
                    } catch (NumberFormatException e) {
                        Log.e(TAG, "NumberFormatException: Failed to parse OpenStreetMap coord from URL: " + url + " - " + e.getMessage());
                    }
                }
            }
            // Apple Maps links
            else if (url.contains("maps.apple.com")) {
                Matcher matcher = APPLE_MAPS_COORD_PATTERN.matcher(url);
                if (matcher.find()) {
                    try {
                        latitude = Double.parseDouble(matcher.group(1));
                        longitude = Double.parseDouble(matcher.group(2));
                        coordinatesFound = true;
                    } catch (NumberFormatException e) {
                        Log.e(TAG, "NumberFormatException: Failed to parse Apple Maps coord from URL: " + url + " - " + e.getMessage());
                    }
                }
            }
            // For other map providers (e.g., 2GIS), if specific coordinate parsing is not implemented,
            // they will fall through to opening the original URL, which is a reasonable and safe fallback.

            // Construct geo URI if coordinates were successfully extracted
            if (coordinatesFound) {
                String geoString = String.format("geo:%f,%f", latitude, longitude);
                if (label != null && !label.isEmpty()) {
                    geoString += "?q=" + Uri.encode(label); // Encode label for geo URI
                }
                geoUri = Uri.parse(geoString);
            }
        }

        Intent intent;
        if (geoUri != null) {
            // If a geo URI was found or constructed, use it
            intent = new Intent(Intent.ACTION_VIEW, geoUri);
        } else {
            // Fallback to the original URL if coordinates couldn't be extracted or it's an unhandled map type
            intent = new Intent(Intent.ACTION_VIEW, parsedUri);
        }

        // Check if there are any activities that can handle this intent
        PackageManager packageManager = context.getPackageManager();
        List activities = packageManager.queryIntentActivities(intent, PackageManager.MATCH_DEFAULT_ONLY);
        boolean isIntentSafe = activities.size() > 0;

        if (isIntentSafe) {
            // Create a chooser intent to let the user pick their preferred map app.
            // This ensures Organic Maps (if installed and capable of handling geo: URIs or the web URL)
            // will be presented as an option alongside other map applications.
            Intent chooserIntent = Intent.createChooser(intent, "Open with:");
            context.startActivity(chooserIntent);
        } else {
            // Log a warning if no app can handle the intent. This is a system warning, not a personal testing log.
            Log.w(TAG, "No application found to handle map link: " + url);
            // In a production application, you might want to display a user-friendly message (e.g., a Toast)
            // through the calling activity/fragment.
        }
    }
}