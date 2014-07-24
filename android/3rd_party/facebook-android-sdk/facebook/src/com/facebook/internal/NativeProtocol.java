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

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.*;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import com.facebook.*;

import java.util.*;

/**
 * com.facebook.internal is solely for the use of other packages within the Facebook SDK for Android. Use of
 * any of the classes in this package is unsupported, and they may be modified or removed without warning at
 * any time.
 */
public final class NativeProtocol {

    public static final int NO_PROTOCOL_AVAILABLE = -1;

    private static final String FACEBOOK_PROXY_AUTH_ACTIVITY = "com.facebook.katana.ProxyAuth";
    private static final String FACEBOOK_TOKEN_REFRESH_ACTIVITY = "com.facebook.katana.platform.TokenRefreshService";

    public static final String FACEBOOK_PROXY_AUTH_PERMISSIONS_KEY = "scope";
    public static final String FACEBOOK_PROXY_AUTH_APP_ID_KEY = "client_id";
    public static final String FACEBOOK_PROXY_AUTH_E2E_KEY = "e2e";

    // ---------------------------------------------------------------------------------------------
    // Native Protocol updated 2012-11

    static final String INTENT_ACTION_PLATFORM_ACTIVITY = "com.facebook.platform.PLATFORM_ACTIVITY";
    static final String INTENT_ACTION_PLATFORM_SERVICE = "com.facebook.platform.PLATFORM_SERVICE";

    public static final int PROTOCOL_VERSION_20121101 = 20121101;
    public static final int PROTOCOL_VERSION_20130502 = 20130502;
    public static final int PROTOCOL_VERSION_20130618 = 20130618;
    public static final int PROTOCOL_VERSION_20131107 = 20131107;
    public static final int PROTOCOL_VERSION_20140204 = 20140204;
    public static final int PROTOCOL_VERSION_20140324 = 20140324;

    public static final String EXTRA_PROTOCOL_VERSION = "com.facebook.platform.protocol.PROTOCOL_VERSION";
    public static final String EXTRA_PROTOCOL_ACTION = "com.facebook.platform.protocol.PROTOCOL_ACTION";
    public static final String EXTRA_PROTOCOL_CALL_ID = "com.facebook.platform.protocol.CALL_ID";
    public static final String EXTRA_GET_INSTALL_DATA_PACKAGE = "com.facebook.platform.extra.INSTALLDATA_PACKAGE";

    // Messages supported by PlatformService:
    public static final int MESSAGE_GET_ACCESS_TOKEN_REQUEST = 0x10000;
    public static final int MESSAGE_GET_ACCESS_TOKEN_REPLY   = 0x10001;
    static final int MESSAGE_GET_PROTOCOL_VERSIONS_REQUEST = 0x10002;
    static final int MESSAGE_GET_PROTOCOL_VERSIONS_REPLY   = 0x10003;
    public static final int MESSAGE_GET_INSTALL_DATA_REQUEST = 0x10004;
    public static final int MESSAGE_GET_INSTALL_DATA_REPLY   = 0x10005;

    // MESSAGE_ERROR_REPLY data keys:
    // See STATUS_*

    // MESSAGE_GET_ACCESS_TOKEN_REQUEST data keys:
    // EXTRA_APPLICATION_ID

    // MESSAGE_GET_ACCESS_TOKEN_REPLY data keys:
    // EXTRA_ACCESS_TOKEN
    // EXTRA_EXPIRES_SECONDS_SINCE_EPOCH
    // EXTRA_PERMISSIONS

    // MESSAGE_GET_PROTOCOL_VERSIONS_REPLY data keys:
    static final String EXTRA_PROTOCOL_VERSIONS = "com.facebook.platform.extra.PROTOCOL_VERSIONS";

    // Values of EXTRA_PROTOCOL_ACTION supported by PlatformActivity:
    public static final String ACTION_FEED_DIALOG = "com.facebook.platform.action.request.FEED_DIALOG";
    public static final String ACTION_MESSAGE_DIALOG = "com.facebook.platform.action.request.MESSAGE_DIALOG";
    public static final String ACTION_OGACTIONPUBLISH_DIALOG =
            "com.facebook.platform.action.request.OGACTIONPUBLISH_DIALOG";
    public static final String ACTION_OGMESSAGEPUBLISH_DIALOG =
            "com.facebook.platform.action.request.OGMESSAGEPUBLISH_DIALOG";

    // Values of EXTRA_PROTOCOL_ACTION values returned by PlatformActivity:
    public static final String ACTION_FEED_DIALOG_REPLY =
            "com.facebook.platform.action.reply.FEED_DIALOG";
    public static final String ACTION_MESSAGE_DIALOG_REPLY =
            "com.facebook.platform.action.reply.MESSAGE_DIALOG";
    public static final String ACTION_OGACTIONPUBLISH_DIALOG_REPLY =
            "com.facebook.platform.action.reply.OGACTIONPUBLISH_DIALOG";
    public static final String ACTION_OGMESSAGEPUBLISH_DIALOG_REPLY =
            "com.facebook.platform.action.reply.OGMESSAGEPUBLISH_DIALOG";

    // Extras supported for ACTION_LOGIN_DIALOG:
    public static final String EXTRA_PERMISSIONS = "com.facebook.platform.extra.PERMISSIONS";
    public static final String EXTRA_APPLICATION_ID = "com.facebook.platform.extra.APPLICATION_ID";
    public static final String EXTRA_APPLICATION_NAME = "com.facebook.platform.extra.APPLICATION_NAME";

    // Extras returned by setResult() for ACTION_LOGIN_DIALOG
    public static final String EXTRA_ACCESS_TOKEN = "com.facebook.platform.extra.ACCESS_TOKEN";
    public static final String EXTRA_EXPIRES_SECONDS_SINCE_EPOCH =
            "com.facebook.platform.extra.EXPIRES_SECONDS_SINCE_EPOCH";
    // EXTRA_PERMISSIONS

    // Extras supported for ACTION_FEED_DIALOG:
    public static final String EXTRA_PLACE_TAG = "com.facebook.platform.extra.PLACE";
    public static final String EXTRA_FRIEND_TAGS = "com.facebook.platform.extra.FRIENDS";
    public static final String EXTRA_LINK = "com.facebook.platform.extra.LINK";
    public static final String EXTRA_IMAGE = "com.facebook.platform.extra.IMAGE";
    public static final String EXTRA_TITLE = "com.facebook.platform.extra.TITLE";
    public static final String EXTRA_SUBTITLE = "com.facebook.platform.extra.SUBTITLE";
    public static final String EXTRA_DESCRIPTION = "com.facebook.platform.extra.DESCRIPTION";
    public static final String EXTRA_REF = "com.facebook.platform.extra.REF";
    public static final String EXTRA_DATA_FAILURES_FATAL = "com.facebook.platform.extra.DATA_FAILURES_FATAL";
    public static final String EXTRA_PHOTOS = "com.facebook.platform.extra.PHOTOS";

    // Extras supported for ACTION_OGACTIONPUBLISH_DIALOG:
    public static final String EXTRA_ACTION = "com.facebook.platform.extra.ACTION";
    public static final String EXTRA_ACTION_TYPE = "com.facebook.platform.extra.ACTION_TYPE";
    public static final String EXTRA_PREVIEW_PROPERTY_NAME =
            "com.facebook.platform.extra.PREVIEW_PROPERTY_NAME";

    // OG objects will have this key to set to true if they should be created as part of OG Action publish
    public static final String OPEN_GRAPH_CREATE_OBJECT_KEY = "fbsdk:create_object";
    // Determines whether an image is user generated
    public static final String IMAGE_USER_GENERATED_KEY = "user_generated";
    // url key for images
    public static final String IMAGE_URL_KEY = "url";

    // Keys for status data in MESSAGE_ERROR_REPLY from PlatformService and for error
    // extras returned by PlatformActivity's setResult() in case of errors:
    public static final String STATUS_ERROR_TYPE = "com.facebook.platform.status.ERROR_TYPE";
    public static final String STATUS_ERROR_DESCRIPTION =
            "com.facebook.platform.status.ERROR_DESCRIPTION";
    public static final String STATUS_ERROR_CODE = "com.facebook.platform.status.ERROR_CODE";
    public static final String STATUS_ERROR_SUBCODE = "com.facebook.platform.status.ERROR_SUBCODE";
    public static final String STATUS_ERROR_JSON = "com.facebook.platform.status.ERROR_JSON";

    // Expected values for ERROR_KEY_TYPE.  Clients should tolerate other values:
    public static final String ERROR_UNKNOWN_ERROR = "UnknownError";
    public static final String ERROR_PROTOCOL_ERROR = "ProtocolError";
    public static final String ERROR_USER_CANCELED = "UserCanceled";
    public static final String ERROR_APPLICATION_ERROR = "ApplicationError";
    public static final String ERROR_NETWORK_ERROR = "NetworkError";
    public static final String ERROR_PERMISSION_DENIED = "PermissionDenied";
    public static final String ERROR_SERVICE_DISABLED = "ServiceDisabled";

    public static final String AUDIENCE_ME = "SELF";
    public static final String AUDIENCE_FRIENDS = "ALL_FRIENDS";
    public static final String AUDIENCE_EVERYONE = "EVERYONE";

    // Request codes for different categories of native protocol calls.
    public static final int DIALOG_REQUEST_CODE = 0xfacf;

    private static final String CONTENT_SCHEME = "content://";
    private static final String PLATFORM_PROVIDER_VERSIONS = ".provider.PlatformProvider/versions";

    // Columns returned by PlatformProvider
    private static final String PLATFORM_PROVIDER_VERSION_COLUMN = "version";

    // Broadcast action for asynchronously-executing AppCalls
    private static final String PLATFORM_ASYNC_APPCALL_ACTION = "com.facebook.platform.AppCallResultBroadcast";

    private static abstract class NativeAppInfo {
        abstract protected String getPackage();

        private static final String FBI_HASH = "a4b7452e2ed8f5f191058ca7bbfd26b0d3214bfc";
        private static final String FBL_HASH = "5e8f16062ea3cd2c4a0d547876baa6f38cabf625";
        private static final String FBR_HASH = "8a3c4b262d721acd49a4bf97d5213199c86fa2b9";

        private static final HashSet<String> validAppSignatureHashes = buildAppSignatureHashes();

        private static HashSet<String> buildAppSignatureHashes() {
            HashSet<String> set = new HashSet<String>();
            set.add(FBR_HASH);
            set.add(FBI_HASH);
            set.add(FBL_HASH);
            return set;
        }

        public boolean validateSignature(Context context, String packageName) {
            String brand = Build.BRAND;
            int applicationFlags = context.getApplicationInfo().flags;
            if (brand.startsWith("generic") && (applicationFlags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
                // We are debugging on an emulator, don't validate package signature.
                return true;
            }

            PackageInfo packageInfo = null;
            try {
                packageInfo = context.getPackageManager().getPackageInfo(packageName,
                        PackageManager.GET_SIGNATURES);
            } catch (PackageManager.NameNotFoundException e) {
                return false;
            }

            for (Signature signature : packageInfo.signatures) {
                String hashedSignature = Utility.sha1hash(signature.toByteArray());
                if (validAppSignatureHashes.contains(hashedSignature)) {
                    return true;
                }
            }

            return false;
        }
    }

    private static class KatanaAppInfo extends NativeAppInfo {
        static final String KATANA_PACKAGE = "com.facebook.katana";

        @Override
        protected String getPackage() {
            return KATANA_PACKAGE;
        }
    }

    private static class MessengerAppInfo extends NativeAppInfo {
        static final String MESSENGER_PACKAGE = "com.facebook.orca";

        @Override
        protected String getPackage() {
            return MESSENGER_PACKAGE;
        }
    }

    private static class WakizashiAppInfo extends NativeAppInfo {
        static final String WAKIZASHI_PACKAGE = "com.facebook.wakizashi";

        @Override
        protected String getPackage() {
            return WAKIZASHI_PACKAGE;
        }
    }

    private static final NativeAppInfo FACEBOOK_APP_INFO = new KatanaAppInfo();
    private static List<NativeAppInfo> facebookAppInfoList = buildFacebookAppList();
    private static Map<String, List<NativeAppInfo>> actionToAppInfoMap = buildActionToAppInfoMap();

    private static List<NativeAppInfo> buildFacebookAppList() {
        List<NativeAppInfo> list = new ArrayList<NativeAppInfo>();

        // Katana needs to be the first thing in the list since it will get selected as the default FACEBOOK_APP_INFO
        list.add(FACEBOOK_APP_INFO);
        list.add(new WakizashiAppInfo());

        return list;
    }

    private static Map<String, List<NativeAppInfo>> buildActionToAppInfoMap() {
        Map<String, List<NativeAppInfo>> map = new HashMap<String, List<NativeAppInfo>>();

        ArrayList<NativeAppInfo> messengerAppInfoList = new ArrayList<NativeAppInfo>();
        messengerAppInfoList.add(new MessengerAppInfo());

        // Add individual actions and the list they should try
        map.put(ACTION_OGACTIONPUBLISH_DIALOG, facebookAppInfoList);
        map.put(ACTION_FEED_DIALOG, facebookAppInfoList);
        map.put(ACTION_MESSAGE_DIALOG, messengerAppInfoList);
        map.put(ACTION_OGMESSAGEPUBLISH_DIALOG, messengerAppInfoList);

        return map;
    }

    static Intent validateActivityIntent(Context context, Intent intent, NativeAppInfo appInfo) {
        if (intent == null) {
            return null;
        }

        ResolveInfo resolveInfo = context.getPackageManager().resolveActivity(intent, 0);
        if (resolveInfo == null) {
            return null;
        }

        if (!appInfo.validateSignature(context, resolveInfo.activityInfo.packageName)) {
            return null;
        }

        return intent;
    }

    static Intent validateServiceIntent(Context context, Intent intent, NativeAppInfo appInfo) {
        if (intent == null) {
            return null;
        }

        ResolveInfo resolveInfo = context.getPackageManager().resolveService(intent, 0);
        if (resolveInfo == null) {
            return null;
        }

        if (!appInfo.validateSignature(context, resolveInfo.serviceInfo.packageName)) {
            return null;
        }

        return intent;
    }

    public static Intent createProxyAuthIntent(Context context, String applicationId, List<String> permissions,
            String e2e, boolean isRerequest) {
        for (NativeAppInfo appInfo : facebookAppInfoList) {
            Intent intent = new Intent()
                    .setClassName(appInfo.getPackage(), FACEBOOK_PROXY_AUTH_ACTIVITY)
                    .putExtra(FACEBOOK_PROXY_AUTH_APP_ID_KEY, applicationId);

            if (!Utility.isNullOrEmpty(permissions)) {
                intent.putExtra(FACEBOOK_PROXY_AUTH_PERMISSIONS_KEY, TextUtils.join(",", permissions));
            }
            if (!Utility.isNullOrEmpty(e2e)) {
                intent.putExtra(FACEBOOK_PROXY_AUTH_E2E_KEY, e2e);
            }

            intent.putExtra(ServerProtocol.DIALOG_PARAM_RESPONSE_TYPE, ServerProtocol.DIALOG_RESPONSE_TYPE_TOKEN);
            intent.putExtra(ServerProtocol.DIALOG_PARAM_RETURN_SCOPES, ServerProtocol.DIALOG_RETURN_SCOPES_TRUE);

            if (!Settings.getPlatformCompatibilityEnabled()) {
                // Override the API Version for Auth
                intent.putExtra(ServerProtocol.DIALOG_PARAM_LEGACY_OVERRIDE, ServerProtocol.GRAPH_API_VERSION);

                // Only set the rerequest auth type for non legacy requests
                if (isRerequest) {
                    intent.putExtra(ServerProtocol.DIALOG_PARAM_AUTH_TYPE, ServerProtocol.DIALOG_REREQUEST_AUTH_TYPE);
                }
            }

            intent = validateActivityIntent(context, intent, appInfo);

            if (intent != null) {
                return intent;
            }
        }
        return null;
    }

    public static Intent createTokenRefreshIntent(Context context) {
        for (NativeAppInfo appInfo : facebookAppInfoList) {
            Intent intent = new Intent()
                    .setClassName(appInfo.getPackage(), FACEBOOK_TOKEN_REFRESH_ACTIVITY);

            intent = validateServiceIntent(context, intent, appInfo);

            if (intent != null) {
                return intent;
            }
        }
        return null;
    }

    // Note: be sure this stays sorted in descending order; add new versions at the beginning
    private static final List<Integer> KNOWN_PROTOCOL_VERSIONS =
            Arrays.asList(
                    PROTOCOL_VERSION_20140324,
                    PROTOCOL_VERSION_20140204,
                    PROTOCOL_VERSION_20131107,
                    PROTOCOL_VERSION_20130618,
                    PROTOCOL_VERSION_20130502,
                    PROTOCOL_VERSION_20121101
            );

    private static Intent findActivityIntent(Context context, String activityAction, String internalAction) {
        List<NativeAppInfo> list = actionToAppInfoMap.get(internalAction);
        if (list == null) {
            return null;
        }

        Intent intent = null;
        for (NativeAppInfo appInfo : list) {
            intent = new Intent()
                    .setAction(activityAction)
                    .setPackage(appInfo.getPackage())
                    .addCategory(Intent.CATEGORY_DEFAULT);
            intent = validateActivityIntent(context, intent, appInfo);
            if (intent != null) {
                return intent;
            }
        }

        return intent;
    }

    public static Intent createPlatformActivityIntent(Context context, String action, int version, Bundle extras) {
        Intent intent = findActivityIntent(context, INTENT_ACTION_PLATFORM_ACTIVITY, action);
        if (intent == null) {
            return null;
        }

        intent.putExtras(extras)
                .putExtra(EXTRA_PROTOCOL_VERSION, version)
                .putExtra(EXTRA_PROTOCOL_ACTION, action);

        return intent;
    }

    public static Intent createPlatformServiceIntent(Context context) {
        for (NativeAppInfo appInfo : facebookAppInfoList) {
            Intent intent = new Intent(INTENT_ACTION_PLATFORM_SERVICE)
                    .setPackage(appInfo.getPackage())
                    .addCategory(Intent.CATEGORY_DEFAULT);
            intent = validateServiceIntent(context, intent, appInfo);
            if (intent != null) {
                return intent;
            }
        }
        return null;
    }

    public static boolean isErrorResult(Intent resultIntent) {
        return resultIntent.hasExtra(STATUS_ERROR_TYPE);
    }

    public static Exception getErrorFromResult(Intent resultIntent) {
        if (!isErrorResult(resultIntent)) {
            return null;
        }

        String type = resultIntent.getStringExtra(STATUS_ERROR_TYPE);
        String description = resultIntent.getStringExtra(STATUS_ERROR_DESCRIPTION);

        if (type.equalsIgnoreCase(ERROR_USER_CANCELED)) {
            return new FacebookOperationCanceledException(description);
        }
        /* TODO parse error values and create appropriate exception class */
        return new FacebookException(description);
    }

    public static int getLatestAvailableProtocolVersionForService(Context context, final int minimumVersion) {
        // Services are currently always against the Facebook App
        return getLatestAvailableProtocolVersionForAppInfoList(context, facebookAppInfoList, minimumVersion);
    }

    public static int getLatestAvailableProtocolVersionForAction(Context context, String action, final int minimumVersion) {
        List<NativeAppInfo> appInfoList = actionToAppInfoMap.get(action);
        return getLatestAvailableProtocolVersionForAppInfoList(context, appInfoList, minimumVersion);
    }

    private static int getLatestAvailableProtocolVersionForAppInfoList(Context context, List<NativeAppInfo> appInfoList,
            final int minimumVersion) {
        if (appInfoList == null) {
            return NO_PROTOCOL_AVAILABLE;
        }

        // Could potentially cache the NativeAppInfo to latestProtocolVersion
        for (NativeAppInfo appInfo : appInfoList) {
            int protocolVersion = getLatestAvailableProtocolVersionForAppInfo(context, appInfo, minimumVersion);
            if (protocolVersion != NO_PROTOCOL_AVAILABLE) {
                return protocolVersion;
            }
        }

        return NO_PROTOCOL_AVAILABLE;
    }

    private static int getLatestAvailableProtocolVersionForAppInfo(Context context, NativeAppInfo appInfo,
            final int minimumVersion) {
        ContentResolver contentResolver = context.getContentResolver();

        String [] projection = new String[]{ PLATFORM_PROVIDER_VERSION_COLUMN };
        Uri uri = buildPlatformProviderVersionURI(appInfo);
        Cursor c = null;
        try {
            c = contentResolver.query(uri, projection, null, null, null);
            if (c == null) {
                return NO_PROTOCOL_AVAILABLE;
            }

            Set<Integer> versions = new HashSet<Integer>();
            while (c.moveToNext()) {
                int version = c.getInt(c.getColumnIndex(PLATFORM_PROVIDER_VERSION_COLUMN));
                versions.add(version);
            }

            for (Integer knownVersion : KNOWN_PROTOCOL_VERSIONS) {
                if (knownVersion < minimumVersion) {
                    return NO_PROTOCOL_AVAILABLE;
                }

                if (versions.contains(knownVersion)) {
                    return knownVersion;
                }
            }
        } finally {
            if (c != null) {
                c.close();
            }
        }

        return NO_PROTOCOL_AVAILABLE;
    }

    private static Uri buildPlatformProviderVersionURI(NativeAppInfo appInfo) {
        return Uri.parse(CONTENT_SCHEME + appInfo.getPackage() + PLATFORM_PROVIDER_VERSIONS);
    }
}
