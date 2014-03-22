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

package com.facebook.internal;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.provider.Settings.Secure;
import android.text.TextUtils;
import android.util.Log;
import android.webkit.CookieManager;
import android.webkit.CookieSyncManager;
import com.facebook.*;
import com.facebook.android.BuildConfig;
import com.facebook.model.GraphObject;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URLConnection;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

/**
 * com.facebook.internal is solely for the use of other packages within the Facebook SDK for Android. Use of
 * any of the classes in this package is unsupported, and they may be modified or removed without warning at
 * any time.
 */
public final class Utility {
    static final String LOG_TAG = "FacebookSDK";
    private static final String HASH_ALGORITHM_MD5 = "MD5";
    private static final String HASH_ALGORITHM_SHA1 = "SHA-1";
    private static final String URL_SCHEME = "https";
    private static final String SUPPORTS_ATTRIBUTION = "supports_attribution";
    private static final String SUPPORTS_IMPLICIT_SDK_LOGGING = "supports_implicit_sdk_logging";
    private static final String [] APP_SETTING_FIELDS = new String[] {
            SUPPORTS_ATTRIBUTION,
            SUPPORTS_IMPLICIT_SDK_LOGGING
    };
    private static final String APPLICATION_FIELDS = "fields";

    // This is the default used by the buffer streams, but they trace a warning if you do not specify.
    public static final int DEFAULT_STREAM_BUFFER_SIZE = 8192;

    private static Map<String, FetchedAppSettings> fetchedAppSettings =
            new ConcurrentHashMap<String, FetchedAppSettings>();

    public static class FetchedAppSettings {
        private boolean supportsAttribution;
        private boolean supportsImplicitLogging;

        private FetchedAppSettings(boolean supportsAttribution, boolean supportsImplicitLogging) {
            this.supportsAttribution = supportsAttribution;
            this.supportsImplicitLogging = supportsImplicitLogging;
        }

        public boolean supportsAttribution() {
            return supportsAttribution;
        }

        public boolean supportsImplicitLogging() {
            return supportsImplicitLogging;
        }
    }

    // Returns true iff all items in subset are in superset, treating null and
    // empty collections as
    // the same.
    public static <T> boolean isSubset(Collection<T> subset, Collection<T> superset) {
        if ((superset == null) || (superset.size() == 0)) {
            return ((subset == null) || (subset.size() == 0));
        }

        HashSet<T> hash = new HashSet<T>(superset);
        for (T t : subset) {
            if (!hash.contains(t)) {
                return false;
            }
        }
        return true;
    }

    public static <T> boolean isNullOrEmpty(Collection<T> c) {
        return (c == null) || (c.size() == 0);
    }

    public static boolean isNullOrEmpty(String s) {
        return (s == null) || (s.length() == 0);
    }

    public static <T> Collection<T> unmodifiableCollection(T... ts) {
        return Collections.unmodifiableCollection(Arrays.asList(ts));
    }

    public static <T> ArrayList<T> arrayList(T... ts) {
        ArrayList<T> arrayList = new ArrayList<T>(ts.length);
        for (T t : ts) {
            arrayList.add(t);
        }
        return arrayList;
    }

    static String md5hash(String key) {
        return hashWithAlgorithm(HASH_ALGORITHM_MD5, key);
    }

    private static String sha1hash(String key) {
        return hashWithAlgorithm(HASH_ALGORITHM_SHA1, key);
    }

    private static String hashWithAlgorithm(String algorithm, String key) {
        MessageDigest hash = null;
        try {
            hash = MessageDigest.getInstance(algorithm);
        } catch (NoSuchAlgorithmException e) {
            return null;
        }

        hash.update(key.getBytes());
        byte[] digest = hash.digest();
        StringBuilder builder = new StringBuilder();
        for (int b : digest) {
            builder.append(Integer.toHexString((b >> 4) & 0xf));
            builder.append(Integer.toHexString((b >> 0) & 0xf));
        }
        return builder.toString();
    }

    public static Uri buildUri(String authority, String path, Bundle parameters) {
        Uri.Builder builder = new Uri.Builder();
        builder.scheme(URL_SCHEME);
        builder.authority(authority);
        builder.path(path);
        for (String key : parameters.keySet()) {
            Object parameter = parameters.get(key);
            if (parameter instanceof String) {
                builder.appendQueryParameter(key, (String) parameter);
            }
        }
        return builder.build();
    }

    public static void putObjectInBundle(Bundle bundle, String key, Object value) {
        if (value instanceof String) {
            bundle.putString(key, (String) value);
        } else if (value instanceof Parcelable) {
            bundle.putParcelable(key, (Parcelable) value);
        } else if (value instanceof byte[]) {
            bundle.putByteArray(key, (byte[]) value);
        } else {
            throw new FacebookException("attempted to add unsupported type to Bundle");
        }
    }

    public static void closeQuietly(Closeable closeable) {
        try {
            if (closeable != null) {
                closeable.close();
            }
        } catch (IOException ioe) {
            // ignore
        }
    }

    public static void disconnectQuietly(URLConnection connection) {
        if (connection instanceof HttpURLConnection) {
            ((HttpURLConnection)connection).disconnect();
        }
    }

    public static String getMetadataApplicationId(Context context) {
        Validate.notNull(context, "context");

        try {
            ApplicationInfo ai = context.getPackageManager().getApplicationInfo(
                    context.getPackageName(), PackageManager.GET_META_DATA);
            if (ai.metaData != null) {
                return ai.metaData.getString(Session.APPLICATION_ID_PROPERTY);
            }
        } catch (PackageManager.NameNotFoundException e) {
            // if we can't find it in the manifest, just return null
        }

        return null;
    }

    static Map<String, Object> convertJSONObjectToHashMap(JSONObject jsonObject) {
        HashMap<String, Object> map = new HashMap<String, Object>();
        JSONArray keys = jsonObject.names();
        for (int i = 0; i < keys.length(); ++i) {
            String key;
            try {
                key = keys.getString(i);
                Object value = jsonObject.get(key);
                if (value instanceof JSONObject) {
                    value = convertJSONObjectToHashMap((JSONObject) value);
                }
                map.put(key, value);
            } catch (JSONException e) {
            }
        }
        return map;
    }

    // Returns either a JSONObject or JSONArray representation of the 'key' property of 'jsonObject'.
    public static Object getStringPropertyAsJSON(JSONObject jsonObject, String key, String nonJSONPropertyKey)
            throws JSONException {
        Object value = jsonObject.opt(key);
        if (value != null && value instanceof String) {
            JSONTokener tokener = new JSONTokener((String) value);
            value = tokener.nextValue();
        }

        if (value != null && !(value instanceof JSONObject || value instanceof JSONArray)) {
            if (nonJSONPropertyKey != null) {
                // Facebook sometimes gives us back a non-JSON value such as
                // literal "true" or "false" as a result.
                // If we got something like that, we present it to the caller as
                // a GraphObject with a single
                // property. We only do this if the caller wants that behavior.
                jsonObject = new JSONObject();
                jsonObject.putOpt(nonJSONPropertyKey, value);
                return jsonObject;
            } else {
                throw new FacebookException("Got an unexpected non-JSON object.");
            }
        }

        return value;

    }

    public static String readStreamToString(InputStream inputStream) throws IOException {
        BufferedInputStream bufferedInputStream = null;
        InputStreamReader reader = null;
        try {
            bufferedInputStream = new BufferedInputStream(inputStream);
            reader = new InputStreamReader(bufferedInputStream);
            StringBuilder stringBuilder = new StringBuilder();

            final int bufferSize = 1024 * 2;
            char[] buffer = new char[bufferSize];
            int n = 0;
            while ((n = reader.read(buffer)) != -1) {
                stringBuilder.append(buffer, 0, n);
            }

            return stringBuilder.toString();
        } finally {
            closeQuietly(bufferedInputStream);
            closeQuietly(reader);
        }
    }

    public static boolean stringsEqualOrEmpty(String a, String b) {
        boolean aEmpty = TextUtils.isEmpty(a);
        boolean bEmpty = TextUtils.isEmpty(b);

        if (aEmpty && bEmpty) {
            // Both null or empty, they match.
            return true;
        }
        if (!aEmpty && !bEmpty) {
            // Both non-empty, check equality.
            return a.equals(b);
        }
        // One empty, one non-empty, can't match.
        return false;
    }

    private static void clearCookiesForDomain(Context context, String domain) {
        // This is to work around a bug where CookieManager may fail to instantiate if CookieSyncManager
        // has never been created.
        CookieSyncManager syncManager = CookieSyncManager.createInstance(context);
        syncManager.sync();

        CookieManager cookieManager = CookieManager.getInstance();

        String cookies = cookieManager.getCookie(domain);
        if (cookies == null) {
            return;
        }

        String[] splitCookies = cookies.split(";");
        for (String cookie : splitCookies) {
            String[] cookieParts = cookie.split("=");
            if (cookieParts.length > 0) {
                String newCookie = cookieParts[0].trim() + "=;expires=Sat, 1 Jan 2000 00:00:01 UTC;";
                cookieManager.setCookie(domain, newCookie);
            }
        }
        cookieManager.removeExpiredCookie();
    }

    public static void clearFacebookCookies(Context context) {
        // setCookie acts differently when trying to expire cookies between builds of Android that are using
        // Chromium HTTP stack and those that are not. Using both of these domains to ensure it works on both.
        clearCookiesForDomain(context, "facebook.com");
        clearCookiesForDomain(context, ".facebook.com");
        clearCookiesForDomain(context, "https://facebook.com");
        clearCookiesForDomain(context, "https://.facebook.com");
    }

    public static void logd(String tag, Exception e) {
        if (BuildConfig.DEBUG && tag != null && e != null) {
            Log.d(tag, e.getClass().getSimpleName() + ": " + e.getMessage());
        }
    }

    public static void logd(String tag, String msg) {
        if (BuildConfig.DEBUG && tag != null && msg != null) {
            Log.d(tag, msg);
        }
    }

    public static <T> boolean areObjectsEqual(T a, T b) {
        if (a == null) {
            return b == null;
        }
        return a.equals(b);
    }

    // Note that this method makes a synchronous Graph API call, so should not be called from the main thread.
    public static FetchedAppSettings queryAppSettings(final String applicationId, final boolean forceRequery) {

        // Cache the last app checked results.
        if (!forceRequery && fetchedAppSettings.containsKey(applicationId)) {
            return fetchedAppSettings.get(applicationId);
        }

        Bundle appSettingsParams = new Bundle();
        appSettingsParams.putString(APPLICATION_FIELDS, TextUtils.join(",", APP_SETTING_FIELDS));

        Request request = Request.newGraphPathRequest(null, applicationId, null);
        request.setParameters(appSettingsParams);

        GraphObject supportResponse = request.executeAndWait().getGraphObject();
        FetchedAppSettings result = new FetchedAppSettings(
                safeGetBooleanFromResponse(supportResponse, SUPPORTS_ATTRIBUTION),
                safeGetBooleanFromResponse(supportResponse, SUPPORTS_IMPLICIT_SDK_LOGGING));

        fetchedAppSettings.put(applicationId, result);

        return result;
    }

    private static boolean safeGetBooleanFromResponse(GraphObject response, String propertyName) {
        Object result = false;
        if (response != null) {
            result = response.getProperty(propertyName);
        }
        if (!(result instanceof Boolean)) {
            result = false;
        }
        return (Boolean) result;
    }

    public static void clearCaches(Context context) {
        ImageDownloader.clearCache(context);
    }

    public static void deleteDirectory(File directoryOrFile) {
        if (!directoryOrFile.exists()) {
            return;
        }

        if (directoryOrFile.isDirectory()) {
            for (File child : directoryOrFile.listFiles()) {
                deleteDirectory(child);
            }
        }
        directoryOrFile.delete();
    }

    public static <T> List<T> asListNoNulls(T... array) {
        ArrayList<T> result = new ArrayList<T>();
        for (T t : array) {
            if (t != null) {
                result.add(t);
            }
        }
        return result;
    }

    // Return a hash of the android_id combined with the appid.  Intended to dedupe requests on the server side
    // in order to do counting of users unknown to Facebook.  Because we put the appid into the key prior to hashing,
    // we cannot do correlation of the same user across multiple apps -- this is intentional.  When we transition to
    // the Google advertising ID, we'll get rid of this and always send that up.
    public static String getHashedDeviceAndAppID(Context context, String applicationId) {
        String androidId = Secure.getString(context.getContentResolver(), Secure.ANDROID_ID);

        if (androidId == null) {
            return null;
        } else {
            return sha1hash(androidId + applicationId);
        }
    }

    public static void setAppEventAttributionParameters(GraphObject params,
            String attributionId, String hashedDeviceAndAppId, boolean limitEventUsage) {
        // Send attributionID if it exists, otherwise send a hashed device+appid specific value as the advertiser_id.
        if (attributionId != null) {
            params.setProperty("attribution", attributionId);
        } else if (hashedDeviceAndAppId != null) {
            params.setProperty("advertiser_id", hashedDeviceAndAppId);
        }

        params.setProperty("application_tracking_enabled", !limitEventUsage);
    }
}
