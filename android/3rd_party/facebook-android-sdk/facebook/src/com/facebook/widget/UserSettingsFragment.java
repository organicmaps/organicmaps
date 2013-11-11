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

package com.facebook.widget;

import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import com.facebook.*;
import com.facebook.android.R;
import com.facebook.internal.AnalyticsEvents;
import com.facebook.internal.ImageDownloader;
import com.facebook.internal.ImageRequest;
import com.facebook.internal.ImageResponse;
import com.facebook.model.GraphUser;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Arrays;
import java.util.List;

/**
 * A Fragment that displays a Login/Logout button as well as the user's
 * profile picture and name when logged in.
 * <p/>
 * This Fragment will create and use the active session upon construction
 * if it has the available data (if the app ID is specified in the manifest).
 * It will also open the active session if it does not require user interaction
 * (i.e. if the session is in the {@link com.facebook.SessionState#CREATED_TOKEN_LOADED} state.
 * Developers can override the use of the active session by calling
 * the {@link #setSession(com.facebook.Session)} method.
 */
public class UserSettingsFragment extends FacebookFragment {

    private static final String NAME = "name";
    private static final String ID = "id";
    private static final String PICTURE = "picture";
    private static final String FIELDS = "fields";
    
    private static final String REQUEST_FIELDS = TextUtils.join(",", new String[] {ID, NAME, PICTURE});

    private LoginButton loginButton;
    private LoginButton.LoginButtonProperties loginButtonProperties = new LoginButton.LoginButtonProperties();
    private TextView connectedStateLabel;
    private GraphUser user;
    private Session userInfoSession; // the Session used to fetch the current user info
    private Drawable userProfilePic;
    private String userProfilePicID;
    private Session.StatusCallback sessionStatusCallback;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.com_facebook_usersettingsfragment, container, false);
        loginButton = (LoginButton) view.findViewById(R.id.com_facebook_usersettingsfragment_login_button);
        loginButton.setProperties(loginButtonProperties);
        loginButton.setFragment(this);
        loginButton.setLoginLogoutEventName(AnalyticsEvents.EVENT_USER_SETTINGS_USAGE);

        Session session = getSession();
        if (session != null && !session.equals(Session.getActiveSession())) {
            loginButton.setSession(session);
        }
        connectedStateLabel = (TextView) view.findViewById(R.id.com_facebook_usersettingsfragment_profile_name);
        
        // if no background is set for some reason, then default to Facebook blue
        if (view.getBackground() == null) {
            view.setBackgroundColor(getResources().getColor(R.color.com_facebook_blue));
        } else {
            view.getBackground().setDither(true);
        }
        return view;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRetainInstance(true);
    }

    /**
     * @throws com.facebook.FacebookException if errors occur during the loading of user information
     */
    @Override
    public void onResume() {
        super.onResume();
        fetchUserInfo();
        updateUI();
    }

    /**
     * Set the Session object to use instead of the active Session. Since a Session
     * cannot be reused, if the user logs out from this Session, and tries to
     * log in again, a new Active Session will be used instead.
     * <p/>
     * If the passed in session is currently opened, this method will also attempt to
     * load some user information for display (if needed).
     *
     * @param newSession the Session object to use
     * @throws com.facebook.FacebookException if errors occur during the loading of user information
     */
    @Override
    public void setSession(Session newSession) {
        super.setSession(newSession);
        if (loginButton != null) {
            loginButton.setSession(newSession);
        }
        fetchUserInfo();
        updateUI();
    }

    /**
     * Sets the default audience to use when the session is opened.
     * This value is only useful when specifying write permissions for the native
     * login dialog.
     *
     * @param defaultAudience the default audience value to use
     */
    public void setDefaultAudience(SessionDefaultAudience defaultAudience) {
        loginButtonProperties.setDefaultAudience(defaultAudience);
    }

    /**
     * Gets the default audience to use when the session is opened.
     * This value is only useful when specifying write permissions for the native
     * login dialog.
     *
     * @return the default audience value to use
     */
    public SessionDefaultAudience getDefaultAudience() {
        return loginButtonProperties.getDefaultAudience();
    }

    /**
     * Set the permissions to use when the session is opened. The permissions here
     * can only be read permissions. If any publish permissions are included, the login
     * attempt by the user will fail. The LoginButton can only be associated with either
     * read permissions or publish permissions, but not both. Calling both
     * setReadPermissions and setPublishPermissions on the same instance of LoginButton
     * will result in an exception being thrown unless clearPermissions is called in between.
     * <p/>
     * This method is only meaningful if called before the session is open. If this is called
     * after the session is opened, and the list of permissions passed in is not a subset
     * of the permissions granted during the authorization, it will log an error.
     * <p/>
     * Since the session can be automatically opened when the UserSettingsFragment is constructed,
     * it's important to always pass in a consistent set of permissions to this method, or
     * manage the setting of permissions outside of the LoginButton class altogether
     * (by managing the session explicitly).
     *
     * @param permissions the read permissions to use
     *
     * @throws UnsupportedOperationException if setPublishPermissions has been called
     */
    public void setReadPermissions(List<String> permissions) {
        loginButtonProperties.setReadPermissions(permissions, getSession());
    }

    /**
     * Set the permissions to use when the session is opened. The permissions here
     * can only be read permissions. If any publish permissions are included, the login
     * attempt by the user will fail. The LoginButton can only be associated with either
     * read permissions or publish permissions, but not both. Calling both
     * setReadPermissions and setPublishPermissions on the same instance of LoginButton
     * will result in an exception being thrown unless clearPermissions is called in between.
     * <p/>
     * This method is only meaningful if called before the session is open. If this is called
     * after the session is opened, and the list of permissions passed in is not a subset
     * of the permissions granted during the authorization, it will log an error.
     * <p/>
     * Since the session can be automatically opened when the UserSettingsFragment is constructed,
     * it's important to always pass in a consistent set of permissions to this method, or
     * manage the setting of permissions outside of the LoginButton class altogether
     * (by managing the session explicitly).
     *
     * @param permissions the read permissions to use
     *
     * @throws UnsupportedOperationException if setPublishPermissions has been called
     */
    public void setReadPermissions(String... permissions) {
        loginButtonProperties.setReadPermissions(Arrays.asList(permissions), getSession());
    }

    /**
     * Set the permissions to use when the session is opened. The permissions here
     * should only be publish permissions. If any read permissions are included, the login
     * attempt by the user may fail. The LoginButton can only be associated with either
     * read permissions or publish permissions, but not both. Calling both
     * setReadPermissions and setPublishPermissions on the same instance of LoginButton
     * will result in an exception being thrown unless clearPermissions is called in between.
     * <p/>
     * This method is only meaningful if called before the session is open. If this is called
     * after the session is opened, and the list of permissions passed in is not a subset
     * of the permissions granted during the authorization, it will log an error.
     * <p/>
     * Since the session can be automatically opened when the LoginButton is constructed,
     * it's important to always pass in a consistent set of permissions to this method, or
     * manage the setting of permissions outside of the LoginButton class altogether
     * (by managing the session explicitly).
     *
     * @param permissions the read permissions to use
     *
     * @throws UnsupportedOperationException if setReadPermissions has been called
     * @throws IllegalArgumentException if permissions is null or empty
     */
    public void setPublishPermissions(List<String> permissions) {
        loginButtonProperties.setPublishPermissions(permissions, getSession());
    }

    /**
     * Set the permissions to use when the session is opened. The permissions here
     * should only be publish permissions. If any read permissions are included, the login
     * attempt by the user may fail. The LoginButton can only be associated with either
     * read permissions or publish permissions, but not both. Calling both
     * setReadPermissions and setPublishPermissions on the same instance of LoginButton
     * will result in an exception being thrown unless clearPermissions is called in between.
     * <p/>
     * This method is only meaningful if called before the session is open. If this is called
     * after the session is opened, and the list of permissions passed in is not a subset
     * of the permissions granted during the authorization, it will log an error.
     * <p/>
     * Since the session can be automatically opened when the LoginButton is constructed,
     * it's important to always pass in a consistent set of permissions to this method, or
     * manage the setting of permissions outside of the LoginButton class altogether
     * (by managing the session explicitly).
     *
     * @param permissions the read permissions to use
     *
     * @throws UnsupportedOperationException if setReadPermissions has been called
     * @throws IllegalArgumentException if permissions is null or empty
     */
    public void setPublishPermissions(String... permissions) {
        loginButtonProperties.setPublishPermissions(Arrays.asList(permissions), getSession());
    }


    /**
     * Clears the permissions currently associated with this LoginButton.
     */
    public void clearPermissions() {
        loginButtonProperties.clearPermissions();
    }

    /**
     * Sets the login behavior for the session that will be opened. If null is specified,
     * the default ({@link SessionLoginBehavior SessionLoginBehavior.SSO_WITH_FALLBACK}
     * will be used.
     *
     * @param loginBehavior The {@link SessionLoginBehavior SessionLoginBehavior} that
     *                      specifies what behaviors should be attempted during
     *                      authorization.
     */
    public void setLoginBehavior(SessionLoginBehavior loginBehavior) {
        loginButtonProperties.setLoginBehavior(loginBehavior);
    }

    /**
     * Gets the login behavior for the session that will be opened. If null is returned,
     * the default ({@link SessionLoginBehavior SessionLoginBehavior.SSO_WITH_FALLBACK}
     * will be used.
     *
     * @return loginBehavior The {@link SessionLoginBehavior SessionLoginBehavior} that
     *                      specifies what behaviors should be attempted during
     *                      authorization.
     */
    public SessionLoginBehavior getLoginBehavior() {
        return loginButtonProperties.getLoginBehavior();
    }

    /**
     * Sets an OnErrorListener for this instance of UserSettingsFragment to call into when
     * certain exceptions occur.
     *
     * @param onErrorListener The listener object to set
     */
    public void setOnErrorListener(LoginButton.OnErrorListener onErrorListener) {
        loginButtonProperties.setOnErrorListener(onErrorListener);
    }

    /**
     * Returns the current OnErrorListener for this instance of UserSettingsFragment.
     *
     * @return The OnErrorListener
     */
    public LoginButton.OnErrorListener getOnErrorListener() {
        return loginButtonProperties.getOnErrorListener();
    }

    /**
     * Sets the callback interface that will be called whenever the status of the Session
     * associated with this LoginButton changes.
     *
     * @param callback the callback interface
     */
    public void setSessionStatusCallback(Session.StatusCallback callback) {
        this.sessionStatusCallback = callback;
    }

    /**
     * Sets the callback interface that will be called whenever the status of the Session
     * associated with this LoginButton changes.

     * @return the callback interface
     */
    public Session.StatusCallback getSessionStatusCallback() {
        return sessionStatusCallback;
    }

    @Override
    protected void onSessionStateChange(SessionState state, Exception exception) {
        fetchUserInfo();
        updateUI();

        if (sessionStatusCallback != null) {
            sessionStatusCallback.call(getSession(), state, exception);
        }
    }

    // For Testing Only
    List<String> getPermissions() {
        return loginButtonProperties.getPermissions();
    }

    private void fetchUserInfo() {
        final Session currentSession = getSession();
        if (currentSession != null && currentSession.isOpened()) {
            if (currentSession != userInfoSession) {
                Request request = Request.newMeRequest(currentSession, new Request.GraphUserCallback() {
                    @Override
                    public void onCompleted(GraphUser me, Response response) {
                        if (currentSession == getSession()) {
                            user = me;
                            updateUI();
                        }
                        if (response.getError() != null) {
                            loginButton.handleError(response.getError().getException());
                        }
                    }
                });
                Bundle parameters = new Bundle();
                parameters.putString(FIELDS, REQUEST_FIELDS);
                request.setParameters(parameters);
                Request.executeBatchAsync(request);
                userInfoSession = currentSession;
            }
        } else {
            user = null;
        }
    }
    
    private void updateUI() {
        if (!isAdded()) {
            return;
        }
        if (isSessionOpen()) {
            connectedStateLabel.setTextColor(getResources().getColor(R.color.com_facebook_usersettingsfragment_connected_text_color));
            connectedStateLabel.setShadowLayer(1f, 0f, -1f,
                    getResources().getColor(R.color.com_facebook_usersettingsfragment_connected_shadow_color));
            
            if (user != null) {
                ImageRequest request = getImageRequest();
                if (request != null) {
                    URI requestUrl = request.getImageUri();
                    // Do we already have the right picture? If so, leave it alone.
                    if (!requestUrl.equals(connectedStateLabel.getTag())) {
                        if (user.getId().equals(userProfilePicID)) {
                            connectedStateLabel.setCompoundDrawables(null, userProfilePic, null, null);
                            connectedStateLabel.setTag(requestUrl);
                        } else {
                            ImageDownloader.downloadAsync(request);
                        }
                    }
                }
                connectedStateLabel.setText(user.getName());
            } else {
                connectedStateLabel.setText(getResources().getString(
                        R.string.com_facebook_usersettingsfragment_logged_in));
                Drawable noProfilePic = getResources().getDrawable(R.drawable.com_facebook_profile_default_icon);
                noProfilePic.setBounds(0, 0,
                        getResources().getDimensionPixelSize(R.dimen.com_facebook_usersettingsfragment_profile_picture_width),
                        getResources().getDimensionPixelSize(R.dimen.com_facebook_usersettingsfragment_profile_picture_height));
                connectedStateLabel.setCompoundDrawables(null, noProfilePic, null, null);
            }
        } else {
            int textColor = getResources().getColor(R.color.com_facebook_usersettingsfragment_not_connected_text_color);
            connectedStateLabel.setTextColor(textColor);
            connectedStateLabel.setShadowLayer(0f, 0f, 0f, textColor);
            connectedStateLabel.setText(getResources().getString(
                    R.string.com_facebook_usersettingsfragment_not_logged_in));
            connectedStateLabel.setCompoundDrawables(null, null, null, null);
            connectedStateLabel.setTag(null);
        }
    }

    private ImageRequest getImageRequest() {
        ImageRequest request = null;
        try {
            ImageRequest.Builder requestBuilder = new ImageRequest.Builder(
                    getActivity(),
                    ImageRequest.getProfilePictureUrl(
                            user.getId(),
                            getResources().getDimensionPixelSize(R.dimen.com_facebook_usersettingsfragment_profile_picture_width),
                            getResources().getDimensionPixelSize(R.dimen.com_facebook_usersettingsfragment_profile_picture_height)));

            request = requestBuilder.setCallerTag(this)
                    .setCallback(
                            new ImageRequest.Callback() {
                                @Override
                                public void onCompleted(ImageResponse response) {
                                    processImageResponse(user.getId(), response);
                                }
                            })
                    .build();
        } catch (URISyntaxException e) {
        }
        return request;
    }

    private void processImageResponse(String id, ImageResponse response) {
        if (response != null) {
            Bitmap bitmap = response.getBitmap();
            if (bitmap != null) {
                BitmapDrawable drawable = new BitmapDrawable(UserSettingsFragment.this.getResources(), bitmap);
                drawable.setBounds(0, 0,
                        getResources().getDimensionPixelSize(R.dimen.com_facebook_usersettingsfragment_profile_picture_width),
                        getResources().getDimensionPixelSize(R.dimen.com_facebook_usersettingsfragment_profile_picture_height));
                userProfilePic = drawable;
                userProfilePicID = id;
                connectedStateLabel.setCompoundDrawables(null, drawable, null, null);
                connectedStateLabel.setTag(response.getRequest().getImageUri());
            }
        }
    }
}
