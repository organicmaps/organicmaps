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

package com.facebook.samples.switchuser;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.view.MenuItem;
import com.facebook.*;
import com.facebook.model.GraphUser;

public class MainActivity extends FragmentActivity {

    private static final String SHOWING_SETTINGS_KEY = "Showing settings";
    private static final String TOKEN_CACHE_NAME_KEY = "TokenCacheName";

    private ProfileFragment profileFragment;
    private SettingsFragment settingsFragment;
    private boolean isShowingSettings;
    private Slot currentSlot;
    private Session currentSession;
    private Session.StatusCallback sessionStatusCallback;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        restoreFragments(savedInstanceState);

        sessionStatusCallback = new Session.StatusCallback() {
            @Override
            public void call(Session session, SessionState state, Exception exception) {
                onSessionStateChange(session, state, exception);
            }
        };

        if (savedInstanceState != null) {
            if (savedInstanceState.getBoolean(SHOWING_SETTINGS_KEY)) {
                showSettings();
            } else {
                showProfile();
            }

            SharedPreferencesTokenCachingStrategy restoredCache = new SharedPreferencesTokenCachingStrategy(
                    this,
                    savedInstanceState.getString(TOKEN_CACHE_NAME_KEY));
            currentSession = Session.restoreSession(
                    this,
                    restoredCache,
                    sessionStatusCallback,
                    savedInstanceState);
        } else {
            showProfile();
        }
    }

    @Override
    public void onBackPressed() {
        if (isShowingSettings()) {
            // This back is from the settings fragment
            showProfile();
        } else {
            // Allow the user to back out of the app as well.
            super.onBackPressed();
        }
    }


    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(SHOWING_SETTINGS_KEY, isShowingSettings());
        if (currentSlot != null) {
            outState.putString(TOKEN_CACHE_NAME_KEY, currentSlot.getTokenCacheName());
        }

        FragmentManager manager = getSupportFragmentManager();
        manager.putFragment(outState, SettingsFragment.TAG, settingsFragment);
        manager.putFragment(outState, ProfileFragment.TAG, profileFragment);

        Session.saveSession(currentSession, outState);
    }

    @Override
    protected void onResume() {
        super.onResume();

        settingsFragment.setSlotChangedListener(new SettingsFragment.OnSlotChangedListener() {
            @Override
            public void onSlotChanged(Slot newSlot) {
                handleSlotChange(newSlot);
            }
        });

        profileFragment.setOnOptionsItemSelectedListener(new ProfileFragment.OnOptionsItemSelectedListener() {
            @Override
            public boolean onOptionsItemSelected(MenuItem item) {
                return handleOptionsItemSelected(item);
            }
        });

        if (currentSession != null) {
            currentSession.addCallback(sessionStatusCallback);
        }

        // Call the 'activateApp' method to log an app event for use in analytics and advertising reporting.  Do so in
        // the onResume methods of the primary Activities that an app may be launched into.
        AppEventsLogger.activateApp(this);
    }

    @Override
    protected void onPause() {
        super.onPause();

        settingsFragment.setSlotChangedListener(null);
        profileFragment.setOnOptionsItemSelectedListener(null);

        if (currentSession != null) {
            currentSession.removeCallback(sessionStatusCallback);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (currentSession != null) {
            currentSession.onActivityResult(this, requestCode, resultCode, data);
        }
    }

    private void onSessionStateChange(Session session, SessionState state, Exception exception) {
        if (session != currentSession) {
            return;
        }

        if (state.isOpened()) {
            // Log in just happened.
            fetchUserInfo();
            showProfile();
        } else if (state.isClosed()) {
            // Log out just happened. Update the UI.
            updateFragments(null);
        }
    }

    private void restoreFragments(Bundle savedInstanceState) {
        FragmentManager manager = getSupportFragmentManager();
        FragmentTransaction transaction = manager.beginTransaction();

        if (savedInstanceState != null) {
            profileFragment = (ProfileFragment)manager.getFragment(savedInstanceState, ProfileFragment.TAG);
            settingsFragment = (SettingsFragment)manager.getFragment(savedInstanceState, SettingsFragment.TAG);
        }

        if (profileFragment == null) {
            profileFragment = new ProfileFragment();
            transaction.add(R.id.fragmentContainer, profileFragment, ProfileFragment.TAG);
        }

        if (settingsFragment == null) {
            settingsFragment = new SettingsFragment();
            transaction.add(R.id.fragmentContainer, settingsFragment, SettingsFragment.TAG);
        }

        transaction.commit();
    }

    private void showSettings() {
        isShowingSettings = true;

        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
        transaction.hide(profileFragment)
                .show(settingsFragment)
                .commit();
    }

    private boolean isShowingSettings() {
        return isShowingSettings;
    }

    private void showProfile() {
        isShowingSettings = false;

        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
        transaction.hide(settingsFragment)
                .show(profileFragment)
                .commit();
    }

    private void fetchUserInfo() {
        if (currentSession != null && currentSession.isOpened()) {
            Request request = Request.newMeRequest(currentSession, new Request.GraphUserCallback() {
                @Override
                public void onCompleted(GraphUser me, Response response) {
                    if (response.getRequest().getSession() == currentSession) {
                        updateFragments(me);
                    }
                }
            });
            request.executeAsync();
        }
    }

    private void handleSlotChange(Slot newSlot) {
        if (currentSession != null) {
            currentSession.close();
            currentSession = null;
        }

        if (newSlot != null) {
            currentSlot = newSlot;
            currentSession = new Session.Builder(this)
                    .setTokenCachingStrategy(currentSlot.getTokenCache())
                    .build();
            currentSession.addCallback(sessionStatusCallback);

            Session.OpenRequest openRequest = new Session.OpenRequest(this);
            openRequest.setLoginBehavior(newSlot.getLoginBehavior());
            openRequest.setRequestCode(Session.DEFAULT_AUTHORIZE_ACTIVITY_CODE);
            currentSession.openForRead(openRequest);
        }
    }

    private boolean handleOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_item_switch:
                showSettings();
                return true;
            default:
                return false;
        }
    }

    private void updateFragments(GraphUser user) {
        settingsFragment.updateViewForUser(user);
        profileFragment.updateViewForUser(user);
    }
}
