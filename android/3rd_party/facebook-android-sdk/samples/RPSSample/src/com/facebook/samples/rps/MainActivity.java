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

package com.facebook.samples.rps;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;
import com.facebook.*;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static com.facebook.samples.rps.RpsGameUtils.*;

public class MainActivity extends FragmentActivity {
    static final int RPS = 0;
    static final int SETTINGS = 1;
    static final int CONTENT = 2;
    static final int FRAGMENT_COUNT = CONTENT +1;

    private Fragment[] fragments = new Fragment[FRAGMENT_COUNT];
    private MenuItem settings;
    private MenuItem friends;
    private MenuItem share;
    private boolean isResumed = false;
    private UiLifecycleHelper uiHelper;
    private Session.StatusCallback callback = new Session.StatusCallback() {
        @Override
        public void call(Session session, SessionState state, Exception exception) {
            onSessionStateChange(session, state, exception);
        }
    };
    private boolean hasNativeLink = false;

    // the deep link url should be of the form http://some.path?fb_object_id=xxxxxxxxxx
    private Pattern deepLinkPattern = Pattern.compile(".*fb_object_id=(\\d*)");

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        uiHelper = new UiLifecycleHelper(this, callback);
        uiHelper.onCreate(savedInstanceState);

        setContentView(R.layout.main);

        FragmentManager fm = getSupportFragmentManager();
        fragments[RPS] = fm.findFragmentById(R.id.rps_fragment);
        fragments[SETTINGS] = fm.findFragmentById(R.id.settings_fragment);
        fragments[CONTENT] = fm.findFragmentById(R.id.content_fragment);

        FragmentTransaction transaction = fm.beginTransaction();
        for(int i = 0; i < fragments.length; i++) {
            transaction.hide(fragments[i]);
        }
        transaction.commit();

        hasNativeLink = handleNativeLink();
    }

    @Override
    public void onResume() {
        super.onResume();
        uiHelper.onResume();
        isResumed = true;

        // Call the 'activateApp' method to log an app event for use in analytics and advertising reporting.  Do so in
        // the onResume methods of the primary Activities that an app may be launched into.
        AppEventsLogger.activateApp(this);
    }

    @Override
    public void onPause() {
        super.onPause();
        uiHelper.onPause();
        isResumed = false;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        uiHelper.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        uiHelper.onDestroy();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        uiHelper.onSaveInstanceState(outState);
    }

    @Override
    protected void onResumeFragments() {
        super.onResumeFragments();

        if (hasNativeLink) {
            showFragment(CONTENT, false);
            hasNativeLink = false;
        } else {
            showFragment(RPS, false);
        }
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        // only add the menu when the selection fragment is showing
        if (fragments[RPS].isVisible()) {
            if (menu.size() == 0) {
                share = menu.add(R.string.share_on_facebook);
                friends = menu.add(R.string.see_friends);
                settings = menu.add(R.string.check_settings);
            }
            return true;
        } else {
            menu.clear();
            settings = null;
        }
        return false;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.equals(settings)) {
            showFragment(SETTINGS, true);
            return true;
        } else if (item.equals(friends)) {
            Intent intent = new Intent();
            intent.setClass(this, FriendActivity.class);
            startActivity(intent);
            return true;
        } else if (item.equals(share)) {
            RpsFragment fragment = (RpsFragment) fragments[RPS];
            fragment.shareUsingNativeDialog();
            return true;
        }
        return false;
    }

    private boolean handleNativeLink() {
        Session existingSession = Session.getActiveSession();
        // If we have a valid existing session, we'll use it; if not, open one using the provided Intent
        // but do not cache the token (we don't want to use the same user identity the next time the
        // app is run).
        if (existingSession == null || !existingSession.isOpened()) {
            AccessToken accessToken = AccessToken.createFromNativeLinkingIntent(getIntent());
            if (accessToken != null) {
                Session newSession = new Session.Builder(this).setTokenCachingStrategy(new NonCachingTokenCachingStrategy())
                        .build();
                newSession.open(accessToken, null);

                Session.setActiveSession(newSession);
            }
        }
        // See if we have a deep link in addition.
        int deepLinkContent = getDeepLinkContent(getIntent().getData());
        if (deepLinkContent != INVALID_CHOICE) {
            ContentFragment fragment = (ContentFragment) fragments[CONTENT];
            fragment.setContentIndex(deepLinkContent);
            return true;
        }
        return false;
    }

    private int getDeepLinkContent(Uri deepLinkUri) {
        if (deepLinkUri != null) {
            String deepLink = deepLinkUri.toString();

            Matcher matcher = deepLinkPattern.matcher(deepLink);
            if (matcher.matches()) {
                String objectId = matcher.group(1);
                for (int i = 0; i < CommonObjects.BUILT_IN_OPEN_GRAPH_OBJECTS.length; i++) {
                    if (CommonObjects.BUILT_IN_OPEN_GRAPH_OBJECTS[i].equals(objectId)) {
                        return i;
                    }
                }
            }
        }
        return INVALID_CHOICE;
    }

    private void onSessionStateChange(Session session, SessionState state, Exception exception) {
        if (isResumed) {
            if (exception != null && !(exception instanceof FacebookOperationCanceledException)) {
                Toast.makeText(this, R.string.login_error, Toast.LENGTH_SHORT).show();
                return;
            }

            if (session.isClosed()) {
                showFragment(RPS, false);
            }
        }
    }

    void showFragment(int fragmentIndex, boolean addToBackStack) {
        FragmentManager fm = getSupportFragmentManager();
        FragmentTransaction transaction = fm.beginTransaction();
        if (addToBackStack) {
            transaction.addToBackStack(null);
        } else {
            int backStackSize = fm.getBackStackEntryCount();
            for (int i = 0; i < backStackSize; i++) {
                fm.popBackStack();
            }
        }
        for (int i = 0; i < fragments.length; i++) {
            if (i == fragmentIndex) {
                transaction.show(fragments[i]);
            } else {
                transaction.hide(fragments[i]);
            }
        }
        transaction.commit();
    }
}
