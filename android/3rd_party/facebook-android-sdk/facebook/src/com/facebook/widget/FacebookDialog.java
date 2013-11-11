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
import com.facebook.internal.NativeProtocol;
import com.facebook.internal.Utility;
import com.facebook.internal.Validate;
import com.facebook.model.*;
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

    private static final int MIN_NATIVE_SHARE_PROTOCOL_VERSION = NativeProtocol.PROTOCOL_VERSION_20130618;

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
         * @param data  a Bundle containing the results of the dialog, whose contents will vary depending on the
         *              type of dialog being displayed.
         */
        void onComplete(PendingCall pendingCall, Bundle data);

        /**
         * Called if an error occurred while presenting a Facebook dialog.
         *
         * @param pendingCall a PendingCall containing the call ID and original Intent used to launch the dialog
         * @param error the error that occurred
         * @param data the full set of extras associated with the activity result
         */
        void onError(PendingCall pendingCall, Exception error, Bundle data);
    }

    private interface DialogFeature {
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
        SHARE_DIALOG(NativeProtocol.PROTOCOL_VERSION_20130618);

        private int minVersion;

        private ShareDialogFeature(int minVersion) {
            this.minVersion = minVersion;
        }

        /**
         * This method is for internal use only.
         */
        public int getMinVersion() {
            return minVersion;
        }
    }

    /**
     * Defines a set of features that may be supported by the native Open Graph action dialog exposed by the Facebook
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

    private FacebookDialog(Activity activity, Fragment fragment, PendingCall appCall, OnPresentCallback onPresentCallback) {
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
     * @param context the Context that is handling the activity result
     * @param appCall an PendingCall containing the call ID and original Intent used to launch the dialog
     * @param requestCode the request code for the activity result
     * @param data the result Intent
     * @param callback a callback to call after parsing the results
     *
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
                callback.onError(appCall, error, data.getExtras());
            } else {
                callback.onComplete(appCall, data.getExtras());
            }
        }

        return true;
    }

    /**
     * Determines whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features of the native Share dialog, which in turn may be used to determine
     * which UI, etc., to present to the user.
     *
     * @param context the calling Context
     * @param features zero or more features to check for; {@link ShareDialogFeature#SHARE_DIALOG} is implicitly checked
     *                 if not explicitly specified
     * @return true if all of the specified features are supported by the currently installed version of the
     * Facebook application; false if any of the features are not supported
     */
    public static boolean canPresentShareDialog(Context context, ShareDialogFeature... features) {
        return handleCanPresent(context, EnumSet.of(ShareDialogFeature.SHARE_DIALOG, features));
    }

    /**
     * Determines whether the version of the Facebook application installed on the user's device is recent
     * enough to support specific features of the native Open Graph action dialog, which in turn may be used to
     * determine which UI, etc., to present to the user.
     *
     * @param context the calling Context
     * @param features zero or more features to check for; {@link OpenGraphActionDialogFeature#OG_ACTION_DIALOG} is implicitly
     *                 checked if not explicitly specified
     * @return true if all of the specified features are supported by the currently installed version of the
     * Facebook application; false if any of the features are not supported
     */
    public static boolean canPresentOpenGraphActionDialog(Context context, OpenGraphActionDialogFeature... features) {
        return handleCanPresent(context, EnumSet.of(OpenGraphActionDialogFeature.OG_ACTION_DIALOG, features));
    }

    private static boolean handleCanPresent(Context context, Iterable<? extends DialogFeature> features) {
        return getProtocolVersionForNativeDialog(context, getMinVersionForFeatures(features))
                != NativeProtocol.NO_PROTOCOL_AVAILABLE;
    }

    private static int getProtocolVersionForNativeDialog(Context context, Integer requiredVersion) {
        return NativeProtocol.getLatestAvailableProtocolVersion(context, requiredVersion);
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

    private abstract static class Builder<CONCRETE extends Builder<?>> {
        final protected Activity activity;
        final protected String applicationId;
        final protected PendingCall appCall;
        protected Fragment fragment;
        protected String applicationName;

        Builder(Activity activity) {
            Validate.notNull(activity, "activity");

            this.activity = activity;
            applicationId = Utility.getMetadataApplicationId(activity);
            appCall = new PendingCall(NativeProtocol.DIALOG_REQUEST_CODE);
        }

        /**
         * Sets the request code that will be passed to handleActivityResult when this activity completes; the
         * default is NativeProtocol.DIALOG_REQUEST_CODE.
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
         * @return a FacebookDialog instance
         */
        public FacebookDialog build() {
            validate();

            Bundle extras = new Bundle();
            putExtra(extras, NativeProtocol.EXTRA_APPLICATION_ID, applicationId);
            putExtra(extras, NativeProtocol.EXTRA_APPLICATION_NAME, applicationName);

            Intent intent = handleBuild(extras);
            if (intent == null) {
                throw new FacebookException("Unable to create Intent; this likely means the Facebook app is not installed.");
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
            return handleCanPresent();
        }

        boolean handleCanPresent() {
            return getProtocolVersionForNativeDialog(activity, MIN_NATIVE_SHARE_PROTOCOL_VERSION)
                    != NativeProtocol.NO_PROTOCOL_AVAILABLE;
        }

        void validate() {}

        OnPresentCallback getOnPresentCallback() {
            return null;
        }

        abstract Intent handleBuild(Bundle extras);

        void putExtra(Bundle extras, String key, String value) {
            if (value != null) {
                extras.putString(key, value);
            }
        }
    }

    /**
     * Provides a builder which can construct a FacebookDialog instance suitable for presenting the native
     * Share dialog. This builder will throw an exception if the Facebook application is not installed, so it
     * should only be used if {@link FacebookDialog.checkCanPresentShareDialog()} indicates the capability
     * is available.
     */
    public static class ShareDialogBuilder extends Builder<ShareDialogBuilder> {
        private String name;
        private String caption;
        private String description;
        private String link;
        private String picture;
        private String place;
        private ArrayList<String> friends;
        private String ref;
        private boolean dataErrorsFatal;

        /**
         * Constructor.
         * @param activity the Activity which is presenting the native Share dialog; must not be null
         */
        public ShareDialogBuilder(Activity activity) {
            super(activity);
        }

        /**
         * Sets the title of the item to be shared.
         * @param name the title
         * @return this instance of the builder
         */
        public ShareDialogBuilder setName(String name) {
            this.name = name;
            return this;
        }

        /**
         * Sets the subtitle of the item to be shared.
         * @param caption the subtitle
         * @return this instance of the builder
         */
        public ShareDialogBuilder setCaption(String caption) {
            this.caption = caption;
            return this;
        }

        /**
         * Sets the description of the item to be shared.
         * @param description the description
         * @return this instance of the builder
         */
        public ShareDialogBuilder setDescription(String description) {
            this.description = description;
            return this;
        }

        /**
         * Sets the URL of the item to be shared.
         * @param link the URL
         * @return this instance of the builder
         */
        public ShareDialogBuilder setLink(String link) {
            this.link = link;
            return this;
        }

        /**
         * Sets the URL of the image of the item to be shared.
         * @param picture the URL of the image
         * @return this instance of the builder
         */
        public ShareDialogBuilder setPicture(String picture) {
            this.picture = picture;
            return this;
        }

        /**
         * Sets the place for the item to be shared.
         * @param place the Facebook ID of the place
         * @return this instance of the builder
         */
        public ShareDialogBuilder setPlace(String place) {
            this.place = place;
            return this;
        }

        /**
         * Sets the tagged friends for the item to be shared.
         * @param friends a list of Facebook IDs of the friends to be tagged in the shared item
         * @return this instance of the builder
         */
        public ShareDialogBuilder setFriends(List<String> friends) {
            this.friends = new ArrayList<String>(friends);
            return this;
        }

        /**
         * Sets the 'ref' property of the item to be shared.
         * @param ref the 'ref' property
         * @return this instance of the builder
         */
        public ShareDialogBuilder setRef(String ref) {
            this.ref = ref;
            return this;
        }

        /**
         * Sets whether errors encountered during previewing the shared item should be considered fatal and
         * cause the dialog to return an error
         * @param dataErrorsFatal true if data errors should be fatal; false if not
         * @return this instance of the builder
         */
        public ShareDialogBuilder setDataErrorsFatal(boolean dataErrorsFatal) {
            this.dataErrorsFatal = dataErrorsFatal;
            return this;
        }

        @Override
        boolean handleCanPresent() {
            return canPresentShareDialog(activity, ShareDialogFeature.SHARE_DIALOG);
        }

        @Override
        Intent handleBuild(Bundle extras) {
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

            int protocolVersion = getProtocolVersionForNativeDialog(activity, MIN_NATIVE_SHARE_PROTOCOL_VERSION);

            Intent intent = NativeProtocol.createPlatformActivityIntent(activity, NativeProtocol.ACTION_FEED_DIALOG,
                    protocolVersion, extras);
            return intent;
        }
    }

    /**
     * Provides a builder which can construct a FacebookDialog instance suitable for presenting the native
     * Open Graph action publish dialog. This builder allows the caller to specify binary images for both the
     * action and any Open Graph objects to be created prior to publishing the action.
     * This builder will throw an exception if the Facebook application is not installed, so it
     * should only be used if {@link FacebookDialog.checkCanPresentOpenGraphDialog();} indicates the capability
     * is available.
     */
    public static class OpenGraphActionDialogBuilder extends Builder<OpenGraphActionDialogBuilder> {
        private String previewPropertyName;
        private OpenGraphAction action;
        private String actionType;
        private HashMap<String, Bitmap> imageAttachments;
        private HashMap<String, File> imageAttachmentFiles;
        private boolean dataErrorsFatal;

        /**
         * Constructor.
         * @param activity the Activity which is presenting the native Open Graph action publish dialog;
         *                 must not be null
         * @param action the Open Graph action to be published, which must contain a reference to at least one
         *               Open Graph object with the property name specified by setPreviewPropertyName; the action
         *               must have had its type specified via the {@link OpenGraphAction#setType(String)} method
         * @param actionType the type of the Open Graph action to be published, which should be the namespace-qualified
         *                   name of the action type (e.g., "myappnamespace:myactiontype"); this will override the type
         *                   of the action passed in.
         * @param previewPropertyName the name of a property on the Open Graph action that contains the
         *                            Open Graph object which will be displayed as a preview to the user
         */
        @Deprecated
        public OpenGraphActionDialogBuilder(Activity activity, OpenGraphAction action, String actionType,
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
                        "Consider using OpenGraphActionDialogBuilder(Activity activity, OpenGraphAction action, " +
                        "String previewPropertyName) instead.");
            }
            this.action = action;
            this.actionType = actionType;
            this.previewPropertyName = previewPropertyName;
        }

        /**
         * Constructor.
         * @param activity the Activity which is presenting the native Open Graph action publish dialog;
         *                 must not be null
         * @param action the Open Graph action to be published, which must contain a reference to at least one
         *               Open Graph object with the property name specified by setPreviewPropertyName; the action
         *               must have had its type specified via the {@link OpenGraphAction#setType(String)} method
         * @param previewPropertyName the name of a property on the Open Graph action that contains the
         *                            Open Graph object which will be displayed as a preview to the user
         */
        public OpenGraphActionDialogBuilder(Activity activity, OpenGraphAction action, String previewPropertyName) {
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
         * @param dataErrorsFatal true if data errors should be fatal; false if not
         * @return this instance of the builder
         */
        public OpenGraphActionDialogBuilder setDataErrorsFatal(boolean dataErrorsFatal) {
            this.dataErrorsFatal = dataErrorsFatal;
            return this;
        }

        /**
         * <p>Specifies a list of images for the Open Graph action that should be uploaded prior to publishing the
         * action. The action must already have been set prior to calling this method. This method will generate unique
         * names for the image attachments and update the action to refer to these attachments. Note that calling
         * setAction again after calling this method will not clear the image attachments already set, but the new
         * action will have no reference to the existing attachments. The images will not be marked as being
         * user-generated.</p>
         *
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param bitmaps a list of Bitmaps to be uploaded and attached to the Open Graph action
         * @return this instance of the builder
         */
        public OpenGraphActionDialogBuilder setImageAttachmentsForAction(List<Bitmap> bitmaps) {
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
         *
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param bitmaps a list of Bitmaps to be uploaded and attached to the Open Graph action
         * @param isUserGenerated if true, specifies that the user_generated flag should be set for these images
         * @return this instance of the builder
         */
        public OpenGraphActionDialogBuilder setImageAttachmentsForAction(List<Bitmap> bitmaps,
                boolean isUserGenerated) {
            Validate.containsNoNulls(bitmaps, "bitmaps");
            if (action == null) {
                throw new FacebookException("Can not set attachments prior to setting action.");
            }

            List<String> attachmentUrls = addImageAttachments(bitmaps);
            updateActionAttachmentUrls(attachmentUrls, isUserGenerated);

            return this;
        }

        /**
         * <p>Specifies a list of images for the Open Graph action that should be uploaded prior to publishing the
         * action. The action must already have been set prior to calling this method.  The images will not be marked
         * as being user-generated. This method will generate unique names for the image attachments and update the
         * action to refer to these attachments. Note that calling setAction again after calling this method will
         * not clear the image attachments already set, but the new action will have no reference to the existing
         * attachments.</p>
         *
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param bitmapFiles a list of Files containing bitmaps to be uploaded and attached to the Open Graph action
         * @return this instance of the builder
         */
        public OpenGraphActionDialogBuilder setImageAttachmentFilesForAction(List<File> bitmapFiles) {
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
         *
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param bitmapFiles a list of Files containing bitmaps to be uploaded and attached to the Open Graph action
         * @param isUserGenerated if true, specifies that the user_generated flag should be set for these images
         * @return this instance of the builder
         */
        public OpenGraphActionDialogBuilder setImageAttachmentFilesForAction(List<File> bitmapFiles,
                boolean isUserGenerated) {
            Validate.containsNoNulls(bitmapFiles, "bitmapFiles");
            if (action == null) {
                throw new FacebookException("Can not set attachments prior to setting action.");
            }

            List<String> attachmentUrls = addImageAttachmentFiles(bitmapFiles);
            updateActionAttachmentUrls(attachmentUrls, isUserGenerated);

            return this;
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
         *
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param objectProperty the name of a property on the action that corresponds to an Open Graph object;
         *                       the object must be marked as a new object to be created
         *                       (i.e., {@link com.facebook.model.OpenGraphObject#getCreateObject()} must return
         *                       true) or an exception will be thrown
         * @param bitmapFiles a list of Files containing bitmaps to be uploaded and attached to the Open Graph object
         * @return this instance of the builder
         */
        public OpenGraphActionDialogBuilder setImageAttachmentsForObject(String objectProperty, List<Bitmap> bitmaps) {
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
         *
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param objectProperty the name of a property on the action that corresponds to an Open Graph object;
         *                       the object must be marked as a new object to be created
         *                       (i.e., {@link com.facebook.model.OpenGraphObject#getCreateObject()} must return
         *                       true) or an exception will be thrown
         * @param objectProperty the name of a property on the action that corresponds to an Open Graph object
         * @param bitmapFiles a list of Files containing bitmaps to be uploaded and attached to the Open Graph object
         * @param isUserGenerated if true, specifies that the user_generated flag should be set for these images
         * @return this instance of the builder
         */
        public OpenGraphActionDialogBuilder setImageAttachmentsForObject(String objectProperty, List<Bitmap> bitmaps,
                boolean isUserGenerated) {
            Validate.notNull(objectProperty, "objectProperty");
            Validate.containsNoNulls(bitmaps, "bitmaps");
            if (action == null) {
                throw new FacebookException("Can not set attachments prior to setting action.");
            }

            List<String> attachmentUrls = addImageAttachments(bitmaps);
            updateObjectAttachmentUrls(objectProperty, attachmentUrls, isUserGenerated);

            return this;
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
         *
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param objectProperty the name of a property on the action that corresponds to an Open Graph object;
         *                       the object must be marked as a new object to be created
         *                       (i.e., {@link com.facebook.model.OpenGraphObject#getCreateObject()} must return
         *                       true) or an exception will be thrown
         * @param bitmaps a list of Bitmaps to be uploaded and attached to the Open Graph object
         * @return this instance of the builder
         */
        public OpenGraphActionDialogBuilder setImageAttachmentFilesForObject(String objectProperty,
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
         *
         * <p>In order for the images to be provided to the Facebook application as part of the app call, the
         * NativeAppCallContentProvider must be specified correctly in the application's AndroidManifest.xml.</p>
         *
         * @param objectProperty the name of a property on the action that corresponds to an Open Graph object;
         *                       the object must be marked as a new object to be created
         *                       (i.e., {@link com.facebook.model.OpenGraphObject#getCreateObject()} must return
         *                       true) or an exception will be thrown
         * @param bitmaps a list of Bitmaps to be uploaded and attached to the Open Graph object
         * @param isUserGenerated if true, specifies that the user_generated flag should be set for these images
         * @return this instance of the builder
         */
        public OpenGraphActionDialogBuilder setImageAttachmentFilesForObject(String objectProperty,
                List<File> bitmapFiles, boolean isUserGenerated) {
            Validate.notNull(objectProperty, "objectProperty");
            Validate.containsNoNulls(bitmapFiles, "bitmapFiles");
            if (action == null) {
                throw new FacebookException("Can not set attachments prior to setting action.");
            }

            List<String> attachmentUrls = addImageAttachmentFiles(bitmapFiles);
            updateObjectAttachmentUrls(objectProperty, attachmentUrls, isUserGenerated);

            return this;
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

        private List<String> addImageAttachments(List<Bitmap> bitmaps) {
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

        private List<String> addImageAttachmentFiles(List<File> bitmapFiles) {
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

        @Override
        boolean handleCanPresent() {
            return canPresentOpenGraphActionDialog(activity, OpenGraphActionDialogFeature.OG_ACTION_DIALOG);
        }

        @Override
        Intent handleBuild(Bundle extras)  {
            putExtra(extras, NativeProtocol.EXTRA_PREVIEW_PROPERTY_NAME, previewPropertyName);
            putExtra(extras, NativeProtocol.EXTRA_ACTION_TYPE, actionType);
            extras.putBoolean(NativeProtocol.EXTRA_DATA_FAILURES_FATAL, dataErrorsFatal);

            JSONObject jsonAction = action.getInnerJSONObject();
            jsonAction = flattenChildrenOfGraphObject(jsonAction);

            String jsonString = jsonAction.toString();
            putExtra(extras, NativeProtocol.EXTRA_ACTION, jsonString);

            int protocolVersion = getProtocolVersionForNativeDialog(activity, MIN_NATIVE_SHARE_PROTOCOL_VERSION);

            Intent intent = NativeProtocol.createPlatformActivityIntent(activity,
                    NativeProtocol.ACTION_OGACTIONPUBLISH_DIALOG, protocolVersion, extras);

            return intent;
        }

        @Override
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

        private OpenGraphActionDialogBuilder addImageAttachment(String imageName, Bitmap bitmap) {
            if (imageAttachments == null) {
                imageAttachments = new HashMap<String, Bitmap>();
            }
            imageAttachments.put(imageName, bitmap);
            return this;
        }

        private OpenGraphActionDialogBuilder addImageAttachment(String imageName, File attachment) {
            if (imageAttachmentFiles == null) {
                imageAttachmentFiles = new HashMap<String, File>();
            }
            imageAttachmentFiles.put(imageName, attachment);
            return this;
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
     * Encapsulates information about a call being made to the Facebook application for Android. A unique String
     * call ID is used to track calls through their lifecycle.
     */
    public static class PendingCall implements Parcelable {
        private UUID callId;
        private Intent requestIntent;
        private int requestCode;

        /**
         * Constructor.
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
            this.requestIntent.putExtra(NativeProtocol.EXTRA_PROTOCOL_CALL_ID, callId.toString());
        }

        /**
         * Returns the Intent that was used to initiate this call to the Facebook application.
         * @return the Intent
         */
        public Intent getRequestIntent() {
            return requestIntent;
        }

        /**
         * Returns the unique ID of this call to the Facebook application.
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
        };}
}
