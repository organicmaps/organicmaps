package com.facebook.internal;

/**
 * com.facebook.internal is solely for the use of other packages within the Facebook SDK for Android. Use of
 * any of the classes in this package is unsupported, and they may be modified or removed without warning at
 * any time.
 */
public class AnalyticsEvents {
    public static final String EVENT_NATIVE_LOGIN_DIALOG_COMPLETE   = "fb_dialogs_native_login_dialog_complete";
    public static final String EVENT_NATIVE_LOGIN_DIALOG_START      = "fb_dialogs_native_login_dialog_start";
    public static final String EVENT_WEB_LOGIN_COMPLETE             = "fb_dialogs_web_login_dialog_complete";
    public static final String EVENT_FRIEND_PICKER_USAGE            = "fb_friend_picker_usage";
    public static final String EVENT_PLACE_PICKER_USAGE             = "fb_place_picker_usage";
    public static final String EVENT_LOGIN_VIEW_USAGE               = "fb_login_view_usage";
    public static final String EVENT_USER_SETTINGS_USAGE            = "fb_user_settings_vc_usage";

    public static final String PARAMETER_WEB_LOGIN_E2E                  = "fb_web_login_e2e";
    public static final String PARAMETER_WEB_LOGIN_SWITCHBACK_TIME      = "fb_web_login_switchback_time";
    public static final String PARAMETER_APP_ID                         = "app_id";
    public static final String PARAMETER_ACTION_ID                      = "action_id";
    public static final String PARAMETER_NATIVE_LOGIN_DIALOG_START_TIME = "fb_native_login_dialog_start_time";
    public static final String PARAMETER_NATIVE_LOGIN_DIALOG_COMPLETE_TIME =
            "fb_native_login_dialog_complete_time";

    public static final String PARAMETER_DIALOG_OUTCOME                 = "fb_dialog_outcome";
    public static final String PARAMETER_DIALOG_OUTCOME_VALUE_COMPLETED = "Completed";
    public static final String PARAMETER_DIALOG_OUTCOME_VALUE_UNKNOWN   = "Unknown";
    public static final String PARAMETER_DIALOG_OUTCOME_VALUE_CANCELLED = "Cancelled";
    public static final String PARAMETER_DIALOG_OUTCOME_VALUE_FAILED    = "Failed";

}
