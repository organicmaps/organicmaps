/**
 * Copyright 2010-present Facebook.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.facebook.samples.placepicker;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import com.facebook.AppEventsLogger;
import com.facebook.Session;
import com.facebook.SessionState;
import com.facebook.UiLifecycleHelper;
import com.facebook.model.GraphLocation;
import com.facebook.model.GraphPlace;

public class PlacePickerSampleActivity extends FragmentActivity implements LocationListener {
    private static final int PLACE_ACTIVITY = 1;
    private static final Location SEATTLE_LOCATION = new Location("") {
        {
            setLatitude(47.6097);
            setLongitude(-122.3331);
        }
    };
    private static final Location SAN_FRANCISCO_LOCATION = new Location("") {
        {
            setLatitude(37.7750);
            setLongitude(-122.4183);
        }
    };
    private static final Location PARIS_LOCATION = new Location("") {
        {
            setLatitude(48.857875);
            setLongitude(2.294635);
        }
    };

    private TextView resultsTextView;
    private LocationManager locationManager;
    private Location lastKnownLocation;
    private UiLifecycleHelper lifecycleHelper;
    private Location pickPlaceForLocationWhenSessionOpened = null;


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        resultsTextView = (TextView) findViewById(R.id.resultsTextView);
        Button button = (Button) findViewById(R.id.seattleButton);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                onClickSeattle();
            }
        });

        button = (Button) findViewById(R.id.sanFranciscoButton);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                onClickSanFrancisco();
            }
        });

        button = (Button) findViewById(R.id.gpsButton);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                onClickGPS();
            }
        });

        lifecycleHelper = new UiLifecycleHelper(this, new Session.StatusCallback() {
            @Override
            public void call(Session session, SessionState state, Exception exception) {
                onSessionStateChanged(session, state, exception);
            }
        });
        lifecycleHelper.onCreate(savedInstanceState);

        ensureOpenSession();

        locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
    }

    private boolean ensureOpenSession() {
        if (Session.getActiveSession() == null ||
                !Session.getActiveSession().isOpened()) {
            Session.openActiveSession(this, true, new Session.StatusCallback() {
                @Override
                public void call(Session session, SessionState state, Exception exception) {
                    onSessionStateChanged(session, state, exception);
                }
            });
            return false;
        }
        return true;
    }

    @Override
    protected void onStart() {
        super.onStart();

        // Update the display every time we are started (this will be "no place selected" on first
        // run, or possibly details of a place if the activity is being re-created).
        displaySelectedPlace(RESULT_OK);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        lifecycleHelper.onDestroy();
    }

    @Override
    protected void onPause() {
        super.onPause();
        lifecycleHelper.onPause();

        // Call the 'deactivateApp' method to log an app event for use in analytics and advertising
        // reporting.  Do so in the onPause methods of the primary Activities that an app may be launched into.
        AppEventsLogger.deactivateApp(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        lifecycleHelper.onResume();

        // Call the 'activateApp' method to log an app event for use in analytics and advertising reporting.  Do so in
        // the onResume methods of the primary Activities that an app may be launched into.
        AppEventsLogger.activateApp(this);
    }

    private void onError(Exception exception) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Error").setMessage(exception.getMessage()).setPositiveButton("OK", null);
        builder.show();
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        lifecycleHelper.onActivityResult(requestCode, resultCode, data);

        switch (requestCode) {
            case PLACE_ACTIVITY:
                displaySelectedPlace(resultCode);
                break;
            default:
                break;
        }
    }

    private void onSessionStateChanged(Session session, SessionState state, Exception exception) {
        if (pickPlaceForLocationWhenSessionOpened != null && state.isOpened()) {
            Location location = pickPlaceForLocationWhenSessionOpened;
            pickPlaceForLocationWhenSessionOpened = null;
            startPickPlaceActivity(location);
        }
    }

    private void displaySelectedPlace(int resultCode) {
        String results = "";
        PlacePickerApplication application = (PlacePickerApplication) getApplication();

        GraphPlace selection = application.getSelectedPlace();
        if (selection != null) {
            GraphLocation location = selection.getLocation();

            results = String.format("Name: %s\nCategory: %s\nLocation: (%f,%f)\nStreet: %s, %s, %s, %s, %s",
                    selection.getName(), selection.getCategory(),
                    location.getLatitude(), location.getLongitude(),
                    location.getStreet(), location.getCity(), location.getState(), location.getZip(),
                    location.getCountry());
        } else {
            results = "<No place selected>";
        }

        resultsTextView.setText(results);
    }

    public void onLocationChanged(Location location) {
        lastKnownLocation = location;
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras) {
    }

    @Override
    public void onProviderEnabled(String provider) {
    }

    @Override
    public void onProviderDisabled(String provider) {
    }

    private void startPickPlaceActivity(Location location) {
        if (ensureOpenSession()) {
            PlacePickerApplication application = (PlacePickerApplication) getApplication();
            application.setSelectedPlace(null);

            Intent intent = new Intent(this, PickPlaceActivity.class);
            PickPlaceActivity.populateParameters(intent, location, null);

            startActivityForResult(intent, PLACE_ACTIVITY);
        } else {
            pickPlaceForLocationWhenSessionOpened = location;
        }
    }

    private void onClickSeattle() {
        try {
            startPickPlaceActivity(SEATTLE_LOCATION);
        } catch (Exception ex) {
            onError(ex);
        }
    }

    private void onClickSanFrancisco() {
        try {
            startPickPlaceActivity(SAN_FRANCISCO_LOCATION);
        } catch (Exception ex) {
            onError(ex);
        }
    }

    private void onClickGPS() {
        try {
            if (lastKnownLocation == null) {
                Criteria criteria = new Criteria();
                String bestProvider = locationManager.getBestProvider(criteria, false);
                if (bestProvider != null) {
                    lastKnownLocation = locationManager.getLastKnownLocation(bestProvider);
                }
            }
            if (lastKnownLocation == null) {
                String model = android.os.Build.MODEL;
                if (model.equals("sdk") || model.equals("google_sdk") || model.contains("x86")) {
                    // Looks like they are on an emulator, pretend we're in Paris if we don't have a
                    // location set.
                    lastKnownLocation = PARIS_LOCATION;
                } else {
                    onError(new Exception(getString(R.string.no_location)));
                    return;
                }
            }
            startPickPlaceActivity(lastKnownLocation);
        } catch (Exception ex) {
            onError(ex);
        }
    }

}
