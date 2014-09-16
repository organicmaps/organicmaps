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
import android.database.Cursor;
import android.net.Uri;
import android.os.Looper;
import android.util.Log;

import com.facebook.FacebookException;

import java.lang.reflect.Method;

/**
 * com.facebook.internal is solely for the use of other packages within the Facebook SDK for Android. Use of
 * any of the classes in this package is unsupported, and they may be modified or removed without warning at
 * any time.
 */
public class AttributionIdentifiers {
    private static final String TAG = AttributionIdentifiers.class.getCanonicalName();
    private static final Uri ATTRIBUTION_ID_CONTENT_URI =
            Uri.parse("content://com.facebook.katana.provider.AttributionIdProvider");
    private static final String ATTRIBUTION_ID_COLUMN_NAME = "aid";
    private static final String ANDROID_ID_COLUMN_NAME = "androidid";
    private static final String LIMIT_TRACKING_COLUMN_NAME = "limit_tracking";

    // com.google.android.gms.common.ConnectionResult.SUCCESS
    private static final int CONNECTION_RESULT_SUCCESS = 0;

    private static final long IDENTIFIER_REFRESH_INTERVAL_MILLIS = 3600 * 1000;

    private String attributionId;
    private String androidAdvertiserId;
    private boolean limitTracking;
    private long fetchTime;

    private static AttributionIdentifiers recentlyFetchedIdentifiers;

    private static AttributionIdentifiers getAndroidId(Context context) {
        AttributionIdentifiers identifiers = new AttributionIdentifiers();
        try {
            // We can't call getAdvertisingIdInfo on the main thread or the app will potentially
            // freeze, if this is the case throw:
            if (Looper.myLooper() == Looper.getMainLooper()) {
              throw new FacebookException("getAndroidId cannot be called on the main thread.");
            }
            Method isGooglePlayServicesAvailable = Utility.getMethodQuietly(
                    "com.google.android.gms.common.GooglePlayServicesUtil",
                    "isGooglePlayServicesAvailable",
                    Context.class
            );

            if (isGooglePlayServicesAvailable == null) {
                return identifiers;
            }

            Object connectionResult = Utility.invokeMethodQuietly(null, isGooglePlayServicesAvailable, context);
            if (!(connectionResult instanceof Integer) || (Integer) connectionResult != CONNECTION_RESULT_SUCCESS) {
                return identifiers;
            }

            Method getAdvertisingIdInfo = Utility.getMethodQuietly(
                    "com.google.android.gms.ads.identifier.AdvertisingIdClient",
                    "getAdvertisingIdInfo",
                    Context.class
            );
            if (getAdvertisingIdInfo == null) {
                return identifiers;
            }
            Object advertisingInfo = Utility.invokeMethodQuietly(null, getAdvertisingIdInfo, context);
            if (advertisingInfo == null) {
                return identifiers;
            }

            Method getId = Utility.getMethodQuietly(advertisingInfo.getClass(), "getId");
            Method isLimitAdTrackingEnabled = Utility.getMethodQuietly(advertisingInfo.getClass(), "isLimitAdTrackingEnabled");
            if (getId == null || isLimitAdTrackingEnabled == null) {
                return identifiers;
            }

            identifiers.androidAdvertiserId = (String) Utility.invokeMethodQuietly(advertisingInfo, getId);
            identifiers.limitTracking = (Boolean) Utility.invokeMethodQuietly(advertisingInfo, isLimitAdTrackingEnabled);
        } catch (Exception e) {
            Utility.logd("android_id", e);
        }
        return identifiers;
    }

    public static AttributionIdentifiers getAttributionIdentifiers(Context context) {
        if (recentlyFetchedIdentifiers != null &&
            System.currentTimeMillis() - recentlyFetchedIdentifiers.fetchTime < IDENTIFIER_REFRESH_INTERVAL_MILLIS) {
            return recentlyFetchedIdentifiers;
        }

        AttributionIdentifiers identifiers = getAndroidId(context);

        try {
            String [] projection = {ATTRIBUTION_ID_COLUMN_NAME, ANDROID_ID_COLUMN_NAME, LIMIT_TRACKING_COLUMN_NAME};
            Cursor c = context.getContentResolver().query(ATTRIBUTION_ID_CONTENT_URI, projection, null, null, null);
            if (c == null || !c.moveToFirst()) {
                return null;
            }
            int attributionColumnIndex = c.getColumnIndex(ATTRIBUTION_ID_COLUMN_NAME);
            int androidIdColumnIndex = c.getColumnIndex(ANDROID_ID_COLUMN_NAME);
            int limitTrackingColumnIndex = c.getColumnIndex(LIMIT_TRACKING_COLUMN_NAME);

            identifiers.attributionId = c.getString(attributionColumnIndex);

            // if we failed to call Google's APIs directly (due to improper integration by the client), it may be
            // possible for the local facebook application to relay it to us.
            if (androidIdColumnIndex > 0 && limitTrackingColumnIndex > 0 && identifiers.getAndroidAdvertiserId() == null) {
                identifiers.androidAdvertiserId = c.getString(androidIdColumnIndex);
                identifiers.limitTracking = Boolean.parseBoolean(c.getString(limitTrackingColumnIndex));
            }
            c.close();
        } catch (Exception e) {
            Log.d(TAG, "Caught unexpected exception in getAttributionId(): " + e.toString());
            return null;
        }

        identifiers.fetchTime = System.currentTimeMillis();
        recentlyFetchedIdentifiers = identifiers;
        return identifiers;
    }

    public String getAttributionId() {
        return attributionId;
    }

    public String getAndroidAdvertiserId() {
        return androidAdvertiserId;
    }

    public boolean isTrackingLimited() {
        return limitTracking;
    }
}