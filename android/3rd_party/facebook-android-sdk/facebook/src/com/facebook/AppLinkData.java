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

package com.facebook;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import com.facebook.internal.NativeProtocol;
import com.facebook.internal.PlatformServiceClient;
import com.facebook.internal.Utility;
import com.facebook.internal.Validate;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Iterator;

/**
 * Class to encapsulate an app link, and provide methods for constructing the data from various sources
 */
public class AppLinkData {

    /**
     * Key that should be used to pull out the UTC Unix tap-time from the arguments for this app link.
     */
    public static final String ARGUMENTS_TAPTIME_KEY = "com.facebook.platform.APPLINK_TAP_TIME_UTC";
    /**
     * Key that should be used to get the "referer_data" field for this app link.
     */
    public static final String ARGUMENTS_REFERER_DATA_KEY = "referer_data";

    static final String BUNDLE_APPLINK_ARGS_KEY = "com.facebook.platform.APPLINK_ARGS";
    private static final String BUNDLE_AL_APPLINK_DATA_KEY = "al_applink_data";
    private static final String APPLINK_BRIDGE_ARGS_KEY = "bridge_args";
    private static final String APPLINK_METHOD_ARGS_KEY = "method_args";
    private static final String APPLINK_VERSION_KEY = "version";
    private static final String BRIDGE_ARGS_METHOD_KEY = "method";
    private static final String METHOD_ARGS_TARGET_URL_KEY = "target_url";
    private static final String METHOD_ARGS_REF_KEY = "ref";
    private static final String REFERER_DATA_REF_KEY = "fb_ref";
    private static final String TAG = AppLinkData.class.getCanonicalName();

    private String ref;
    private Uri targetUri;
    private JSONObject arguments;
    private Bundle argumentBundle;

    /**
     * Asynchronously fetches app link information that might have been stored for use
     * after installation of the app
     * @param context The context
     * @param completionHandler CompletionHandler to be notified with the AppLinkData object or null if none is
     *                          available.  Must not be null.
     */
    public static void fetchDeferredAppLinkData(Context context, CompletionHandler completionHandler) {
        fetchDeferredAppLinkData(context, null, completionHandler);
    }

    /**
     * Asynchronously fetches app link information that might have been stored for use
     * after installation of the app
     * @param context The context
     * @param applicationId Facebook application Id. If null, it is taken from the manifest
     * @param completionHandler CompletionHandler to be notified with the AppLinkData object or null if none is
     *                          available.  Must not be null.
     */
    public static void fetchDeferredAppLinkData(
            Context context,
            String applicationId,
            final CompletionHandler completionHandler) {
        Validate.notNull(context, "context");
        Validate.notNull(completionHandler, "completionHandler");

        if (applicationId == null) {
            applicationId = Utility.getMetadataApplicationId(context);
        }

        Validate.notNull(applicationId, "applicationId");

        DeferredAppLinkDataClient client = new DeferredAppLinkDataClient(context, applicationId);
        DeferredAppLinkDataClient.CompletedListener callback = new DeferredAppLinkDataClient.CompletedListener() {
            @Override
            public void completed(Bundle result) {
                AppLinkData appLinkData = null;
                if (result != null) {
                    final String appLinkArgsJsonString = result.getString(BUNDLE_APPLINK_ARGS_KEY);
                    final long tapTimeUtc = result.getLong(ARGUMENTS_TAPTIME_KEY, -1);

                    // Now create the app link
                    appLinkData = createFromJson(appLinkArgsJsonString);
                    if (tapTimeUtc != -1) {
                        try {
                            if (appLinkData.arguments != null) {
                                appLinkData.arguments.put(ARGUMENTS_TAPTIME_KEY, tapTimeUtc);
                            }
                            if (appLinkData.argumentBundle != null) {
                                appLinkData.argumentBundle.putString(ARGUMENTS_TAPTIME_KEY, Long.toString(tapTimeUtc));
                            }
                        } catch (JSONException e) {
                            Log.d(TAG, "Unable to put tap time in AppLinkData.arguments");
                        }
                    }
                }
                completionHandler.onDeferredAppLinkDataFetched(appLinkData);
            }
        };
        client.setCompletedListener(callback);

        if (!client.start()) {
            // there is not a sufficient version of fb4a present to return a deferred app link, so kick off
            // a call to the completion handler.
            Handler handler = new Handler(Looper.getMainLooper());
            handler.post(new Runnable() {
                @Override
                public void run() {
                    completionHandler.onDeferredAppLinkDataFetched(null);
                }
            });
        }
    }

    /**
     * Parses out any app link data from the Intent of the Activity passed in.
     * @param activity Activity that was started because of an app link
     * @return AppLinkData if found. null if not.
     */
    public static AppLinkData createFromActivity(Activity activity) {
        Validate.notNull(activity, "activity");
        Intent intent = activity.getIntent();
        if (intent == null) {
            return null;
        }

        AppLinkData appLinkData = createFromAlApplinkData(intent);
        if (appLinkData == null) {
            String appLinkArgsJsonString = intent.getStringExtra(BUNDLE_APPLINK_ARGS_KEY);
            appLinkData = createFromJson(appLinkArgsJsonString);
        }
        if (appLinkData == null) {
            // Try regular app linking
            appLinkData = createFromUri(intent.getData());
        }

        return appLinkData;
    }

    private static AppLinkData createFromAlApplinkData(Intent intent) {
        Bundle applinks = intent.getBundleExtra(BUNDLE_AL_APPLINK_DATA_KEY);
        if (applinks == null) {
            return null;
        }

        AppLinkData appLinkData = new AppLinkData();
        appLinkData.targetUri = intent.getData();
        if (appLinkData.targetUri == null) {
            String targetUriString = applinks.getString(METHOD_ARGS_TARGET_URL_KEY);
            if (targetUriString != null) {
                appLinkData.targetUri = Uri.parse(targetUriString);
            }
        }
        appLinkData.argumentBundle = applinks;
        appLinkData.arguments = null;
        Bundle refererData = applinks.getBundle(ARGUMENTS_REFERER_DATA_KEY);
        if (refererData != null) {
            appLinkData.ref = refererData.getString(REFERER_DATA_REF_KEY);
        }

        return appLinkData;
    }

    private static AppLinkData createFromJson(String jsonString) {
        if (jsonString  == null) {
            return null;
        }

        try {
            // Any missing or malformed data will result in a JSONException
            JSONObject appLinkArgsJson = new JSONObject(jsonString);
            String version = appLinkArgsJson.getString(APPLINK_VERSION_KEY);

            JSONObject bridgeArgs = appLinkArgsJson.getJSONObject(APPLINK_BRIDGE_ARGS_KEY);
            String method = bridgeArgs.getString(BRIDGE_ARGS_METHOD_KEY);
            if (method.equals("applink") && version.equals("2")) {
                // We have a new deep link
                AppLinkData appLinkData = new AppLinkData();

                appLinkData.arguments = appLinkArgsJson.getJSONObject(APPLINK_METHOD_ARGS_KEY);
                // first look for the "ref" key in the top level args
                if (appLinkData.arguments.has(METHOD_ARGS_REF_KEY)) {
                    appLinkData.ref = appLinkData.arguments.getString(METHOD_ARGS_REF_KEY);
                } else if (appLinkData.arguments.has(ARGUMENTS_REFERER_DATA_KEY)) {
                    // if it's not in the top level args, it could be in the "referer_data" blob
                    JSONObject refererData = appLinkData.arguments.getJSONObject(ARGUMENTS_REFERER_DATA_KEY);
                    if (refererData.has(REFERER_DATA_REF_KEY)) {
                        appLinkData.ref = refererData.getString(REFERER_DATA_REF_KEY);
                    }
                }

                if (appLinkData.arguments.has(METHOD_ARGS_TARGET_URL_KEY)) {
                    appLinkData.targetUri = Uri.parse(appLinkData.arguments.getString(METHOD_ARGS_TARGET_URL_KEY));
                }

                appLinkData.argumentBundle = toBundle(appLinkData.arguments);

                return appLinkData;
            }
        } catch (JSONException e) {
            Log.d(TAG, "Unable to parse AppLink JSON", e);
        } catch (FacebookException e) {
            Log.d(TAG, "Unable to parse AppLink JSON", e);
        }

        return null;
    }

    private static AppLinkData createFromUri(Uri appLinkDataUri) {
        if (appLinkDataUri == null) {
            return null;
        }

        AppLinkData appLinkData = new AppLinkData();
        appLinkData.targetUri = appLinkDataUri;
        return appLinkData;
    }

    private static Bundle toBundle(JSONObject node) throws JSONException {
        Bundle bundle = new Bundle();
        @SuppressWarnings("unchecked")
        Iterator<String> fields = node.keys();
        while (fields.hasNext()) {
            String key = fields.next();
            Object value;
            value = node.get(key);

            if (value instanceof JSONObject) {
                bundle.putBundle(key, toBundle((JSONObject) value));
            } else if (value instanceof JSONArray) {
                JSONArray valueArr = (JSONArray) value;
                if (valueArr.length() == 0) {
                    bundle.putStringArray(key, new String[0]);
                } else {
                    Object firstNode = valueArr.get(0);
                    if (firstNode instanceof JSONObject) {
                        Bundle[] bundles = new Bundle[valueArr.length()];
                        for (int i = 0; i < valueArr.length(); i++) {
                            bundles[i] = toBundle(valueArr.getJSONObject(i));
                        }
                        bundle.putParcelableArray(key, bundles);
                    } else if (firstNode instanceof JSONArray) {
                        // we don't support nested arrays
                        throw new FacebookException("Nested arrays are not supported.");
                    } else { // just use the string value
                        String[] arrValues = new String[valueArr.length()];
                        for (int i = 0; i < valueArr.length(); i++) {
                            arrValues[i] = valueArr.get(i).toString();
                        }
                        bundle.putStringArray(key, arrValues);
                    }
                }
            } else {
                bundle.putString(key, value.toString());
            }
        }
        return bundle;
    }


    private AppLinkData() {
    }

    /**
     * Returns the target uri for this App Link.
     * @return target uri
     */
    public Uri getTargetUri() {
        return targetUri;
    }

    /**
     * Returns the ref for this App Link.
     * @return ref
     */
    public String getRef() {
        return ref;
    }

    /**
     * This method has been deprecated. Please use {@link AppLinkData#getArgumentBundle()} instead.
     * @return JSONObject property bag.
     */
    @Deprecated
    public JSONObject getArguments() {
        return arguments;
    }

    /**
     * The full set of arguments for this app link. Properties like target uri & ref are typically
     * picked out of this set of arguments.
     * @return App link related arguments as a bundle.
     */
    public Bundle getArgumentBundle() {
        return argumentBundle;
    }

    /**
     * The referer data associated with the app link. This will contain Facebook specific information like
     * fb_access_token, fb_expires_in, and fb_ref.
     * @return the referer data.
     */
    public Bundle getRefererData() {
        if (argumentBundle != null) {
            return argumentBundle.getBundle(ARGUMENTS_REFERER_DATA_KEY);
        }
        return null;
    }

    /**
     * Interface to asynchronously receive AppLinkData after it has been fetched.
     */
    public interface CompletionHandler {
        /**
         * This method is called when deferred app link data has been fetched. If no app link data was found,
         * this method is called with null
         * @param appLinkData The app link data that was fetched. Null if none was found.
         */
        void onDeferredAppLinkDataFetched(AppLinkData appLinkData);
    }

    final static class DeferredAppLinkDataClient extends PlatformServiceClient {

        DeferredAppLinkDataClient(Context context, String applicationId) {
            super(context, NativeProtocol.MESSAGE_GET_INSTALL_DATA_REQUEST, NativeProtocol.MESSAGE_GET_INSTALL_DATA_REPLY,
                    NativeProtocol.PROTOCOL_VERSION_20130618, applicationId);
        }

        @Override
        protected void populateRequestBundle(Bundle data) {
            String packageName = getContext().getPackageName();
            data.putString(NativeProtocol.EXTRA_GET_INSTALL_DATA_PACKAGE, packageName);
        }
    }
}
