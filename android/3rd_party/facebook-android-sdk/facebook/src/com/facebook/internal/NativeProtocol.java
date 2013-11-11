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
    static final String FACEBOOK_PACKAGE = "com.facebook.katana";
    static final String FACEBOOK_PROXY_AUTH_ACTIVITY = "com.facebook.katana.ProxyAuth";
    static final String FACEBOOK_TOKEN_REFRESH_ACTIVITY = "com.facebook.katana.platform.TokenRefreshService";
    static final String KATANA_SIGNATURE =
            "30820268308201d102044a9c4610300d06092a864886f70d0101040500307a310"
                    + "b3009060355040613025553310b30090603550408130243413112301006035504"
                    + "07130950616c6f20416c746f31183016060355040a130f46616365626f6f6b204"
                    + "d6f62696c653111300f060355040b130846616365626f6f6b311d301b06035504"
                    + "03131446616365626f6f6b20436f72706f726174696f6e3020170d30393038333"
                    + "13231353231365a180f32303530303932353231353231365a307a310b30090603"
                    + "55040613025553310b30090603550408130243413112301006035504071309506"
                    + "16c6f20416c746f31183016060355040a130f46616365626f6f6b204d6f62696c"
                    + "653111300f060355040b130846616365626f6f6b311d301b06035504031314466"
                    + "16365626f6f6b20436f72706f726174696f6e30819f300d06092a864886f70d01"
                    + "0101050003818d0030818902818100c207d51df8eb8c97d93ba0c8c1002c928fa"
                    + "b00dc1b42fca5e66e99cc3023ed2d214d822bc59e8e35ddcf5f44c7ae8ade50d7"
                    + "e0c434f500e6c131f4a2834f987fc46406115de2018ebbb0d5a3c261bd97581cc"
                    + "fef76afc7135a6d59e8855ecd7eacc8f8737e794c60a761c536b72b11fac8e603"
                    + "f5da1a2d54aa103b8a13c0dbc10203010001300d06092a864886f70d010104050"
                    + "0038181005ee9be8bcbb250648d3b741290a82a1c9dc2e76a0af2f2228f1d9f9c"
                    + "4007529c446a70175c5a900d5141812866db46be6559e2141616483998211f4a6"
                    + "73149fb2232a10d247663b26a9031e15f84bc1c74d141ff98a02d76f85b2c8ab2"
                    + "571b6469b232d8e768a7f7ca04f7abe4a775615916c07940656b58717457b42bd"
                    + "928a2";
    private static final String BASIC_INFO = "basic_info";
    public static final String FACEBOOK_PROXY_AUTH_PERMISSIONS_KEY = "scope";
    public static final String FACEBOOK_PROXY_AUTH_APP_ID_KEY = "client_id";
    public static final String FACEBOOK_PROXY_AUTH_E2E_KEY = "e2e";

    static final boolean validateSignature(Context context, String packageName) {
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
            if (signature.toCharsString().equals(KATANA_SIGNATURE)) {
                return true;
            }
        }

        return false;
    }

    static Intent validateKatanaActivityIntent(Context context, Intent intent) {
        if (intent == null) {
            return null;
        }

        ResolveInfo resolveInfo = context.getPackageManager().resolveActivity(intent, 0);
        if (resolveInfo == null) {
            return null;
        }

        if (!validateSignature(context, resolveInfo.activityInfo.packageName)) {
            return null;
        }

        return intent;
    }

    static Intent validateKatanaServiceIntent(Context context, Intent intent) {
        if (intent == null) {
            return null;
        }

        ResolveInfo resolveInfo = context.getPackageManager().resolveService(intent, 0);
        if (resolveInfo == null) {
            return null;
        }

        if (!validateSignature(context, resolveInfo.serviceInfo.packageName)) {
            return null;
        }

        return intent;
    }

    public static Intent createProxyAuthIntent(Context context, String applicationId, List<String> permissions,
            String e2e) {
        Intent intent = new Intent()
                .setClassName(FACEBOOK_PACKAGE, FACEBOOK_PROXY_AUTH_ACTIVITY)
                .putExtra(FACEBOOK_PROXY_AUTH_APP_ID_KEY, applicationId);

        if (!Utility.isNullOrEmpty(permissions)) {
            intent.putExtra(FACEBOOK_PROXY_AUTH_PERMISSIONS_KEY, TextUtils.join(",", permissions));
        }
        if (!Utility.isNullOrEmpty(e2e)) {
            intent.putExtra(FACEBOOK_PROXY_AUTH_E2E_KEY, e2e);
        }

        return validateKatanaActivityIntent(context, intent);
    }

    public static Intent createTokenRefreshIntent(Context context) {
        Intent intent = new Intent()
                .setClassName(FACEBOOK_PACKAGE, FACEBOOK_TOKEN_REFRESH_ACTIVITY);

        return validateKatanaServiceIntent(context, intent);
    }

    // ---------------------------------------------------------------------------------------------
    // Native Protocol updated 2012-11

    static final String INTENT_ACTION_PLATFORM_ACTIVITY = "com.facebook.platform.PLATFORM_ACTIVITY";
    static final String INTENT_ACTION_PLATFORM_SERVICE = "com.facebook.platform.PLATFORM_SERVICE";

    public static final int PROTOCOL_VERSION_20121101 = 20121101;
    public static final int PROTOCOL_VERSION_20130502 = 20130502;
    public static final int PROTOCOL_VERSION_20130618 = 20130618;
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
    public static final String ACTION_LOGIN_DIALOG = "com.facebook.platform.action.request.LOGIN_DIALOG";
    public static final String ACTION_FEED_DIALOG = "com.facebook.platform.action.request.FEED_DIALOG";
    public static final String ACTION_OGACTIONPUBLISH_DIALOG =
            "com.facebook.platform.action.request.OGACTIONPUBLISH_DIALOG";

    // Values of EXTRA_PROTOCOL_ACTION values returned by PlatformActivity:
    static final String ACTION_LOGIN_DIALOG_REPLY =
            "com.facebook.platform.action.reply.LOGIN_DIALOG";
    public static final String ACTION_FEED_DIALOG_REPLY =
            "com.facebook.platform.action.reply.FEED_DIALOG";
    public static final String ACTION_OGACTIONPUBLISH_DIALOG_REPLY =
            "com.facebook.platform.action.reply.OGACTIONPUBLISH_DIALOG";

    // Extras supported for ACTION_LOGIN_DIALOG:
    public static final String EXTRA_PERMISSIONS = "com.facebook.platform.extra.PERMISSIONS";
    public static final String EXTRA_WRITE_PRIVACY = "com.facebook.platform.extra.WRITE_PRIVACY";
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

    // URIs for PlatformProvider
    public static final String CONTENT_SCHEME = "content://";
    public static final String PLATFORM_PROVIDER = FACEBOOK_PACKAGE + ".provider.PlatformProvider";
    public static final Uri PLATFORM_PROVIDER_VERSIONS_URI =
            Uri.parse(CONTENT_SCHEME + PLATFORM_PROVIDER + "/versions");

    // Columns returned by PlatformProvider
    public static final String PLATFORM_PROVIDER_VERSION_COLUMN = "version";

    // Note: be sure this stays sorted in descending order; add new versions at the beginning
    private static final List<Integer> KNOWN_PROTOCOL_VERSIONS =
            Arrays.asList(PROTOCOL_VERSION_20130618,
                    PROTOCOL_VERSION_20130502,
                    PROTOCOL_VERSION_20121101);

    public static Intent createPlatformActivityIntent(Context context, String action, int version, Bundle extras) {
        Intent intent = new Intent()
                .setAction(INTENT_ACTION_PLATFORM_ACTIVITY)
                .setPackage(FACEBOOK_PACKAGE)
                .addCategory(Intent.CATEGORY_DEFAULT)
                .putExtras(extras)
                .putExtra(EXTRA_PROTOCOL_VERSION, version)
                .putExtra(EXTRA_PROTOCOL_ACTION, action);
        return validateKatanaActivityIntent(context, intent);
    }

    public static Intent createPlatformServiceIntent(Context context) {
        Intent intent = new Intent(INTENT_ACTION_PLATFORM_SERVICE)
                .setPackage(FACEBOOK_PACKAGE)
                .addCategory(Intent.CATEGORY_DEFAULT);
        return validateKatanaServiceIntent(context, intent);
    }

    public static Intent createLoginDialog20121101Intent(Context context, String applicationId, ArrayList<String> permissions,
            String audience) {
        Intent intent = new Intent()
                    .setAction(INTENT_ACTION_PLATFORM_ACTIVITY)
                    .setPackage(FACEBOOK_PACKAGE)
                    .addCategory(Intent.CATEGORY_DEFAULT)
                    .putExtra(EXTRA_PROTOCOL_VERSION, PROTOCOL_VERSION_20121101)
                    .putExtra(EXTRA_PROTOCOL_ACTION, ACTION_LOGIN_DIALOG)
                    .putExtra(EXTRA_APPLICATION_ID, applicationId)
                    .putStringArrayListExtra(EXTRA_PERMISSIONS, ensureDefaultPermissions(permissions))
                    .putExtra(EXTRA_PROTOCOL_CALL_ID, generateCallId())
                    .putExtra(EXTRA_WRITE_PRIVACY, ensureDefaultAudience(audience));
        return validateKatanaActivityIntent(context, intent);
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

    private static String generateCallId() {
        return UUID.randomUUID().toString();
    }

    private static String ensureDefaultAudience(String audience) {
        if (Utility.isNullOrEmpty(audience)) {
            return AUDIENCE_ME;
        } else {
            return audience;
        }
    }

    private static ArrayList<String> ensureDefaultPermissions(ArrayList<String> permissions) {
        ArrayList<String> updated;

        // Return if we are doing publish, or if basic_info is already included
        if (Utility.isNullOrEmpty(permissions)) {
            updated = new ArrayList<String>();
        } else {
            for (String permission : permissions) {
                if (Session.isPublishPermission(permission) || BASIC_INFO.equals(permission)) {
                    return permissions;
                }
            }
            updated = new ArrayList<String>(permissions);
        }

        updated.add(BASIC_INFO);
        return updated;
    }

    public static boolean isServiceDisabledResult20121101(Intent data) {
        int protocolVersion = data.getIntExtra(EXTRA_PROTOCOL_VERSION, 0);
        String errorType = data.getStringExtra(STATUS_ERROR_TYPE);

        return ((PROTOCOL_VERSION_20121101 == protocolVersion) && ERROR_SERVICE_DISABLED.equals(errorType));
    }

    public static final int NO_PROTOCOL_AVAILABLE = -1;

    public static int getLatestAvailableProtocolVersion(Context context, final int minimumVersion) {
        ContentResolver contentResolver = context.getContentResolver();

        String [] projection = new String[]{ PLATFORM_PROVIDER_VERSION_COLUMN };
        Cursor c = contentResolver.query(PLATFORM_PROVIDER_VERSIONS_URI, projection, null, null, null);
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

        return NO_PROTOCOL_AVAILABLE;
    }
}
