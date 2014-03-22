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

import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.widget.Toast;
import com.facebook.FacebookException;
import com.facebook.widget.PickerFragment;
import com.facebook.widget.PlacePickerFragment;

// This class provides an example of an Activity that uses PlacePickerFragment to display a list of
// the places. It takes a layout-based approach to creating the PlacePickerFragment with the
// desired parameters -- see PickFriendActivity in the FriendPickerSample project for an example of an
// Activity creating a fragment (in this case a FriendPickerFragment) programmatically rather than
// via XML layout.
public class PickPlaceActivity extends FragmentActivity {
    PlacePickerFragment placePickerFragment;

    // A helper to simplify life for callers who want to populate a Bundle with the necessary
    // parameters. A more sophisticated Activity might define its own set of parameters; our needs
    // are simple, so we just populate what we want to pass to the PlacePickerFragment.
    public static void populateParameters(Intent intent, Location location, String searchText) {
        intent.putExtra(PlacePickerFragment.LOCATION_BUNDLE_KEY, location);
        intent.putExtra(PlacePickerFragment.SEARCH_TEXT_BUNDLE_KEY, searchText);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.pick_place_activity);

        FragmentManager fm = getSupportFragmentManager();
        placePickerFragment = (PlacePickerFragment) fm.findFragmentById(R.id.place_picker_fragment);
        if (savedInstanceState == null) {
            // If this is the first time we have created the fragment, update its properties based on
            // any parameters we received via our Intent.
            placePickerFragment.setSettingsFromBundle(getIntent().getExtras());
        }

        placePickerFragment.setOnErrorListener(new PickerFragment.OnErrorListener() {
            @Override
            public void onError(PickerFragment<?> fragment, FacebookException error) {
                PickPlaceActivity.this.onError(error);
            }
        });

        // We finish the activity when either the Done button is pressed or when a place is
        // selected (since only a single place can be selected).
        placePickerFragment.setOnSelectionChangedListener(new PickerFragment.OnSelectionChangedListener() {
            @Override
            public void onSelectionChanged(PickerFragment<?> fragment) {
                if (placePickerFragment.getSelection() != null) {
                    finishActivity();
                }
            }
        });
        placePickerFragment.setOnDoneButtonClickedListener(new PickerFragment.OnDoneButtonClickedListener() {
            @Override
            public void onDoneButtonClicked(PickerFragment<?> fragment) {
                finishActivity();
            }
        });
    }

    private void finishActivity() {
        // We just store our selection in the Application for other activities to look at.
        PlacePickerApplication application = (PlacePickerApplication) getApplication();
        application.setSelectedPlace(placePickerFragment.getSelection());

        setResult(RESULT_OK, null);
        finish();
    }

    private void onError(Exception error) {
        String text = getString(R.string.exception, error.getMessage());
        Toast toast = Toast.makeText(this, text, Toast.LENGTH_SHORT);
        toast.show();
    }

    @Override
    protected void onStart() {
        super.onStart();
        try {
            // Load data, unless a query has already taken place.
            placePickerFragment.loadData(false);
        } catch (Exception ex) {
            onError(ex);
        }
    }
}
