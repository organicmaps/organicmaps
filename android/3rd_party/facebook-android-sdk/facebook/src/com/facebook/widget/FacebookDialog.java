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

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.v4.app.Fragment;
import com.facebook.*;
import com.facebook.internal.AnalyticsEvents;
import com.facebook.internal.NativeProtocol;
import com.facebook.internal.Utility;
import com.facebook.internal.Validate;
import com.facebook.model.GraphObject;
import com.facebook.model.GraphObjectList;
import com.facebook.model.OpenGraphAction;
import com.facebook.model.OpenGraphObject;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.*;

/*
 * Provides an interface for presenting dialogs provided by the Facebook application for Android. This class
 * provides builders that present a strongly-typed interface to generate properly-formed Intents for launching
 * the appropriate Activities provided by the Facebook application.
 */
public class FacebookDialog {

    public static final String COMPLETION_GESTURE_CANCEL = "cancel";

    private static final String EXTRA_DIALOG_COMPLETE_KEY = "com.facebook.platform.extra.DID_COMPLETE";
    private static final String EXTRA_DIALOG_COMPLETION_GESTURE_KEY =
            "com.facebook.platform.extra.COMPLETION_GESTURE";
    private static final String EXTRA_DIALOG_COMPLETION_ID_KEY = "com.facebook.platform.extra.POST_ID";

    private static NativeAppCallAttachmentStore attachmentStore;

    /**
     * Defines a callback interface that will be called when the user completes interacting with a Facebook
     * dialog, or if an error occurs.
     */
    public interface Callback {
        /**
         * Called when the user completes interacting with a Facebook dialog.
         *
         * @param pendingCall a PendingCall containing the call ID and original Intent used to launch the dialog
         * @param data        a Bundle containing the results of the dialog, whose contents will vary depending on the
         *                    type of dialog being displayed.
         */
        void onComplete(PendingCall pendingCall, Bundle data);

        /**
         * Called if an error occurred while presenting a Facebook dialog.
         *
         * @param pendingCall a PendingCall containing the call ID and original Intent used to launch the dialog
         * @param error       the error that occurred
         * @param data        the full set of extras associated with the activity result
         */
        void onError(PendingCall pendingCall, Exception error, Bundle data);
    }

    private interface DialogFeature {
        String getAction();
        int getMinVersion();
    }

    /**
     * Defines a set of features that may be supported by the native Share dialog exposed by the Facebook application.
     * As additional features are added, these flags may be passed to
     * {@link FacebookDialog#canPresentShareDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.ShareDialogFeature...)}
     * to determine whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features, which in turn may be used to determine which UI, etc., to present to the
     * user.
     */
    public enum ShareDialogFeature implements DialogFeature {
        /**
         * Indicates whether the native Share dialog itself is supported by the installed version of the
         * Facebook application.
         */
        SHARE_DIALOG(NativeProtocol.PROTOCOL_VERSION_20130618),
        /**
         * Indicates whether the native Share dialog supports sharing of photo images.
         */
        PHOTOS(NativeProtocol.PROTOCOL_VERSION_20140204),
        ;

        private int minVersion;

        private ShareDialogFeature(int minVersion) {
            this.minVersion = minVersion;
        }

        /**
         * This method is for internal use only.
         */
        public String getAction() {
            return NativeProtocol.ACTION_FEED_DIALOG;
        }

        /**
         * This method is for internal use only.
         */
        public int getMinVersion() {
            return minVersion;
        }
    }

    /**
     * Defines a set of features that may be supported by the native Message dialog exposed by the Facebook Messenger application.
     * As additional features are added, these flags may be passed to
     * {@link FacebookDialog#canPresentMessageDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.MessageDialogFeature...)}
     * to determine whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features, which in turn may be used to determine which UI, etc., to present to the
     * user.
     */
    public enum MessageDialogFeature implements DialogFeature {
        /**
         * Indicates whether the native Message dialog itself is supported by the installed version of the
         * Facebook application.
         */
        MESSAGE_DIALOG(NativeProtocol.PROTOCOL_VERSION_20140204),
        /**
         * Indicates whether the native Message dialog supports sharing of photo images.
         */
        PHOTOS(NativeProtocol.PROTOCOL_VERSION_20140324),
        ;

        private int minVersion;

        private MessageDialogFeature(int minVersion) {
            this.minVersion = minVersion;
        }

        /**
         * This method is for internal use only.
         */
        public String getAction() {
            return NativeProtocol.ACTION_MESSAGE_DIALOG;
        }

        /**
         * This method is for internal use only.
         */
        public int getMinVersion() {
            return minVersion;
        }
    }


    /**
     * Defines a set of features that may be supported by the native Open Graph dialogs exposed by the Facebook
     * application. As additional features are added, these flags may be passed to
     * {@link FacebookDialog#canPresentOpenGraphActionDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.OpenGraphActionDialogFeature...)}
     * to determine whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features, which in turn may be used to determine which UI, etc., to present to the
     * user.
     */
    public enum OpenGraphActionDialogFeature implements DialogFeature {
        /**
         * Indicates whether the native Open Graph action dialog itself is supported by the installed version of the
         * Facebook application.
         */
        OG_ACTION_DIALOG(NativeProtocol.PROTOCOL_VERSION_20130618);

        private int minVersion;

        private OpenGraphActionDialogFeature(int minVersion) {
            this.minVersion = minVersion;
        }

        /**
         * This method is for internal use only.
         */
        public String getAction() {
            return NativeProtocol.ACTION_OGACTIONPUBLISH_DIALOG;
        }

        /**
         * This method is for internal use only.
         */
        public int getMinVersion() {
            return minVersion;
        }
    }

    /**
     * Defines a set of features that may be supported by the native Open Graph Message dialogs exposed by the Facebook
     * application. As additional features are added, these flags may be passed to
     * {@link FacebookDialog#canPresentOpenGraphMessageDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.OpenGraphMessageDialogFeature...)}
     * to determine whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features, which in turn may be used to determine which UI, etc., to present to the
     * user.
     */
    public enum OpenGraphMessageDialogFeature implements DialogFeature {
        /**
         * Indicates whether the native Open Graph Message dialog itself is supported by the installed version of the
         * Messenger application.
         */
        OG_MESSAGE_DIALOG(NativeProtocol.PROTOCOL_VERSION_20140204);

        private int minVersion;

        private OpenGraphMessageDialogFeature(int minVersion) {
            this.minVersion = minVersion;
        }

        /**
         * This method is for internal use only.
         */
        public String getAction() {
            return NativeProtocol.ACTION_OGMESSAGEPUBLISH_DIALOG;
        }

        /**
         * This method is for internal use only.
         */
        public int getMinVersion() {
            return minVersion;
        }
    }

    interface OnPresentCallback {
        void onPresent(Context context) throws Exception;
    }

    /**
     * Determines whether the native dialog completed normally (without error or exception).
     *
     * @param result the bundle passed back to onActivityResult
     * @return true if the native dialog completed normally
     */
    public static boolean getNativeDialogDidComplete(Bundle result) {
        return result.getBoolean(EXTRA_DIALOG_COMPLETE_KEY, false);
    }

    /**
     * Returns the gesture with which the user completed the native dialog. This is only returned if the
     * user has previously authorized the calling app with basic permissions.
     *
     * @param result the bundle passed back to onActivityResult
     * @return "post" or "cancel" as the completion gesture
     */
    public static String getNativeDialogCompletionGesture(Bundle result) {
        return result.getString(EXTRA_DIALOG_COMPLETION_GESTURE_KEY);
    }

    /**
     * Returns the id of the published post. This is only returned if the user has previously given the
     * app publish permissions.
     *
     * @param result the bundle passed back to onActivityResult
     * @return the id of the published post
     */
    public static String getNativeDialogPostId(Bundle result) {
        return result.getString(EXTRA_DIALOG_COMPLETION_ID_KEY);
    }

    private Activity activity;
    private Fragment fragment;
    private PendingCall appCall;
    private OnPresentCallback onPresentCallback;

    private FacebookDialog(Activity activity, Fragment fragment, PendingCall appCall,
            OnPresentCallback onPresentCallback) {
        this.activity = activity;
        this.fragment = fragment;
        this.appCall = appCall;
        this.onPresentCallback = onPresentCallback;
    }

    /**
     * Launches an activity in the Facebook application to present the desired dialog. This method returns a
     * PendingCall that contains a unique ID associated with this call to the Facebook application. In general,
     * a calling Activity should use UiLifecycleHelper to handle incoming activity results, in order to ensure
     * proper processing of the results from this dialog.
     *
     * @return a PendingCall containing the unique call ID corresponding to this call to the Facebook application
     */
    public PendingCall present() {
        logDialogActivity(activity, fragment, getEventName(appCall.getRequestIntent()),
                AnalyticsEvents.PARAMETER_DIALOG_OUTCOME_VALUE_COMPLETED);

        if (onPresentCallback != null) {
            try {
                onPresentCallback.onPresent(activity);
            } catch (Exception e) {
                throw new FacebookException(e);
            }
        }

        if (fragment != null) {
            fragment.startActivityForResult(appCall.getRequestIntent(), appCall.getRequestCode());
        } else {
            activity.startActivityForResult(appCall.getRequestIntent(), appCall.getRequestCode());
        }
        return appCall;
    }

    /**
     * Parses the results of a dialog activity and calls the appropriate method on the provided Callback.
     *
     * @param context     the Context that is handling the activity result
     * @param appCall     an PendingCall containing the call ID and original Intent used to launch the dialog
     * @param requestCode the request code for the activity result
     * @param data        the result Intent
     * @param callback    a callback to call after parsing the results
     * @return true if the activity result was handled, false if not
     */
    public static boolean handleActivityResult(Context context, PendingCall appCall, int requestCode, Intent data,
            Callback callback) {
        if (requestCode != appCall.getRequestCode()) {
            return false;
        }

        if (attachmentStore != null) {
            attachmentStore.cleanupAttachmentsForCall(context, appCall.getCallId());
        }

        if (callback != null) {
            if (NativeProtocol.isErrorResult(data)) {
                Exception error = NativeProtocol.getErrorFromResult(data);

                // TODO  - data.getExtras() doesn't work for the bucketed protocol.
                callback.onError(appCall, error, data.getExtras());
            } else {
                callback.onComplete(appCall, NativeProtocol.getSuccessResultsFromIntent(data));
            }
        }

        return true;
    }

    /**
     * Determines whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features of the native Share dialog, which in turn may be used to determine
     * which UI, etc., to present to the user.
     *
     * @param context  the calling Context
     * @param features zero or more features to check for; {@link ShareDialogFeature#SHARE_DIALOG} is implicitly checked
     *                 if not explicitly specified
     * @return true if all of the specified features are supported by the currently installed version of the
     *         Facebook application; false if any of the features are not supported
     */
    public static boolean canPresentShareDialog(Context context, ShareDialogFeature... features) {
        return handleCanPresent(context, EnumSet.of(ShareDialogFeature.SHARE_DIALOG, features));
    }

    /**
     * Determines whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features of the native Message dialog, which in turn may be used to determine
     * which UI, etc., to present to the user.
     *
     * @param context  the calling Context
     * @param features zero or more features to check for; {@link com.facebook.widget.FacebookDialog.MessageDialogFeature#MESSAGE_DIALOG} is implicitly
     *                 checked if not explicitly specified
     * @return true if all of the specified features are supported by the currently installed version of the
     *         Facebook application; false if any of the features are not supported
     */
    public static boolean canPresentMessageDialog(Context context, MessageDialogFeature... features) {
        return handleCanPresent(context, EnumSet.of(MessageDialogFeature.MESSAGE_DIALOG, features));
    }

    /**
     * Determines whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features of the native Open Graph action dialog, which in turn may be used to
     * determine which UI, etc., to present to the user.
     *
     * @param context  the calling Context
     * @param features zero or more features to check for; {@link OpenGraphActionDialogFeature#OG_ACTION_DIALOG} is implicitly
     *                 checked if not explicitly specified
     * @return true if all of the specified features are supported by the currently installed version of the
     *         Facebook application; false if any of the features are not supported
     */
    public static boolean canPresentOpenGraphActionDialog(Context context, OpenGraphActionDialogFeature... features) {
        return handleCanPresent(context, EnumSet.of(OpenGraphActionDialogFeature.OG_ACTION_DIALOG, features));
    }

    /**
     * Determines whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features of the native Open Graph Message dialog, which in turn may be used to
     * determine which UI, etc., to present to the user.
     *
     * @param context  the calling Context
     * @param features zero or more features to check for; {@link com.facebook.widget.FacebookDialog.OpenGraphMessageDialogFeature#OG_MESSAGE_DIALOG} is
     *                 implicitly checked if not explicitly specified
     * @return true if all of the specified features are supported by the currently installed version of the
     *         Facebook application; false if any of the features are not supported
     */
    public static boolean canPresentOpenGraphMessageDialog(Context context, OpenGraphMessageDialogFeature... features) {
        return handleCanPresent(context, EnumSet.of(OpenGraphMessageDialogFeature.OG_MESSAGE_DIALOG, features));
    }

    private static boolean handleCanPresent(Context context, Iterable<? extends DialogFeature> features) {
        return getProtocolVersionForNativeDialog(context, getActionForFeatures(features), getMinVersionForFeatures(features))
                != NativeProtocol.NO_PROTOCOL_AVAILABLE;
    }

    private static int getProtocolVersionForNativeDialog(Context context, String action, int requiredVersion) {
        return NativeProtocol.getLatestAvailableProtocolVersionForAction(context, action, requiredVersion);
    }

    private static NativeAppCallAttachmentStore getAttachmentStore() {
        if (attachmentStore == null) {
            attachmentStore = new NativeAppCallAttachmentStore();
        }
        return attachmentStore;
    }

    private static int getMinVersionForFeatures(Iterable<? extends DialogFeature> features) {
        int minVersion = Integer.MIN_VALUE;
        for (DialogFeature feature : features) {
            // Minimum version to support all features is the maximum of each feature's minimum version.
            minVersion = Math.max(minVersion, feature.getMinVersion());
        }
        return minVersion;
    }

    private static String getActionForFeatures(Iterable<? extends DialogFeature> features) {
        String action = null;
        for (DialogFeature feature : features) {
            // All actions in a set of DialogFeatures should have the same action
            // So we can break after assigning the first one
            action = feature.getAction();
            break;
        }
        return action;
    }

    private static void logDialogActivity(Activity activity, Fragment fragment, String eventName, String outcome) {
        AppEventsLogger logger = AppEventsLogger.newLogger(fragment != null ? fragment.getActivity() : activity);
        Bundle parameters = new Bundle();
        parameters.putString(AnalyticsEvents.PARAMETER_DIALOG_OUTCOME, outcome);
        logger.logSdkEvent(eventName, null, parameters);
    }

    static private String getEventName(Intent intent) {
        String action = intent.getStringExtra(NativeProtocol.EXTRA_PROTOCOL_ACTION);
        boolean hasPhotos = intent.hasExtra(NativeProtocol.EXTRA_PHOTOS);
        return getEventName(action, hasPhotos);
    }

    static private String getEventName(String action, boolean hasPhotos) {
        String eventName;

        if (action.equals(NativeProtocol.ACTION_FEED_DIALOG)) {
            eventName = hasPhotos ?
                    AnalyticsEvents.EVENT_NATIVE_DIALOG_TYPE_PHOTO_SHARE :
                    AnalyticsEvents.EVENT_NATIVE_DIALOG_TYPE_SHARE;
        } else if (action.equals(NativeProtocol.ACTION_MESSAGE_DIALOG)) {
            eventName = hasPhotos ?
                    AnalyticsEvents.EVENT_NATIVE_DIALOG_TYPE_PHOTO_MESSAGE :
                    AnalyticsEvents.EVENT_NATIVE_DIALOG_TYPE_MESSAGE;
        } else if (action.equals(NativeProtocol.ACTION_OGACTIONPUBLISH_DIALOG)) {
            eventName = AnalyticsEvents.EVENT_NATIVE_DIALOG_TYPE_OG_SHARE;
        } else if (action.equals(NativeProtocol.ACTION_OGMESSAGEPUBLISH_DIALOG)) {
            eventName = AnalyticsEvents.EVENT_NATIVE_DIALOG_TYPE_OG_MESSAGE;
        } else {
            throw new FacebookException("An unspecified action was presented");
        }
        return eventName;
    }

    /**
     * Provides a base class for various FacebookDialog builders. This is public primarily to allow its use elsewhere
     * in the Android SDK; developers are discouraged from constructing their own FacebookDialog builders as the
     * internal API may change.
     *
     * @param <CONCRETE> The concrete base class of the builder.
     */
    public abstract static class Builder<CONCRETE extends Builder<?>> {
        final protected Activity activity;
        final protected String applicationId;
        final protected PendingCall appCall;
        protected Fragment fragment;
        protected String applicationName;
        protected HashMap<String, Bitmap> imageAttachments = new HashMap<String, Bitmap>();
        protected HashMap<String, File> imageAttachmentFiles = new HashMap<String, File>();

        /**
         * Constructor.
         *
         * @param activity the Activity which is presenting the native Share dialog; must not be null
         */
        public Builder(Activity activity) {
            Validate.notNull(activity, "activity");

            this.activity = activity;
            applicationId = Utility.getMetadataApplicationId(activity);
            appCall = new PendingCall(NativeProtocol.DIALOG_REQUEST_CODE);
        }

        /**
         * Sets the request code that will be passed to handleActivityResult when this activity completes; the
         * default is NativeProtocol.DIALOG_REQUEST_CODE.
         *
         * @param requestCode the request code
         * @return this instance of the builder
         */
        public CONCRETE setRequestCode(int requestCode) {
            this.appCall.setRequestCode(requestCode);
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the name of the application to be displayed in the dialog. If provided, this optimizes the user
         * experience as a preview of a shared item, etc., can be displayed sooner.
         *
         * @param applicationName the name of the Facebook application
         * @return this instance of the builder
         */
        public CONCRETE setApplicationName(String applicationName) {
            this.applicationName = applicationName;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the fragment that should launch the dialog. This allows the dialog to be
         * launched from a Fragment, and will allow the fragment to receive the
         * {@link Fragment#onActivityResult(int, int, android.content.Intent) onActivityResult}
         * call rather than the Activity.
         *
         * @param fragment the fragment that contains this control
         */
        public CONCRETE setFragment(Fragment fragment) {
            this.fragment = fragment;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Constructs a FacebookDialog with an Intent that is correctly populated to present the dialog within
         * the Facebook application.
         *
         * @return a FacebookDialog instance
         */
        public FacebookDialog build() {
            validate();

            String action = getActionForFeatures(getDialogFeatures());
            int protocolVersion = getProtocolVersionForNativeDialog(activity, action,
                    getMinVersionForFeatures(getDialogFeatures()));

            Bundle extras = null;
            if (NativeProtocol.isVersionCompatibleWithBucketedIntent(protocolVersion)) {
                // Facebook app supports the new bucketed protocol
                extras = getMethodArguments();
            } else {
                // Facebook app only supports the old flat protocol
                extras = setBundleExtras(new Bundle());
            }

            Intent intent = NativeProtocol.createPlatformActivityIntent(
                    activity,
                    appCall.getCallId().toString(),
                    action,
                    protocolVersion,
                    applicationName,
                    extras);
            if (intent == null) {
                logDialogActivity(activity, fragment,
                        getEventName(action, extras.containsKey(NativeProtocol.EXTRA_PHOTOS)),
                        AnalyticsEvents.PARAMETER_DIALOG_OUTCOME_VALUE_FAILED);

                throw new FacebookException(
                        "Unable to create Intent; this likely means the Facebook app is not installed.");
            }

            appCall.setRequestIntent(intent);

            return new FacebookDialog(activity, fragment, appCall, getOnPresentCallback());
        }

        /**
         * Determines whether the native dialog can be presented (i.e., whether the required version of the
         * Facebook application is installed on the device, and whether the installed version supports all of
         * the parameters specified for the dialog).
         *
         * @return true if the dialog can be presented; false if not
         */
        public boolean canPresent() {
            return handleCanPresent(activity, getDialogFeatures());
        }

        void validate() {
        }

        OnPresentCallback getOnPresentCallback() {
            return new OnPresentCallback() {
                @Override
                public void onPresent(Context context) throws Exception {
                    // We're actually being presented, so put our attachments in the content provider.
                    if (imageAttachments != null && imageAttachments.size() > 0) {
                        getAttachmentStore().addAttachmentsForCall(context, appCall.getCallId(), imageAttachments);
                    }
                    if (imageAttachmentFiles != null && imageAttachmentFiles.size() > 0) {
                        getAttachmentStore().addAttachmentFilesForCall(context, appCall.getCallId(),
                                imageAttachmentFiles);
                    }
                }
            };
        }

        protected List<String> addImageAttachments(Collection<Bitmap> bitmaps) {
            ArrayList<String> attachmentUrls = new ArrayList<String>();
            for (Bitmap bitmap : bitmaps) {
                String attachmentName = UUID.randomUUID().toString();

                addImageAttachment(attachmentName, bitmap);

                String url = NativeAppCallContentProvider.getAttachmentUrl(applicationId, appCall.getCallId(),
                        attachmentName);
                attachmentUrls.add(url);
            }

            return attachmentUrls;
        }

        protected List<String> addImageAttachmentFiles(Collection<File> bitmapFiles) {
            ArrayList<String> attachmentUrls = new ArrayList<String>();
            for (File bitmapFile : bitmapFiles) {
                String attachmentName = UUID.randomUUID().toString();

                addImageAttachment(attachmentName, bitmapFile);

                String url = NativeAppCallContentProvider.getAttachmentUrl(applicationId, appCall.getCallId(),
                        attachmentName);
                attachmentUrls.add(url);
            }

            return attachmentUrls;
        }

        List<String> getImageAttachmentNames() {
            return new ArrayList<String>(imageAttachments.keySet());
        }

        protected abstract Bundle setBundleExtras(Bundle extras);

        protected abstract Bundle getMethodArguments();

        protected void putExtra(Bundle extras, String key, String value) {
            if (value != null) {
                extras.putString(key, value);
            }
        }

        abstract EnumSet<? extends DialogFeature> getDialogFeatures();

        protected CONCRETE addImageAttachment(String imageName, Bitmap bitmap) {
            imageAttachments.put(imageName, bitmap);
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        protected CONCRETE addImageAttachment(String imageName, File attachment) {
            imageAttachmentFiles.put(imageName, attachment);
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }
    }

    private abstract static class ShareDialogBuilderBase<CONCRETE extends ShareDialogBuilderBase<?>> extends Builder<CONCRETE> {
        private String name;
        private String caption;
        private String description;
        protected String link;
        private String picture;
        private String place;
        private ArrayList<String> friends;
        private String ref;
        private boolean dataErrorsFatal;

        /**
         * Constructor.
         *
         * @param activity the Activity which is presenting the native Share dialog; must not be null
         */
        public ShareDialogBuilderBase(Activity activity) {
            super(activity);
        }

        /**
         * Sets the title of the item to be shared.
         *
         * @param name the title
         * @return this instance of the builder
         */
        public CONCRETE setName(String name) {
            this.name = name;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the subtitle of the item to be shared.
         *
         * @param caption the subtitle
         * @return this instance of the builder
         */
        public CONCRETE setCaption(String caption) {
            this.caption = caption;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the description of the item to be shared.
         *
         * @param description the description
         * @return this instance of the builder
         */
        public CONCRETE setDescription(String description) {
            this.description = description;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the URL of the item to be shared.
         *
         * @param link the URL
         * @return this instance of the builder
         */
        public CONCRETE setLink(String link) {
            this.link = link;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the URL of the image of the item to be shared.
         *
         * @param picture the URL of the image
         * @return this instance of the builder
         */
        public CONCRETE setPicture(String picture) {
            this.picture = picture;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the place for the item to be shared.
         *
         * @param place the Facebook ID of the place
         * @return this instance of the builder
         */
        public CONCRETE setPlace(String place) {
            this.place = place;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the tagged friends for the item to be shared.
         *
         * @param friends a list of Facebook IDs of the friends to be tagged in the shared item
         * @return this instance of the builder
         */
        public CONCRETE setFriends(List<String> friends) {
            this.friends = new ArrayList<String>(friends);
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the 'ref' property of the item to be shared.
         *
         * @param ref the 'ref' property
         * @return this instance of the builder
         */
        public CONCRETE setRef(String ref) {
            this.ref = ref;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets whether errors encountered during previewing the shared item should be considered fatal and
         * cause the dialog to return an error
         *
         * @param dataErrorsFatal true if data errors should be fatal; false if not
         * @return this instance of the builder
         */
        public CONCRETE setDataErrorsFatal(boolean dataErrorsFatal) {
            this.dataErrorsFatal = dataErrorsFatal;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        @Override
        protected Bundle setBundleExtras(Bundle extras) {
            putExtra(extras, NativeProtocol.EXTRA_APPLICATION_ID, applicationId);
            putExtra(extras, NativeProtocol.EXTRA_APPLICATION_NAME, applicationName);
            putExtra(extras, NativeProtocol.EXTRA_TITLE, name);
            putExtra(extras, NativeProtocol.EXTRA_SUBTITLE, caption);
            putExtra(extras, NativeProtocol.EXTRA_DESCRIPTION, description);
            putExtra(extras, NativeProtocol.EXTRA_LINK, link);
            putExtra(extras, NativeProtocol.EXTRA_IMAGE, picture);
            putExtra(extras, NativeProtocol.EXTRA_PLACE_TAG, place);
            putExtra(extras, NativeProtocol.EXTRA_TITLE, name);
            putExtra(extras, NativeProtocol.EXTRA_REF, ref);

            extras.putBoolean(NativeProtocol.EXTRA_DATA_FAILURES_FATAL, dataErrorsFatal);
            if (!Utility.isNullOrEmpty(friends)) {
                extras.putStringArrayList(NativeProtocol.EXTRA_FRIEND_TAGS, friends);
            }
            return extras;
        }

        @Override
        protected Bundle getMethodArguments() {
            Bundle methodArguments = new Bundle();

            putExtra(methodArguments, NativeProtocol.METHOD_ARGS_TITLE, name);
            putExtra(methodArguments, NativeProtocol.METHOD_ARGS_SUBTITLE, caption);
            putExtra(methodArguments, NativeProtocol.METHOD_ARGS_DESCRIPTION, description);
            putExtra(methodArguments, NativeProtocol.METHOD_ARGS_LINK, link);
            putExtra(methodArguments, NativeProtocol.METHOD_ARGS_IMAGE, picture);
            putExtra(methodArguments, NativeProtocol.METHOD_ARGS_PLACE_TAG, place);
            putExtra(methodArguments, NativeProtocol.METHOD_ARGS_TITLE, name);
            putExtra(methodArguments, NativeProtocol.METHOD_ARGS_REF, ref);

            methodArguments.putBoolean(NativeProtocol.METHOD_ARGS_DATA_FAILURES_FATAL, dataErrorsFatal);
            if (!Utility.isNullOrEmpty(friends)) {
                methodArguments.putStringArrayList(NativeProtocol.METHOD_ARGS_FRIEND_TAGS, friends);
            }

            return methodArguments;
        }
    }

    /**
     * Provides a builder which can construct a FacebookDialog instance suitable for presenting the native
     * Share dialog. This builder will throw an exception if the Facebook application is not installed, so it
     * should only be used if {@link FacebookDialog#canPresentShareDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.ShareDialogFeature...)}  indicates the capability is available.
     */
    public static class ShareDialogBuilder extends ShareDialogBuilderBase<ShareDialogBuilder> {

        /**
         * Constructor.
         *
         * @param activity the Activity which is presenting the native Share dialog; must not be null
         */
        public ShareDialogBuilder(Activity activity) {
            super(activity);
        }

        @Override
        EnumSet<? extends DialogFeature> getDialogFeatures() {
            return EnumSet.of(ShareDialogFeature.SHARE_DIALOG);
        }
    }

    private static abstract class PhotoDialogBuilderBase<CONCRETE extends PhotoDialogBuilderBase<?>>
            extends Builder<CONCRETE> {
        static int MAXIMUM_PHOTO_COUNT = 6;
        private String place;
        private ArrayList<String> friends;
        private ArrayList<String> imageAttachmentUrls = new ArrayList<String>();

        /**
         * Constructor.
         *
         * @param activity the Activity which is presenting the native Share dialog; must not be null
         */
        public PhotoDialogBuilderBase(Activity activity) {
            super(activity);
        }

        /**
         * Sets the place for the item to be shared.
         *
         * @param place the Facebook ID of the place
         * @return this instance of the builder
         */
        public CONCRETE setPlace(String place) {
            this.place = place;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Sets the tagged friends for the item to be shared.
         *
         * @param friends a list of Facebook IDs of the friends to be tagged in the shared item
         * @return this instance of the builder
         */
        public CONCRETE setFriends(List<String> friends) {
            this.friends = new ArrayList<String>(friends);
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * <p></p>Adds one or more photos to the list of photos to display in the native Share dialog, by providing
         * an in-memory representation of the photos. The dialog's callback will be called once the user has
         * shared the photos, but the photos themselves may be uploaded in the background by the Facebook app;
         * apps wishing to be notified when the photo upload has succeeded or failed should extend the
         * FacebookBroadcastReceiver class and register it in their AndroidManifest.xml.</p>
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         * No more than six photos may be shared at a time.
         * @param photos a collection of Files representing photos to be uploaded
         * @return this instance of the builder
         */
        public CONCRETE addPhotos(Collection<Bitmap> photos) {
            imageAttachmentUrls.addAll(addImageAttachments(photos));
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * Adds one or more photos to the list of photos to display in the native Share dialog, by specifying
         * their location in the file system. The dialog's callback will be called once the user has
         * shared the photos, but the photos themselves may be uploaded in the background by the Facebook app;
         * apps wishing to be notified when the photo upload has succeeded or failed should extend the
         * FacebookBroadcastReceiver class and register it in their AndroidManifest.xml.
         * No more than six photos may be shared at a time.
         * @param photos a collection of Files representing photos to be uploaded
         * @return this instance of the builder
         */
        public CONCRETE addPhotoFiles(Collection<File> photos) {
            imageAttachmentUrls.addAll(addImageAttachmentFiles(photos));
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        abstract int getMaximumNumberOfPhotos();

        @Override
        void validate() {
            super.validate();

            if (imageAttachmentUrls.isEmpty()) {
                throw new FacebookException("Must specify at least one photo.");
            }

            if (imageAttachmentUrls.size() > getMaximumNumberOfPhotos()) {
                throw new FacebookException(String.format("Cannot add more than %d photos.", getMaximumNumberOfPhotos()));
            }
        }

        @Override
        protected Bundle setBundleExtras(Bundle extras) {
            putExtra(extras, NativeProtocol.EXTRA_APPLICATION_ID, applicationId);
            putExtra(extras, NativeProtocol.EXTRA_APPLICATION_NAME, applicationName);
            putExtra(extras, NativeProtocol.EXTRA_PLACE_TAG, place);
            extras.putStringArrayList(NativeProtocol.EXTRA_PHOTOS, imageAttachmentUrls);

            if (!Utility.isNullOrEmpty(friends)) {
                extras.putStringArrayList(NativeProtocol.EXTRA_FRIEND_TAGS, friends);
            }
            return extras;
        }

        @Override
        protected Bundle getMethodArguments() {
            Bundle methodArgs = new Bundle();

            putExtra(methodArgs, NativeProtocol.METHOD_ARGS_PLACE_TAG, place);
            methodArgs.putStringArrayList(NativeProtocol.METHOD_ARGS_PHOTOS, imageAttachmentUrls);

            if (!Utility.isNullOrEmpty(friends)) {
                methodArgs.putStringArrayList(NativeProtocol.METHOD_ARGS_FRIEND_TAGS, friends);
            }

            return methodArgs;
        }
    }

    /**
     * Provides a builder which can construct a FacebookDialog instance suitable for presenting the native
     * Share dialog for sharing photos. This builder will throw an exception if the Facebook application is not
     * installed, so it should only be used if {@link FacebookDialog#canPresentShareDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.ShareDialogFeature...)}  indicates the capability is available.
     */
    public static class PhotoShareDialogBuilder extends PhotoDialogBuilderBase<PhotoShareDialogBuilder> {
        /**
         * Constructor.
         *
         * @param activity the Activity which is presenting the native Share dialog; must not be null
         */
        public PhotoShareDialogBuilder(Activity activity) {
            super(activity);
        }

        @Override
        EnumSet<? extends DialogFeature> getDialogFeatures() {
            return EnumSet.of(ShareDialogFeature.SHARE_DIALOG, ShareDialogFeature.PHOTOS);
        }

        @Override
        int getMaximumNumberOfPhotos() {
            return MAXIMUM_PHOTO_COUNT;
        }
    }

    /**
     * Provides a builder which can construct a FacebookDialog instance suitable for presenting the native
     * Message dialog for sharing photos. This builder will throw an exception if the Messenger application is not
     * installed, so it should only be used if {@link FacebookDialog#canPresentMessageDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.MessageDialogFeature...)} indicates the capability is available.
     */
    public static class PhotoMessageDialogBuilder extends PhotoDialogBuilderBase<PhotoMessageDialogBuilder> {
        /**
         * Constructor.
         *
         * @param activity the Activity which is presenting the native Message dialog; must not be null
         */
        public PhotoMessageDialogBuilder(Activity activity) {
            super(activity);
        }

        @Override
        EnumSet<? extends DialogFeature> getDialogFeatures() {
            return EnumSet.of(MessageDialogFeature.MESSAGE_DIALOG, MessageDialogFeature.PHOTOS);
        }

        @Override
        int getMaximumNumberOfPhotos() {
            return MAXIMUM_PHOTO_COUNT;
        }
    }

    /**
     * Provides a builder which can construct a FacebookDialog instance suitable for presenting the native
     * Message dialog. This builder will throw an exception if the Facebook Messenger application is not installed, so it
     * should only be used if {@link FacebookDialog#canPresentMessageDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.MessageDialogFeature...)}  indicates the capability is available.
     * The "friends" and "place" properties will be ignored as the Facebook Messenger app does not support tagging.
     */
    public static class MessageDialogBuilder extends ShareDialogBuilderBase<MessageDialogBuilder> {

        /**
         * Constructor.
         *
         * @param activity the Activity which is presenting the native Message dialog; must not be null
         */
        public MessageDialogBuilder(Activity activity) {
            super(activity);
        }

        @Override
        EnumSet<? extends DialogFeature> getDialogFeatures() {
            return EnumSet.of(MessageDialogFeature.MESSAGE_DIALOG);
        }
    }

    private static abstract class OpenGraphDialogBuilderBase<CONCRETE extends OpenGraphDialogBuilderBase<?>>
            extends Builder<CONCRETE> {

        private String previewPropertyName;
        private OpenGraphAction action;
        private String actionType;
        private boolean dataErrorsFatal;

        /**
         * Constructor.
         *
         * @param activity            the Activity which is presenting the native Open Graph action publish dialog;
         *                            must not be null
         * @param action              the Open Graph action to be published, which must contain a reference to at least one
         *                            Open Graph object with the property name specified by setPreviewPropertyName; the action
         *                            must have had its type specified via the {@link OpenGraphAction#setType(String)} method
         * @param actionType          the type of the Open Graph action to be published, which should be the namespace-qualified
         *                            name of the action type (e.g., "myappnamespace:myactiontype"); this will override the type
         *                            of the action passed in.
         * @param previewPropertyName the name of a property on the Open Graph action that contains the
         *                            Open Graph object which will be displayed as a preview to the user
         */
        @Deprecated
        public OpenGraphDialogBuilderBase(Activity activity, OpenGraphAction action, String actionType,
                String previewPropertyName) {
            super(activity);

            Validate.notNull(action, "action");
            Validate.notNullOrEmpty(actionType, "actionType");
            Validate.notNullOrEmpty(previewPropertyName, "previewPropertyName");
            if (action.getProperty(previewPropertyName) == null) {
                throw new IllegalArgumentException(
                        "A property named \"" + previewPropertyName + "\" was not found on the action.  The name of " +
                                "the preview property must match the name of an action property.");
            }
            String typeOnAction = action.getType();
            if (!Utility.isNullOrEmpty(typeOnAction) && !typeOnAction.equals(actionType)) {
                throw new IllegalArgumentException("'actionType' must match the type of 'action' if it is specified. " +
                        "Consider using OpenGraphDialogBuilderBase(Activity activity, OpenGraphAction action, " +
                        "String previewPropertyName) instead.");
            }
            this.action = action;
            this.actionType = actionType;
            this.previewPropertyName = previewPropertyName;
        }

        /**
         * Constructor.
         *
         * @param activity            the Activity which is presenting the native Open Graph action publish dialog;
         *                            must not be null
         * @param action              the Open Graph action to be published, which must contain a reference to at least one
         *                            Open Graph object with the property name specified by setPreviewPropertyName; the action
         *                            must have had its type specified via the {@link OpenGraphAction#setType(String)} method
         * @param previewPropertyName the name of a property on the Open Graph action that contains the
         *                            Open Graph object which will be displayed as a preview to the user
         */
        public OpenGraphDialogBuilderBase(Activity activity, OpenGraphAction action, String previewPropertyName) {
            super(activity);

            Validate.notNull(action, "action");
            Validate.notNullOrEmpty(action.getType(), "action.getType()");
            Validate.notNullOrEmpty(previewPropertyName, "previewPropertyName");
            if (action.getProperty(previewPropertyName) == null) {
                throw new IllegalArgumentException(
                        "A property named \"" + previewPropertyName + "\" was not found on the action.  The name of " +
                                "the preview property must match the name of an action property.");
            }

            this.action = action;
            this.actionType = action.getType();
            this.previewPropertyName = previewPropertyName;
        }

        /**
         * Sets whether errors encountered during previewing the shared item should be considered fatal and
         * cause the dialog to return an error
         *
         * @param dataErrorsFatal true if data errors should be fatal; false if not
         * @return this instance of the builder
         */
        public CONCRETE setDataErrorsFatal(boolean dataErrorsFatal) {
            this.dataErrorsFatal = dataErrorsFatal;
            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * <p>Specifies a list of images for the Open Graph action that should be uploaded prior to publishing the
         * action. The action must already have been set prior to calling this method. This method will generate unique
         * names for the image attachments and update the action to refer to these attachments. Note that calling
         * setAction again after calling this method will not clear the image attachments already set, but the new
         * action will have no reference to the existing attachments. The images will not be marked as being
         * user-generated.</p>
         * <p/>
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param bitmaps a list of Bitmaps to be uploaded and attached to the Open Graph action
         * @return this instance of the builder
         */
        public CONCRETE setImageAttachmentsForAction(List<Bitmap> bitmaps) {
            return setImageAttachmentsForAction(bitmaps, false);
        }

        /**
         * <p>Specifies a list of images for the Open Graph action that should be uploaded prior to publishing the
         * action. The action must already have been set prior to calling this method. This method will generate unique
         * names for the image attachments and update the action to refer to these attachments. Note that calling
         * setAction again after calling this method will not clear the image attachments already set, but the new
         * action will have no reference to the existing attachments. The images may be marked as being
         * user-generated -- refer to
         * <a href="https://developers.facebook.com/docs/opengraph/howtos/adding-photos-to-stories/">this article</a>
         * for more information.</p>
         * <p/>
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param bitmaps         a list of Bitmaps to be uploaded and attached to the Open Graph action
         * @param isUserGenerated if true, specifies that the user_generated flag should be set for these images
         * @return this instance of the builder
         */
        public CONCRETE setImageAttachmentsForAction(List<Bitmap> bitmaps,
                boolean isUserGenerated) {
            Validate.containsNoNulls(bitmaps, "bitmaps");
            if (action == null) {
                throw new FacebookException("Can not set attachments prior to setting action.");
            }

            List<String> attachmentUrls = addImageAttachments(bitmaps);
            updateActionAttachmentUrls(attachmentUrls, isUserGenerated);

            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * <p>Specifies a list of images for the Open Graph action that should be uploaded prior to publishing the
         * action. The action must already have been set prior to calling this method.  The images will not be marked
         * as being user-generated. This method will generate unique names for the image attachments and update the
         * action to refer to these attachments. Note that calling setAction again after calling this method will
         * not clear the image attachments already set, but the new action will have no reference to the existing
         * attachments.</p>
         * <p/>
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param bitmapFiles a list of Files containing bitmaps to be uploaded and attached to the Open Graph action
         * @return this instance of the builder
         */
        public CONCRETE setImageAttachmentFilesForAction(List<File> bitmapFiles) {
            return setImageAttachmentFilesForAction(bitmapFiles, false);
        }

        /**
         * <p>Specifies a list of images for the Open Graph action that should be uploaded prior to publishing the
         * action. The action must already have been set prior to calling this method. The images may be marked as being
         * user-generated -- refer to
         * <a href="https://developers.facebook.com/docs/opengraph/howtos/adding-photos-to-stories/">this article</a>
         * for more information. This method will generate unique
         * names for the image attachments and update the action to refer to these attachments. Note that calling
         * setAction again after calling this method will not clear the image attachments already set, but the new
         * action will have no reference to the existing attachments.</p>
         * <p/>
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param bitmapFiles     a list of Files containing bitmaps to be uploaded and attached to the Open Graph action
         * @param isUserGenerated if true, specifies that the user_generated flag should be set for these images
         * @return this instance of the builder
         */
        public CONCRETE setImageAttachmentFilesForAction(List<File> bitmapFiles,
                boolean isUserGenerated) {
            Validate.containsNoNulls(bitmapFiles, "bitmapFiles");
            if (action == null) {
                throw new FacebookException("Can not set attachments prior to setting action.");
            }

            List<String> attachmentUrls = addImageAttachmentFiles(bitmapFiles);
            updateActionAttachmentUrls(attachmentUrls, isUserGenerated);

            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        private void updateActionAttachmentUrls(List<String> attachmentUrls, boolean isUserGenerated) {
            List<JSONObject> attachments = action.getImage();
            if (attachments == null) {
                attachments = new ArrayList<JSONObject>(attachmentUrls.size());
            }

            for (String url : attachmentUrls) {
                JSONObject jsonObject = new JSONObject();
                try {
                    jsonObject.put(NativeProtocol.IMAGE_URL_KEY, url);
                    if (isUserGenerated) {
                        jsonObject.put(NativeProtocol.IMAGE_USER_GENERATED_KEY, true);
                    }
                } catch (JSONException e) {
                    throw new FacebookException("Unable to attach images", e);
                }
                attachments.add(jsonObject);
            }
            action.setImage(attachments);
        }


        /**
         * <p>Specifies a list of images for an Open Graph object referenced by the action that should be uploaded
         * prior to publishing the action. The images will not be marked as user-generated.
         * The action must already have been set prior to calling this method, and
         * the action must have a GraphObject-valued property with the specified property name. This method will
         * generate unique names for the image attachments and update the graph object to refer to these
         * attachments. Note that calling setObject again after calling this method, or modifying the value of the
         * specified property, will not clear the image attachments already set, but the new action (or objects)
         * will have no reference to the existing attachments.</p>
         * <p/>
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param objectProperty the name of a property on the action that corresponds to an Open Graph object;
         *                       the object must be marked as a new object to be created
         *                       (i.e., {@link com.facebook.model.OpenGraphObject#getCreateObject()} must return
         *                       true) or an exception will be thrown
         * @param bitmaps        a list of Files containing bitmaps to be uploaded and attached to the Open Graph object
         * @return this instance of the builder
         */
        public CONCRETE setImageAttachmentsForObject(String objectProperty, List<Bitmap> bitmaps) {
            return setImageAttachmentsForObject(objectProperty, bitmaps, false);
        }

        /**
         * <p>Specifies a list of images for an Open Graph object referenced by the action that should be uploaded
         * prior to publishing the action. The images may be marked as being
         * user-generated -- refer to
         * <a href="https://developers.facebook.com/docs/opengraph/howtos/adding-photos-to-stories/">this article</a>
         * for more information.
         * The action must already have been set prior to calling this method, and
         * the action must have a GraphObject-valued property with the specified property name. This method will
         * generate unique names for the image attachments and update the graph object to refer to these
         * attachments. Note that calling setObject again after calling this method, or modifying the value of the
         * specified property, will not clear the image attachments already set, but the new action (or objects)
         * will have no reference to the existing attachments.</p>
         * <p/>
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param objectProperty  the name of a property on the action that corresponds to an Open Graph object;
         *                        the object must be marked as a new object to be created
         *                        (i.e., {@link com.facebook.model.OpenGraphObject#getCreateObject()} must return
         *                        true) or an exception will be thrown
         * @param objectProperty  the name of a property on the action that corresponds to an Open Graph object
         * @param bitmaps         a list of Files containing bitmaps to be uploaded and attached to the Open Graph object
         * @param isUserGenerated if true, specifies that the user_generated flag should be set for these images
         * @return this instance of the builder
         */
        public CONCRETE setImageAttachmentsForObject(String objectProperty, List<Bitmap> bitmaps,
                boolean isUserGenerated) {
            Validate.notNull(objectProperty, "objectProperty");
            Validate.containsNoNulls(bitmaps, "bitmaps");
            if (action == null) {
                throw new FacebookException("Can not set attachments prior to setting action.");
            }

            List<String> attachmentUrls = addImageAttachments(bitmaps);
            updateObjectAttachmentUrls(objectProperty, attachmentUrls, isUserGenerated);

            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        /**
         * <p>Specifies a list of images for an Open Graph object referenced by the action that should be uploaded
         * prior to publishing the action. The images will not be marked as user-generated.
         * The action must already have been set prior to calling this method, and
         * the action must have a GraphObject-valued property with the specified property name. This method will
         * generate unique names for the image attachments and update the graph object to refer to these
         * attachments. Note that calling setObject again after calling this method, or modifying the value of the
         * specified property, will not clear the image attachments already set, but the new action (or objects)
         * will have no reference to the existing attachments.</p>
         * <p/>
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param objectProperty the name of a property on the action that corresponds to an Open Graph object;
         *                       the object must be marked as a new object to be created
         *                       (i.e., {@link com.facebook.model.OpenGraphObject#getCreateObject()} must return
         *                       true) or an exception will be thrown
         * @param bitmapFiles    a list of Bitmaps to be uploaded and attached to the Open Graph object
         * @return this instance of the builder
         */
        public CONCRETE setImageAttachmentFilesForObject(String objectProperty,
                List<File> bitmapFiles) {
            return setImageAttachmentFilesForObject(objectProperty, bitmapFiles, false);
        }

        /**
         * <p>Specifies a list of images for an Open Graph object referenced by the action that should be uploaded
         * prior to publishing the action. The images may be marked as being
         * user-generated -- refer to
         * <a href="https://developers.facebook.com/docs/opengraph/howtos/adding-photos-to-stories/">this article</a>
         * for more information.
         * The action must already have been set prior to calling this method, and
         * the action must have a GraphObject-valued property with the specified property name. This method will
         * generate unique names for the image attachments and update the graph object to refer to these
         * attachments. Note that calling setObject again after calling this method, or modifying the value of the
         * specified property, will not clear the image attachments already set, but the new action (or objects)
         * will have no reference to the existing attachments.</p>
         * <p/>
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param objectProperty  the name of a property on the action that corresponds to an Open Graph object;
         *                        the object must be marked as a new object to be created
         *                        (i.e., {@link com.facebook.model.OpenGraphObject#getCreateObject()} must return
         *                        true) or an exception will be thrown
         * @param bitmapFiles     a list of Bitmaps to be uploaded and attached to the Open Graph object
         * @param isUserGenerated if true, specifies that the user_generated flag should be set for these images
         * @return this instance of the builder
         */
        public CONCRETE setImageAttachmentFilesForObject(String objectProperty,
                List<File> bitmapFiles, boolean isUserGenerated) {
            Validate.notNull(objectProperty, "objectProperty");
            Validate.containsNoNulls(bitmapFiles, "bitmapFiles");
            if (action == null) {
                throw new FacebookException("Can not set attachments prior to setting action.");
            }

            List<String> attachmentUrls = addImageAttachmentFiles(bitmapFiles);
            updateObjectAttachmentUrls(objectProperty, attachmentUrls, isUserGenerated);

            @SuppressWarnings("unchecked")
            CONCRETE result = (CONCRETE) this;
            return result;
        }

        void updateObjectAttachmentUrls(String objectProperty, List<String> attachmentUrls, boolean isUserGenerated) {
            final OpenGraphObject object;
            try {
                object = action.getPropertyAs(objectProperty, OpenGraphObject.class);
                if (object == null) {
                    throw new IllegalArgumentException("Action does not contain a property '" + objectProperty + "'");
                }
            } catch (FacebookGraphObjectException exception) {
                throw new IllegalArgumentException("Property '" + objectProperty + "' is not a graph object");
            }
            if (!object.getCreateObject()) {
                throw new IllegalArgumentException(
                        "The Open Graph object in '" + objectProperty + "' is not marked for creation");
            }

            GraphObjectList<GraphObject> attachments = object.getImage();
            if (attachments == null) {
                attachments = GraphObject.Factory.createList(GraphObject.class);
            }
            for (String url : attachmentUrls) {
                GraphObject graphObject = GraphObject.Factory.create();
                graphObject.setProperty(NativeProtocol.IMAGE_URL_KEY, url);
                if (isUserGenerated) {
                    graphObject.setProperty(NativeProtocol.IMAGE_USER_GENERATED_KEY, true);
                }
                attachments.add(graphObject);
            }
            object.setImage(attachments);
        }

        @Override
        protected Bundle setBundleExtras(Bundle extras) {
            putExtra(extras, NativeProtocol.EXTRA_PREVIEW_PROPERTY_NAME, previewPropertyName);
            putExtra(extras, NativeProtocol.EXTRA_ACTION_TYPE, actionType);
            extras.putBoolean(NativeProtocol.EXTRA_DATA_FAILURES_FATAL, dataErrorsFatal);

            JSONObject jsonAction = action.getInnerJSONObject();
            jsonAction = flattenChildrenOfGraphObject(jsonAction);

            String jsonString = jsonAction.toString();
            putExtra(extras, NativeProtocol.EXTRA_ACTION, jsonString);

            return extras;
        }

        @Override
        protected Bundle getMethodArguments() {
            Bundle methodArgs = new Bundle();

            putExtra(methodArgs, NativeProtocol.METHOD_ARGS_PREVIEW_PROPERTY_NAME, previewPropertyName);
            putExtra(methodArgs, NativeProtocol.METHOD_ARGS_ACTION_TYPE, actionType);
            methodArgs.putBoolean(NativeProtocol.METHOD_ARGS_DATA_FAILURES_FATAL, dataErrorsFatal);

            JSONObject jsonAction = action.getInnerJSONObject();
            jsonAction = flattenChildrenOfGraphObject(jsonAction);

            String jsonString = jsonAction.toString();
            putExtra(methodArgs, NativeProtocol.METHOD_ARGS_ACTION, jsonString);

            return methodArgs;
        }

        private JSONObject flattenChildrenOfGraphObject(JSONObject graphObject) {
            try {
                // Clone the existing object to avoid modifying it from under the caller.
                graphObject = new JSONObject(graphObject.toString());

                @SuppressWarnings("unchecked")
                Iterator<String> keys = graphObject.keys();
                while (keys.hasNext()) {
                    String key = keys.next();
                    // The "image" property should not be flattened
                    if (!key.equalsIgnoreCase("image")) {
                        Object object = graphObject.get(key);

                        object = flattenObject(object);
                        graphObject.put(key, object);
                    }
                }

                return graphObject;
            } catch (JSONException e) {
                throw new FacebookException(e);
            }
        }

        private Object flattenObject(Object object) throws JSONException {
            if (object == null) {
                return null;
            }

            if (object instanceof JSONObject) {
                JSONObject jsonObject = (JSONObject) object;

                // Don't flatten objects that are marked as create_object.
                if (jsonObject.optBoolean(NativeProtocol.OPEN_GRAPH_CREATE_OBJECT_KEY)) {
                    return object;
                }
                if (jsonObject.has("id")) {
                    return jsonObject.getString("id");
                } else if (jsonObject.has("url")) {
                    return jsonObject.getString("url");
                }
            } else if (object instanceof JSONArray) {
                JSONArray jsonArray = (JSONArray) object;
                JSONArray newArray = new JSONArray();
                int length = jsonArray.length();

                for (int i = 0; i < length; ++i) {
                    newArray.put(flattenObject(jsonArray.get(i)));
                }

                return newArray;
            }

            return object;
        }
    }

    /**
     * Provides a builder which can construct a FacebookDialog instance suitable for presenting the native
     * Open Graph action publish dialog. This builder allows the caller to specify binary images for both the
     * action and any Open Graph objects to be created prior to publishing the action.
     * This builder will throw an exception if the Facebook application is not installed, so it
     * should only be used if {@link FacebookDialog#canPresentOpenGraphActionDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.OpenGraphActionDialogFeature...)} indicates the capability is available.
     */
    public static class OpenGraphActionDialogBuilder extends OpenGraphDialogBuilderBase<OpenGraphActionDialogBuilder> {
        /**
         * Constructor.
         *
         * @param activity            the Activity which is presenting the native Open Graph action publish dialog;
         *                            must not be null
         * @param action              the Open Graph action to be published, which must contain a reference to at least one
         *                            Open Graph object with the property name specified by setPreviewPropertyName; the action
         *                            must have had its type specified via the {@link OpenGraphAction#setType(String)} method
         * @param actionType          the type of the Open Graph action to be published, which should be the namespace-qualified
         *                            name of the action type (e.g., "myappnamespace:myactiontype"); this will override the type
         *                            of the action passed in.
         * @param previewPropertyName the name of a property on the Open Graph action that contains the
         *                            Open Graph object which will be displayed as a preview to the user
         */
        @Deprecated
        public OpenGraphActionDialogBuilder(Activity activity, OpenGraphAction action, String actionType,
                String previewPropertyName) {
            super(activity, action, actionType, previewPropertyName);
        }

        /**
         * Constructor.
         *
         * @param activity            the Activity which is presenting the native Open Graph action publish dialog;
         *                            must not be null
         * @param action              the Open Graph action to be published, which must contain a reference to at least one
         *                            Open Graph object with the property name specified by setPreviewPropertyName; the action
         *                            must have had its type specified via the {@link OpenGraphAction#setType(String)} method
         * @param previewPropertyName the name of a property on the Open Graph action that contains the
         *                            Open Graph object which will be displayed as a preview to the user
         */
        public OpenGraphActionDialogBuilder(Activity activity, OpenGraphAction action, String previewPropertyName) {
            super(activity, action, previewPropertyName);
        }

        @Override
        EnumSet<? extends DialogFeature> getDialogFeatures() {
            return EnumSet.of(OpenGraphActionDialogFeature.OG_ACTION_DIALOG);
        }
    }

    /**
     * Provides a builder which can construct a FacebookDialog instance suitable for presenting the native
     * Open Graph action message dialog. This builder allows the caller to specify binary images for both the
     * action and any Open Graph objects to be created prior to publishing the action.
     * This builder will throw an exception if the Facebook application is not installed, so it
     * should only be used if {@link FacebookDialog#canPresentOpenGraphMessageDialog(android.content.Context,
     * com.facebook.widget.FacebookDialog.OpenGraphMessageDialogFeature...)} indicates the capability is available.
     */
    public static class OpenGraphMessageDialogBuilder extends OpenGraphDialogBuilderBase<OpenGraphMessageDialogBuilder> {
        /**
         * Constructor.
         *
         * @param activity            the Activity which is presenting the native Open Graph action message dialog;
         *                            must not be null
         * @param action              the Open Graph action to be sent, which must contain a reference to at least one
         *                            Open Graph object with the property name specified by setPreviewPropertyName; the action
         *                            must have had its type specified via the {@link OpenGraphAction#setType(String)} method
         * @param previewPropertyName the name of a property on the Open Graph action that contains the
         *                            Open Graph object which will be displayed as a preview to the user
         */
        public OpenGraphMessageDialogBuilder(Activity activity, OpenGraphAction action, String previewPropertyName) {
            super(activity, action, previewPropertyName);
        }

        @Override
        EnumSet<? extends DialogFeature> getDialogFeatures() {
            return EnumSet.of(OpenGraphMessageDialogFeature.OG_MESSAGE_DIALOG);
        }
    }

    /**
     * Encapsulates information about a call being made to the Facebook application for Android. A unique String
     * call ID is used to track calls through their lifecycle.
     */
    public static class PendingCall implements Parcelable {
        private UUID callId;
        private Intent requestIntent;
        private int requestCode;

        /**
         * Constructor.
         *
         * @param requestCode the request code for this app call
         */
        public PendingCall(int requestCode) {
            callId = UUID.randomUUID();
            this.requestCode = requestCode;
        }

        private PendingCall(Parcel in) {
            callId = UUID.fromString(in.readString());
            requestIntent = in.readParcelable(null);
            requestCode = in.readInt();
        }

        private void setRequestIntent(Intent requestIntent) {
            this.requestIntent = requestIntent;
        }

        /**
         * Returns the Intent that was used to initiate this call to the Facebook application.
         *
         * @return the Intent
         */
        public Intent getRequestIntent() {
            return requestIntent;
        }

        /**
         * Returns the unique ID of this call to the Facebook application.
         *
         * @return the unique ID
         */
        public UUID getCallId() {
            return callId;
        }

        private void setRequestCode(int requestCode) {
            this.requestCode = requestCode;
        }

        /**
         * Gets the request code for this call.
         *
         * @return the request code that will be passed to handleActivityResult upon completion.
         */
        public int getRequestCode() {
            return requestCode;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel parcel, int i) {
            parcel.writeString(callId.toString());
            parcel.writeParcelable(requestIntent, 0);
            parcel.writeInt(requestCode);
        }

        public static final Creator<PendingCall> CREATOR
                = new Creator<PendingCall>() {
            public PendingCall createFromParcel(Parcel in) {
                return new PendingCall(in);
            }

            public PendingCall[] newArray(int size) {
                return new PendingCall[size];
            }
        };
    }
}
