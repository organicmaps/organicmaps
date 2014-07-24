package com.facebook;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

public class AppLinkDataTests extends FacebookTestCase {
    private static final String TARGET_URI_STRING = "http://test.app/foo";
    private static final String FB_REF_KEY = "fb_ref";
    private static final String FB_REF_VALUE = "foobar";
    private static final String REFERER_DATA_KEY = "referer_data";
    private static final String EXTRA_ARGS_KEY = "extra_args";
    private static final String EXTRA_ARGS_VALUE = "extra_value";
    private static final String TARGET_URL_KEY = "target_url";
    private static final String USER_AGENT_KEY = "user_agent";
    private static final String USER_AGENT_VALUE = "foobarUserAgent";

    private static final String JSON_DATA_REGULAR =
            "{"
            + "\"version\":2,"
            + "\"bridge_args\": {\"method\": \"applink\"},"
            + "\"method_args\": {"
            + "    \"ref\": \"" + FB_REF_VALUE + "\","
            + "    \"" + TARGET_URL_KEY + "\": \"" + TARGET_URI_STRING + "\""
            + "  }"
            + "}";

    private static final String JSON_DATA_REGULAR_WITH_NESTED_ARRAY =
            "{"
            + "\"version\":2,"
            + "\"bridge_args\": {\"method\": \"applink\"},"
            + "\"method_args\": {"
            + "    \"ref\": \"" + FB_REF_VALUE + "\","
            + "    \"" + TARGET_URL_KEY + "\": \"" + TARGET_URI_STRING + "\","
            + "    \"other\": [ [1, 2], [3, 4] ]"
            + "  }"
            + "}";

    private static final String JSON_DATA_WITH_REFERER_DATA =
            "{"
            + "\"version\":2,"
            + "\"bridge_args\": {\"method\": \"applink\"},"
            + "\"method_args\": {"
            + "    \"referer_data\" : {"
            + "      \"" + FB_REF_KEY + "\": \"" + FB_REF_VALUE + "\","
            + "      \"" + EXTRA_ARGS_KEY + "\": \"" + EXTRA_ARGS_VALUE + "\""
            + "    },"
            + "    \"" + TARGET_URL_KEY + "\": \"" + TARGET_URI_STRING + "\""
            + "  }"
            + "}";


    private static class MockActivityWithAppLinkData extends Activity {
        public Intent getIntent() {
            Uri targetUri = Uri.parse(TARGET_URI_STRING);
            Intent intent = new Intent(Intent.ACTION_VIEW, targetUri);
            Bundle applinks = new Bundle();
            Bundle refererData = new Bundle();
            refererData.putString(FB_REF_KEY, FB_REF_VALUE);
            refererData.putString(EXTRA_ARGS_KEY, EXTRA_ARGS_VALUE);
            applinks.putBundle(REFERER_DATA_KEY, refererData);
            applinks.putString(TARGET_URL_KEY, TARGET_URI_STRING);
            applinks.putString(USER_AGENT_KEY, USER_AGENT_VALUE);
            intent.putExtra("al_applink_data", applinks);
            return intent;
        }
    }

    private static class MockActivityWithJsonData extends Activity {
        private boolean useRefererData;

        public MockActivityWithJsonData(boolean useRefererData) {
            this.useRefererData = useRefererData;
        }
        public Intent getIntent() {
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.putExtra(AppLinkData.BUNDLE_APPLINK_ARGS_KEY,
                    useRefererData ? JSON_DATA_WITH_REFERER_DATA : JSON_DATA_REGULAR);
            return intent;
        }
    }

    private static class MockActivityWithErrorJsonData extends Activity {
        public Intent getIntent() {
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.putExtra(AppLinkData.BUNDLE_APPLINK_ARGS_KEY, JSON_DATA_REGULAR_WITH_NESTED_ARRAY);
            return intent;
        }
    }

    public void testCreateFromAlApplinkData() {
        AppLinkData appLinkData = AppLinkData.createFromActivity(new MockActivityWithAppLinkData());
        assertNotNull("app link data not null", appLinkData);
        assertEquals("ref param", FB_REF_VALUE, appLinkData.getRef());
        assertEquals("target_url", TARGET_URI_STRING, appLinkData.getTargetUri().toString());
        Bundle args = appLinkData.getArgumentBundle();
        assertNotNull("app link args not null", args);
        assertEquals("user agent", USER_AGENT_VALUE, args.getString(USER_AGENT_KEY));
        Bundle refererData = appLinkData.getRefererData();
        assertNotNull("referer data not null", refererData);
        assertEquals("ref param in referer data", FB_REF_VALUE, refererData.getString(FB_REF_KEY));
        assertEquals("extra param", EXTRA_ARGS_VALUE, refererData.getString(EXTRA_ARGS_KEY));
    }

    public void testCreateFromJson() {
        AppLinkData appLinkData = AppLinkData.createFromActivity(new MockActivityWithJsonData(false));
        assertNotNull("app link data not null", appLinkData);
        assertEquals("ref param", FB_REF_VALUE, appLinkData.getRef());
        assertEquals("target_url", TARGET_URI_STRING, appLinkData.getTargetUri().toString());
        Bundle args = appLinkData.getArgumentBundle();
        assertNotNull("app link args not null", args);
        assertNull("user agent", args.getString(USER_AGENT_KEY));
        Bundle refererData = appLinkData.getRefererData();
        assertNull("referer data", refererData);
    }

    public void testCreateFromJsonWithNestedArray() {
        AppLinkData appLinkData = AppLinkData.createFromActivity(new MockActivityWithErrorJsonData());
        assertNull(appLinkData);
    }


    public void testCreateFromJsonWithRefererData() {
        AppLinkData appLinkData = AppLinkData.createFromActivity(new MockActivityWithJsonData(true));
        assertNotNull("app link data not null", appLinkData);
        assertEquals("ref param", FB_REF_VALUE, appLinkData.getRef());
        assertEquals("target_url", TARGET_URI_STRING, appLinkData.getTargetUri().toString());
        Bundle args = appLinkData.getArgumentBundle();
        assertNotNull("app link args not null", args);
        assertNull("user agent", args.getString(USER_AGENT_KEY));
        Bundle refererData = appLinkData.getRefererData();
        assertNotNull("referer data", refererData);
        assertEquals("ref param in referer data", FB_REF_VALUE, refererData.getString(FB_REF_KEY));
        assertEquals("extra param", EXTRA_ARGS_VALUE, refererData.getString(EXTRA_ARGS_KEY));
    }
}
