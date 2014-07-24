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

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.net.http.SslError;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.*;
import android.webkit.SslErrorHandler;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import com.facebook.*;
import com.facebook.android.*;
import com.facebook.internal.Logger;
import com.facebook.internal.ServerProtocol;
import com.facebook.internal.Utility;
import com.facebook.internal.Validate;

/**
 * This class provides a mechanism for displaying Facebook Web dialogs inside a Dialog. Helper
 * methods are provided to construct commonly-used dialogs, or a caller can specify arbitrary
 * parameters to call other dialogs.
 */
public class WebDialog extends Dialog {
    private static final String LOG_TAG = Logger.LOG_TAG_BASE + "WebDialog";
    private static final String DISPLAY_TOUCH = "touch";
    private static final String USER_AGENT = "user_agent";
    static final String REDIRECT_URI = "fbconnect://success";
    static final String CANCEL_URI = "fbconnect://cancel";
    static final boolean DISABLE_SSL_CHECK_FOR_TESTING = false;

    // width below which there are no extra margins
    private static final int NO_PADDING_SCREEN_WIDTH = 480;
    // width beyond which we're always using the MIN_SCALE_FACTOR
    private static final int MAX_PADDING_SCREEN_WIDTH = 800;
    // height below which there are no extra margins
    private static final int NO_PADDING_SCREEN_HEIGHT = 800;
    // height beyond which we're always using the MIN_SCALE_FACTOR
    private static final int MAX_PADDING_SCREEN_HEIGHT = 1280;

    // the minimum scaling factor for the web dialog (50% of screen size)
    private static final double MIN_SCALE_FACTOR = 0.5;
    // translucent border around the webview
    private static final int BACKGROUND_GRAY = 0xCC000000;

    public static final int DEFAULT_THEME = android.R.style.Theme_Translucent_NoTitleBar;

    private String url;
    private OnCompleteListener onCompleteListener;
    private WebView webView;
    private ProgressDialog spinner;
    private ImageView crossImageView;
    private FrameLayout contentFrameLayout;
    private boolean listenerCalled = false;
    private boolean isDetached = false;

    /**
     * Interface that implements a listener to be called when the user's interaction with the
     * dialog completes, whether because the dialog finished successfully, or it was cancelled,
     * or an error was encountered.
     */
    public interface OnCompleteListener {
        /**
         * Called when the dialog completes.
         *
         * @param values on success, contains the values returned by the dialog
         * @param error  on an error, contains an exception describing the error
         */
        void onComplete(Bundle values, FacebookException error);
    }

    /**
     * Constructor which can be used to display a dialog with an already-constructed URL.
     *
     * @param context the context to use to display the dialog
     * @param url     the URL of the Web Dialog to display; no validation is done on this URL, but it should
     *                be a valid URL pointing to a Facebook Web Dialog
     */
    public WebDialog(Context context, String url) {
        this(context, url, DEFAULT_THEME);
    }

    /**
     * Constructor which can be used to display a dialog with an already-constructed URL and a custom theme.
     *
     * @param context the context to use to display the dialog
     * @param url     the URL of the Web Dialog to display; no validation is done on this URL, but it should
     *                be a valid URL pointing to a Facebook Web Dialog
     * @param theme   identifier of a theme to pass to the Dialog class
     */
    public WebDialog(Context context, String url, int theme) {
        super(context, theme);
        this.url = url;
    }

    /**
     * Constructor which will construct the URL of the Web dialog based on the specified parameters.
     *
     * @param context    the context to use to display the dialog
     * @param action     the portion of the dialog URL following "dialog/"
     * @param parameters parameters which will be included as part of the URL
     * @param theme      identifier of a theme to pass to the Dialog class
     * @param listener the listener to notify, or null if no notification is desired
     */
    public WebDialog(Context context, String action, Bundle parameters, int theme, OnCompleteListener listener) {
        super(context, theme);

        if (parameters == null) {
            parameters = new Bundle();
        }

        // our webview client only handles the redirect uri we specify, so just hard code it here
        parameters.putString(ServerProtocol.DIALOG_PARAM_REDIRECT_URI, REDIRECT_URI);

        parameters.putString(ServerProtocol.DIALOG_PARAM_DISPLAY, DISPLAY_TOUCH);

        Uri uri = Utility.buildUri(
                ServerProtocol.getDialogAuthority(),
                ServerProtocol.getAPIVersion() + "/" + ServerProtocol.DIALOG_PATH + action,
                parameters);
        this.url = uri.toString();
        onCompleteListener = listener;
    }

    /**
     * Sets the listener which will be notified when the dialog finishes.
     *
     * @param listener the listener to notify, or null if no notification is desired
     */
    public void setOnCompleteListener(OnCompleteListener listener) {
        onCompleteListener = listener;
    }

    /**
     * Gets the listener which will be notified when the dialog finishes.
     *
     * @return the listener, or null if none has been specified
     */
    public OnCompleteListener getOnCompleteListener() {
        return onCompleteListener;
    }

    @Override
    public void dismiss() {
        if (webView != null) {
            webView.stopLoading();
        }
        if (!isDetached) {
            if (spinner.isShowing()) {
                spinner.dismiss();
            }
            super.dismiss();
        }
    }

    @Override
    public void onDetachedFromWindow() {
        isDetached = true;
        super.onDetachedFromWindow();
    }

    @Override
    public void onAttachedToWindow() {
        isDetached = false;
        super.onAttachedToWindow();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialogInterface) {
                sendCancelToListener();
            }
        });

        spinner = new ProgressDialog(getContext());
        spinner.requestWindowFeature(Window.FEATURE_NO_TITLE);
        spinner.setMessage(getContext().getString(R.string.com_facebook_loading));
        spinner.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialogInterface) {
                sendCancelToListener();
                WebDialog.this.dismiss();
            }
        });

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        contentFrameLayout = new FrameLayout(getContext());

        // First calculate how big the frame layout should be
        calculateSize();
        getWindow().setGravity(Gravity.CENTER);

        // resize the dialog if the soft keyboard comes up
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);

        /* Create the 'x' image, but don't add to the contentFrameLayout layout yet
         * at this point, we only need to know its drawable width and height
         * to place the webview
         */
        createCrossImage();

        /* Now we know 'x' drawable width and height,
         * layout the webview and add it the contentFrameLayout layout
         */
        int crossWidth = crossImageView.getDrawable().getIntrinsicWidth();

        setUpWebView(crossWidth / 2 + 1);

        /* Finally add the 'x' image to the contentFrameLayout layout and
        * add contentFrameLayout to the Dialog view
        */
        contentFrameLayout.addView(crossImageView, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        setContentView(contentFrameLayout);
    }

    private void calculateSize() {
        WindowManager wm = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        DisplayMetrics metrics = new DisplayMetrics();
        display.getMetrics(metrics);
        // always use the portrait dimensions to do the scaling calculations so we always get a portrait shaped
        // web dialog
        int width = metrics.widthPixels < metrics.heightPixels ? metrics.widthPixels : metrics.heightPixels;
        int height = metrics.widthPixels < metrics.heightPixels ? metrics.heightPixels : metrics.widthPixels;

        int dialogWidth = Math.min(
                getScaledSize(width, metrics.density, NO_PADDING_SCREEN_WIDTH, MAX_PADDING_SCREEN_WIDTH),
                metrics.widthPixels);
        int dialogHeight = Math.min(
                getScaledSize(height, metrics.density, NO_PADDING_SCREEN_HEIGHT, MAX_PADDING_SCREEN_HEIGHT),
                metrics.heightPixels);

        getWindow().setLayout(dialogWidth, dialogHeight);
    }

    /**
     * Returns a scaled size (either width or height) based on the parameters passed.
     * @param screenSize a pixel dimension of the screen (either width or height)
     * @param density density of the screen
     * @param noPaddingSize the size at which there's no padding for the dialog
     * @param maxPaddingSize the size at which to apply maximum padding for the dialog
     * @return a scaled size.
     */
    private int getScaledSize(int screenSize, float density, int noPaddingSize, int maxPaddingSize) {
        int scaledSize = (int) ((float) screenSize / density);
        double scaleFactor;
        if (scaledSize <= noPaddingSize) {
            scaleFactor = 1.0;
        } else if (scaledSize >= maxPaddingSize) {
            scaleFactor = MIN_SCALE_FACTOR;
        } else {
            // between the noPadding and maxPadding widths, we take a linear reduction to go from 100%
            // of screen size down to MIN_SCALE_FACTOR
            scaleFactor = MIN_SCALE_FACTOR +
                    ((double) (maxPaddingSize - scaledSize))
                            / ((double) (maxPaddingSize - noPaddingSize))
                            * (1.0 - MIN_SCALE_FACTOR);
        }
        return (int) (screenSize * scaleFactor);
    }

    private void sendSuccessToListener(Bundle values) {
        if (onCompleteListener != null && !listenerCalled) {
            listenerCalled = true;
            onCompleteListener.onComplete(values, null);
        }
    }

    private void sendErrorToListener(Throwable error) {
        if (onCompleteListener != null && !listenerCalled) {
            listenerCalled = true;
            FacebookException facebookException = null;
            if (error instanceof FacebookException) {
                facebookException = (FacebookException) error;
            } else {
                facebookException = new FacebookException(error);
            }
            onCompleteListener.onComplete(null, facebookException);
        }
    }

    private void sendCancelToListener() {
        sendErrorToListener(new FacebookOperationCanceledException());
    }

    private void createCrossImage() {
        crossImageView = new ImageView(getContext());
        // Dismiss the dialog when user click on the 'x'
        crossImageView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                sendCancelToListener();
                WebDialog.this.dismiss();
            }
        });
        Drawable crossDrawable = getContext().getResources().getDrawable(R.drawable.com_facebook_close);
        crossImageView.setImageDrawable(crossDrawable);
        /* 'x' should not be visible while webview is loading
         * make it visible only after webview has fully loaded
         */
        crossImageView.setVisibility(View.INVISIBLE);
    }

    @SuppressLint("SetJavaScriptEnabled")
    private void setUpWebView(int margin) {
        LinearLayout webViewContainer = new LinearLayout(getContext());
        webView = new WebView(getContext());
        webView.setVerticalScrollBarEnabled(false);
        webView.setHorizontalScrollBarEnabled(false);
        webView.setWebViewClient(new DialogWebViewClient());
        webView.getSettings().setJavaScriptEnabled(true);
        webView.loadUrl(url);
        webView.setLayoutParams(new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        webView.setVisibility(View.INVISIBLE);
        webView.getSettings().setSavePassword(false);
        webView.getSettings().setSaveFormData(false);

        webViewContainer.setPadding(margin, margin, margin, margin);
        webViewContainer.addView(webView);
        webViewContainer.setBackgroundColor(BACKGROUND_GRAY);
        contentFrameLayout.addView(webViewContainer);
    }

    private class DialogWebViewClient extends WebViewClient {
        @Override
        @SuppressWarnings("deprecation")
        public boolean shouldOverrideUrlLoading(WebView view, String url) {
            Utility.logd(LOG_TAG, "Redirect URL: " + url);
            if (url.startsWith(WebDialog.REDIRECT_URI)) {
                Bundle values = Util.parseUrl(url);

                String error = values.getString("error");
                if (error == null) {
                    error = values.getString("error_type");
                }

                String errorMessage = values.getString("error_msg");
                if (errorMessage == null) {
                    errorMessage = values.getString("error_description");
                }
                String errorCodeString = values.getString("error_code");
                int errorCode = FacebookRequestError.INVALID_ERROR_CODE;
                if (!Utility.isNullOrEmpty(errorCodeString)) {
                    try {
                        errorCode = Integer.parseInt(errorCodeString);
                    } catch (NumberFormatException ex) {
                        errorCode = FacebookRequestError.INVALID_ERROR_CODE;
                    }
                }

                if (Utility.isNullOrEmpty(error) && Utility
                        .isNullOrEmpty(errorMessage) && errorCode == FacebookRequestError.INVALID_ERROR_CODE) {
                    sendSuccessToListener(values);
                } else if (error != null && (error.equals("access_denied") ||
                        error.equals("OAuthAccessDeniedException"))) {
                    sendCancelToListener();
                } else {
                    FacebookRequestError requestError = new FacebookRequestError(errorCode, error, errorMessage);
                    sendErrorToListener(new FacebookServiceException(requestError, errorMessage));
                }

                WebDialog.this.dismiss();
                return true;
            } else if (url.startsWith(WebDialog.CANCEL_URI)) {
                sendCancelToListener();
                WebDialog.this.dismiss();
                return true;
            } else if (url.contains(DISPLAY_TOUCH)) {
                return false;
            }
            // launch non-dialog URLs in a full browser
            getContext().startActivity(
                    new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
            return true;
        }

        @Override
        public void onReceivedError(WebView view, int errorCode,
                String description, String failingUrl) {
            super.onReceivedError(view, errorCode, description, failingUrl);
            sendErrorToListener(new FacebookDialogException(description, errorCode, failingUrl));
            WebDialog.this.dismiss();
        }

        @Override
        public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error) {
            if (DISABLE_SSL_CHECK_FOR_TESTING) {
                handler.proceed();
            } else {
                super.onReceivedSslError(view, handler, error);

                sendErrorToListener(new FacebookDialogException(null, ERROR_FAILED_SSL_HANDSHAKE, null));
                handler.cancel();
                WebDialog.this.dismiss();
            }
        }

        @Override
        public void onPageStarted(WebView view, String url, Bitmap favicon) {
            Utility.logd(LOG_TAG, "Webview loading URL: " + url);
            super.onPageStarted(view, url, favicon);
            if (!isDetached) {
                spinner.show();
            }
        }

        @Override
        public void onPageFinished(WebView view, String url) {
            super.onPageFinished(view, url);
            if (!isDetached) {
                spinner.dismiss();
            }
            /*
             * Once web view is fully loaded, set the contentFrameLayout background to be transparent
             * and make visible the 'x' image.
             */
            contentFrameLayout.setBackgroundColor(Color.TRANSPARENT);
            webView.setVisibility(View.VISIBLE);
            crossImageView.setVisibility(View.VISIBLE);
        }
    }

    private static class BuilderBase<CONCRETE extends BuilderBase<?>> {
        private Context context;
        private Session session;
        private String applicationId;
        private String action;
        private int theme = DEFAULT_THEME;
        private OnCompleteListener listener;
        private Bundle parameters;

        protected BuilderBase(Context context, String action) {
            Session activeSession = Session.getActiveSession();
            if (activeSession != null && activeSession.isOpened()) {
                this.session = activeSession;
            } else {
                String applicationId = Utility.getMetadataApplicationId(context);
                if (applicationId != null) {
                    this.applicationId = applicationId;
                } else {
                    throw new FacebookException("Attempted to create a builder without an open" +
                            " Active Session or a valid default Application ID.");
                }
            }
            finishInit(context, action, null);
        }

        protected BuilderBase(Context context, Session session, String action, Bundle parameters) {
            Validate.notNull(session, "session");
            if (!session.isOpened()) {
                throw new FacebookException("Attempted to use a Session that was not open.");
            }
            this.session = session;

            finishInit(context, action, parameters);
        }

        protected BuilderBase(Context context, String applicationId, String action, Bundle parameters) {
            if (applicationId == null) {
                applicationId = Utility.getMetadataApplicationId(context);
            }
            Validate.notNullOrEmpty(applicationId, "applicationId");
            this.applicationId = applicationId;

            finishInit(context, action, parameters);
        }

        /**
         * Sets a theme identifier which will be passed to the underlying Dialog.
         *
         * @param theme a theme identifier which will be passed to the Dialog class
         * @return the builder
         */
        public CONCRETE setTheme(int theme) {
            this.theme = theme;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the listener which will be notified when the dialog finishes.
         *
         * @param listener the listener to notify, or null if no notification is desired
         * @return the builder
         */
        public CONCRETE setOnCompleteListener(OnCompleteListener listener) {
            this.listener = listener;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Constructs a WebDialog using the parameters provided. The dialog is not shown,
         * but is ready to be shown by calling Dialog.show().
         *
         * @return the WebDialog
         */
        public WebDialog build() {
            if (session != null && session.isOpened()) {
                parameters.putString(ServerProtocol.DIALOG_PARAM_APP_ID, session.getApplicationId());
                parameters.putString(ServerProtocol.DIALOG_PARAM_ACCESS_TOKEN, session.getAccessToken());
            } else {
                parameters.putString(ServerProtocol.DIALOG_PARAM_APP_ID, applicationId);
            }

            return new WebDialog(context, action, parameters, theme, listener);
        }

        protected String getApplicationId() {
            return applicationId;
        }

        protected Context getContext() {
            return context;
        }

        protected int getTheme() {
            return theme;
        }

        protected Bundle getParameters() {
            return parameters;
        }

        protected WebDialog.OnCompleteListener getListener() {
            return listener;
        }

        private void finishInit(Context context, String action, Bundle parameters) {
            this.context = context;
            this.action = action;
            if (parameters != null) {
                this.parameters = parameters;
            } else {
                this.parameters = new Bundle();
            }
        }
    }

    /**
     * Provides a builder that allows construction of an arbitary Facebook web dialog.
     */
    public static class Builder extends BuilderBase<Builder> {
        /**
         * Constructor that builds a dialog using either the active session, or the application
         * id specified in the application/meta-data.
         *
         * @param context the Context within which the dialog will be shown.
         * @param action the portion of the dialog URL following www.facebook.com/dialog/.
         *               See https://developers.facebook.com/docs/reference/dialogs/ for details.
         */
        public Builder(Context context, String action) {
            super(context, action);
        }

        /**
         * Constructor that builds a dialog for an authenticated user.
         *
         * @param context the Context within which the dialog will be shown.
         * @param session the Session representing an authenticating user to use for
         *                showing the dialog; must not be null, and must be opened.
         * @param action the portion of the dialog URL following www.facebook.com/dialog/.
         *               See https://developers.facebook.com/docs/reference/dialogs/ for details.
         * @param parameters a Bundle containing parameters to pass as part of the URL.
         */
        public Builder(Context context, Session session, String action, Bundle parameters) {
            super(context, session, action, parameters);
        }

        /**
         * Constructor that builds a dialog without an authenticated user.
         *
         * @param context the Context within which the dialog will be shown.
         * @param applicationId the application ID to be included in the dialog URL.
         * @param action the portion of the dialog URL following www.facebook.com/dialog/.
         *               See https://developers.facebook.com/docs/reference/dialogs/ for details.
         * @param parameters a Bundle containing parameters to pass as part of the URL.
         */
        public Builder(Context context, String applicationId, String action, Bundle parameters) {
            super(context, applicationId, action, parameters);
        }
    }

    /**
     * Provides a builder that allows construction of the parameters for showing
     * the <a href="https://developers.facebook.com/docs/reference/dialogs/feed">Feed Dialog</a>.
     */
    public static class FeedDialogBuilder extends BuilderBase<FeedDialogBuilder> {
        private static final String FEED_DIALOG = "feed";
        private static final String FROM_PARAM = "from";
        private static final String TO_PARAM = "to";
        private static final String LINK_PARAM = "link";
        private static final String PICTURE_PARAM = "picture";
        private static final String SOURCE_PARAM = "source";
        private static final String NAME_PARAM = "name";
        private static final String CAPTION_PARAM = "caption";
        private static final String DESCRIPTION_PARAM = "description";

        /**
         * Constructor that builds a Feed Dialog using either the active session, or the application
         * ID specified in the application/meta-data.
         *
         * @param context the Context within which the dialog will be shown.
         */
        public FeedDialogBuilder(Context context) {
            super(context, FEED_DIALOG);
        }

        /**
         * Constructor that builds a Feed Dialog using the provided session.
         *
         * @param context the Context within which the dialog will be shown.
         * @param session the Session representing an authenticating user to use for
         *                showing the dialog; must not be null, and must be opened.
         */
        public FeedDialogBuilder(Context context, Session session) {
            super(context, session, FEED_DIALOG, null);
        }

        /**
         * Constructor that builds a Feed Dialog using the provided session and parameters.
         *
         * @param context    the Context within which the dialog will be shown.
         * @param session    the Session representing an authenticating user to use for
         *                   showing the dialog; must not be null, and must be opened.
         * @param parameters a Bundle containing parameters to pass as part of the
         *                   dialog URL. No validation is done on these parameters; it is
         *                   the caller's responsibility to ensure they are valid. For more information,
         *                   see <a href="https://developers.facebook.com/docs/reference/dialogs/feed/">
         *                   https://developers.facebook.com/docs/reference/dialogs/feed/</a>.
         */
        public FeedDialogBuilder(Context context, Session session, Bundle parameters) {
            super(context, session, FEED_DIALOG, parameters);
        }

        /**
         * Constructor that builds a Feed Dialog using the provided application ID and parameters.
         *
         * @param context       the Context within which the dialog will be shown.
         * @param applicationId the application ID to use. If null, the application ID specified in the
         *                      application/meta-data will be used instead.
         * @param parameters    a Bundle containing parameters to pass as part of the
         *                      dialog URL. No validation is done on these parameters; it is
         *                      the caller's responsibility to ensure they are valid. For more information,
         *                      see <a href="https://developers.facebook.com/docs/reference/dialogs/feed/">
         *                      https://developers.facebook.com/docs/reference/dialogs/feed/</a>.
         */
        public FeedDialogBuilder(Context context, String applicationId, Bundle parameters) {
            super(context, applicationId, FEED_DIALOG, parameters);
        }

        /**
         * Sets the ID of the profile that is posting to Facebook. If none is specified,
         * the default is "me". This profile must be either the authenticated user or a
         * Page that the user is an administrator of.
         *
         * @param id Facebook ID of the profile to post from
         * @return the builder
         */
        public FeedDialogBuilder setFrom(String id) {
            getParameters().putString(FROM_PARAM, id);
            return this;
        }

        /**
         * Sets the ID of the profile that the story will be published to. If not specified, it
         * will default to the same profile that the story is being published from. The ID must be a friend who also
         * uses your app.
         *
         * @param id Facebook ID of the profile to post to
         * @return the builder
         */
        public FeedDialogBuilder setTo(String id) {
            getParameters().putString(TO_PARAM, id);
            return this;
        }

        /**
         * Sets the URL of a link to be shared.
         *
         * @param link the URL
         * @return the builder
         */
        public FeedDialogBuilder setLink(String link) {
            getParameters().putString(LINK_PARAM, link);
            return this;
        }

        /**
         * Sets the URL of a picture to be shared.
         *
         * @param picture the URL of the picture
         * @return the builder
         */
        public FeedDialogBuilder setPicture(String picture) {
            getParameters().putString(PICTURE_PARAM, picture);
            return this;
        }

        /**
         * Sets the URL of a media file attached to this post. If this is set, any picture
         * set via setPicture will be ignored.
         *
         * @param source the URL of the media file
         * @return the builder
         */
        public FeedDialogBuilder setSource(String source) {
            getParameters().putString(SOURCE_PARAM, source);
            return this;
        }

        /**
         * Sets the name of the item being shared.
         *
         * @param name the name
         * @return the builder
         */
        public FeedDialogBuilder setName(String name) {
            getParameters().putString(NAME_PARAM, name);
            return this;
        }

        /**
         * Sets the caption to be displayed.
         *
         * @param caption the caption
         * @return the builder
         */
        public FeedDialogBuilder setCaption(String caption) {
            getParameters().putString(CAPTION_PARAM, caption);
            return this;
        }

        /**
         * Sets the description to be displayed.
         *
         * @param description the description
         * @return the builder
         */
        public FeedDialogBuilder setDescription(String description) {
            getParameters().putString(DESCRIPTION_PARAM, description);
            return this;
        }
    }

    /**
     * Provides a builder that allows construction of the parameters for showing
     * the <a href="https://developers.facebook.com/docs/reference/dialogs/requests">Requests Dialog</a>.
     */
    public static class RequestsDialogBuilder extends BuilderBase<RequestsDialogBuilder> {
        private static final String APPREQUESTS_DIALOG = "apprequests";
        private static final String MESSAGE_PARAM = "message";
        private static final String TO_PARAM = "to";
        private static final String DATA_PARAM = "data";
        private static final String TITLE_PARAM = "title";

        /**
         * Constructor that builds a Requests Dialog using either the active session, or the application
         * ID specified in the application/meta-data.
         *
         * @param context the Context within which the dialog will be shown.
         */
        public RequestsDialogBuilder(Context context) {
            super(context, APPREQUESTS_DIALOG);
        }

        /**
         * Constructor that builds a Requests Dialog using the provided session.
         *
         * @param context the Context within which the dialog will be shown.
         * @param session the Session representing an authenticating user to use for
         *                showing the dialog; must not be null, and must be opened.
         */
        public RequestsDialogBuilder(Context context, Session session) {
            super(context, session, APPREQUESTS_DIALOG, null);
        }

        /**
         * Constructor that builds a Requests Dialog using the provided session and parameters.
         *
         * @param context    the Context within which the dialog will be shown.
         * @param session    the Session representing an authenticating user to use for
         *                   showing the dialog; must not be null, and must be opened.
         * @param parameters a Bundle containing parameters to pass as part of the
         *                   dialog URL. No validation is done on these parameters; it is
         *                   the caller's responsibility to ensure they are valid. For more information,
         *                   see <a href="https://developers.facebook.com/docs/reference/dialogs/requests/">
         *                   https://developers.facebook.com/docs/reference/dialogs/requests/</a>.
         */
        public RequestsDialogBuilder(Context context, Session session, Bundle parameters) {
            super(context, session, APPREQUESTS_DIALOG, parameters);
        }

        /**
         * Constructor that builds a Requests Dialog using the provided application ID and parameters.
         *
         * @param context       the Context within which the dialog will be shown.
         * @param applicationId the application ID to use. If null, the application ID specified in the
         *                      application/meta-data will be used instead.
         * @param parameters    a Bundle containing parameters to pass as part of the
         *                      dialog URL. No validation is done on these parameters; it is
         *                      the caller's responsibility to ensure they are valid. For more information,
         *                      see <a href="https://developers.facebook.com/docs/reference/dialogs/requests/">
         *                      https://developers.facebook.com/docs/reference/dialogs/requests/</a>.
         */
        public RequestsDialogBuilder(Context context, String applicationId, Bundle parameters) {
            super(context, applicationId, APPREQUESTS_DIALOG, parameters);
        }

        /**
         * Sets the string users receiving the request will see. The maximum length
         * is 60 characters.
         *
         * @param message the message
         * @return the builder
         */
        public RequestsDialogBuilder setMessage(String message) {
            getParameters().putString(MESSAGE_PARAM, message);
            return this;
        }

        /**
         * Sets the user ID or user name the request will be sent to. If this is not
         * specified, a friend selector will be displayed and the user can select up
         * to 50 friends.
         *
         * @param id the id or user name to send the request to
         * @return the builder
         */
        public RequestsDialogBuilder setTo(String id) {
            getParameters().putString(TO_PARAM, id);
            return this;
        }

        /**
         * Sets optional data which can be used for tracking; maximum length is 255
         * characters.
         *
         * @param data the data
         * @return the builder
         */
        public RequestsDialogBuilder setData(String data) {
            getParameters().putString(DATA_PARAM, data);
            return this;
        }

        /**
         * Sets an optional title for the dialog; maximum length is 50 characters.
         *
         * @param title the title
         * @return the builder
         */
        public RequestsDialogBuilder setTitle(String title) {
            getParameters().putString(TITLE_PARAM, title);
            return this;
        }
    }
}
