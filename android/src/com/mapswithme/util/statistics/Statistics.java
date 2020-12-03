package com.mapswithme.util.statistics;

import android.app.Activity;
import android.content.Context;
import android.location.Location;
import android.net.NetworkInfo;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Pair;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.android.billingclient.api.BillingClient;
import com.facebook.ads.AdError;
import com.facebook.appevents.AppEventsLogger;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.ads.MwmNativeAd;
import com.mapswithme.maps.ads.NativeAdError;
import com.mapswithme.maps.analytics.ExternalLibrariesMediator;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.editor.OsmOAuth;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.maplayer.Mode;
import com.mapswithme.maps.purchase.ValidationStatus;
import com.mapswithme.maps.routing.RoutePointInfo;
import com.mapswithme.maps.routing.RoutingOptions;
import com.mapswithme.maps.settings.RoadType;
import com.mapswithme.maps.taxi.TaxiInfoError;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.maps.taxi.TaxiType;
import com.mapswithme.maps.widget.menu.MainMenu;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.BatteryState;
import com.mapswithme.util.Config;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Counters;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.PowerManagment;
import com.mapswithme.util.SharedPropertiesUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import static com.mapswithme.util.BatteryState.CHARGING_STATUS_PLUGGED;
import static com.mapswithme.util.BatteryState.CHARGING_STATUS_UNKNOWN;
import static com.mapswithme.util.BatteryState.CHARGING_STATUS_UNPLUGGED;
import static com.mapswithme.util.statistics.Statistics.EventName.APPLICATION_COLD_STARTUP_INFO;
import static com.mapswithme.util.statistics.Statistics.EventName.AUTH_DECLINED;
import static com.mapswithme.util.statistics.Statistics.EventName.AUTH_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.AUTH_EXTERNAL_REQUEST_SUCCESS;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_BOOKMARKS_LIST_CATEGORY_SELECT;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_BOOKMARKS_LIST_COLLECTION_SELECT;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_BOOKMARKS_VISIBILITY_CHANGE;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_GUIDES_DOWNLOADDIALOGUE_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_GUIDE_VISIBILITY_CHANGE;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_RESTORE_PROPOSAL_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_RESTORE_PROPOSAL_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_RESTORE_PROPOSAL_SUCCESS;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_SYNC_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_SYNC_PROPOSAL_APPROVED;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_SYNC_PROPOSAL_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_SYNC_PROPOSAL_SHOWN;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_SYNC_PROPOSAL_TOGGLE;
import static com.mapswithme.util.statistics.Statistics.EventName.BM_SYNC_SUCCESS;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.ELEVATION_PROFILE_PAGE_CLOSE;
import static com.mapswithme.util.statistics.Statistics.EventName.ELEVATION_PROFILE_PAGE_OPEN;
import static com.mapswithme.util.statistics.Statistics.EventName.GUIDES_BOOKMARK_SELECT;
import static com.mapswithme.util.statistics.Statistics.EventName.GUIDES_OPEN;
import static com.mapswithme.util.statistics.Statistics.EventName.GUIDES_SHOWN;
import static com.mapswithme.util.statistics.Statistics.EventName.GUIDES_TRACK_SELECT;
import static com.mapswithme.util.statistics.Statistics.EventName.INAPP_PURCHASE_PREVIEW_SELECT;
import static com.mapswithme.util.statistics.Statistics.EventName.INAPP_PURCHASE_PREVIEW_SHOW;
import static com.mapswithme.util.statistics.Statistics.EventName.INAPP_PURCHASE_PRODUCT_DELIVERED;
import static com.mapswithme.util.statistics.Statistics.EventName.INAPP_PURCHASE_STORE_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.INAPP_PURCHASE_VALIDATION_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_BLANK;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_CLOSE;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_SHOW;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_OWNERSHIP_BUTTON_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_BOOK;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_OPEN;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_SHOWN;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_USER_ITEM;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSOR_ITEM_SELECTED;
import static com.mapswithme.util.statistics.Statistics.EventName.ROUTING_PLAN_TOOLTIP_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.ROUTING_ROUTE_FINISH;
import static com.mapswithme.util.statistics.Statistics.EventName.ROUTING_ROUTE_START;
import static com.mapswithme.util.statistics.Statistics.EventName.SEARCH_FILTER_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.SEARCH_QUICKFILTER_APPLY;
import static com.mapswithme.util.statistics.Statistics.EventName.SEARCH_QUICKFILTER_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.SEARCH_QUICKFILTER_OPEN;
import static com.mapswithme.util.statistics.Statistics.EventName.TIPS_TRICKS_CLOSE;
import static com.mapswithme.util.statistics.Statistics.EventName.TOOLBAR_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.TOOLBAR_MENU_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.UGC_AUTH_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.UGC_AUTH_EXTERNAL_REQUEST_SUCCESS;
import static com.mapswithme.util.statistics.Statistics.EventName.UGC_REVIEW_START;
import static com.mapswithme.util.statistics.Statistics.EventParam.ACTION;
import static com.mapswithme.util.statistics.Statistics.EventParam.BANNER;
import static com.mapswithme.util.statistics.Statistics.EventParam.BATTERY;
import static com.mapswithme.util.statistics.Statistics.EventParam.BUTTON;
import static com.mapswithme.util.statistics.Statistics.EventParam.CATEGORY;
import static com.mapswithme.util.statistics.Statistics.EventParam.CHARGING;
import static com.mapswithme.util.statistics.Statistics.EventParam.COUNT_LOWERCASE;
import static com.mapswithme.util.statistics.Statistics.EventParam.DESTINATION;
import static com.mapswithme.util.statistics.Statistics.EventParam.ERROR;
import static com.mapswithme.util.statistics.Statistics.EventParam.ERROR_CODE;
import static com.mapswithme.util.statistics.Statistics.EventParam.ERROR_MESSAGE;
import static com.mapswithme.util.statistics.Statistics.EventParam.FEATURE_ID;
import static com.mapswithme.util.statistics.Statistics.EventParam.FILTER;
import static com.mapswithme.util.statistics.Statistics.EventParam.FILTERS;
import static com.mapswithme.util.statistics.Statistics.EventParam.FIRST_LAUNCH;
import static com.mapswithme.util.statistics.Statistics.EventParam.FROM;
import static com.mapswithme.util.statistics.Statistics.EventParam.HAS_AUTH;
import static com.mapswithme.util.statistics.Statistics.EventParam.HOTEL;
import static com.mapswithme.util.statistics.Statistics.EventParam.HOTEL_LAT;
import static com.mapswithme.util.statistics.Statistics.EventParam.HOTEL_LON;
import static com.mapswithme.util.statistics.Statistics.EventParam.ID;
import static com.mapswithme.util.statistics.Statistics.EventParam.INTERRUPTED;
import static com.mapswithme.util.statistics.Statistics.EventParam.ITEM;
import static com.mapswithme.util.statistics.Statistics.EventParam.MAP_DATA_SIZE;
import static com.mapswithme.util.statistics.Statistics.EventParam.METHOD;
import static com.mapswithme.util.statistics.Statistics.EventParam.MODE;
import static com.mapswithme.util.statistics.Statistics.EventParam.MWM_NAME;
import static com.mapswithme.util.statistics.Statistics.EventParam.MWM_VERSION;
import static com.mapswithme.util.statistics.Statistics.EventParam.NAME;
import static com.mapswithme.util.statistics.Statistics.EventParam.NETWORK;
import static com.mapswithme.util.statistics.Statistics.EventParam.OBJECT_LAT;
import static com.mapswithme.util.statistics.Statistics.EventParam.OBJECT_LON;
import static com.mapswithme.util.statistics.Statistics.EventParam.OPTION;
import static com.mapswithme.util.statistics.Statistics.EventParam.PLACEMENT;
import static com.mapswithme.util.statistics.Statistics.EventParam.PRODUCT;
import static com.mapswithme.util.statistics.Statistics.EventParam.PROVIDER;
import static com.mapswithme.util.statistics.Statistics.EventParam.PURCHASE;
import static com.mapswithme.util.statistics.Statistics.EventParam.RESTAURANT;
import static com.mapswithme.util.statistics.Statistics.EventParam.RESTAURANT_LAT;
import static com.mapswithme.util.statistics.Statistics.EventParam.RESTAURANT_LON;
import static com.mapswithme.util.statistics.Statistics.EventParam.SERVER_ID;
import static com.mapswithme.util.statistics.Statistics.EventParam.SERVER_IDS;
import static com.mapswithme.util.statistics.Statistics.EventParam.STATE;
import static com.mapswithme.util.statistics.Statistics.EventParam.TRIAL;
import static com.mapswithme.util.statistics.Statistics.EventParam.TURN_ON;
import static com.mapswithme.util.statistics.Statistics.EventParam.TYPE;
import static com.mapswithme.util.statistics.Statistics.EventParam.VALUE;
import static com.mapswithme.util.statistics.Statistics.EventParam.VENDOR;
import static com.mapswithme.util.statistics.Statistics.ParamValue.BACKUP;
import static com.mapswithme.util.statistics.Statistics.ParamValue.BICYCLE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.BOOKING_COM;
import static com.mapswithme.util.statistics.Statistics.ParamValue.DISK_NO_SPACE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.FACEBOOK;
import static com.mapswithme.util.statistics.Statistics.ParamValue.FALSE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.GOOGLE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.HOLIDAY;
import static com.mapswithme.util.statistics.Statistics.ParamValue.MAPSME;
import static com.mapswithme.util.statistics.Statistics.ParamValue.MAPSME_GUIDES;
import static com.mapswithme.util.statistics.Statistics.ParamValue.NO_BACKUP;
import static com.mapswithme.util.statistics.Statistics.ParamValue.OFFSCREEEN;
import static com.mapswithme.util.statistics.Statistics.ParamValue.OPENTABLE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.PARTNER;
import static com.mapswithme.util.statistics.Statistics.ParamValue.PEDESTRIAN;
import static com.mapswithme.util.statistics.Statistics.ParamValue.PHONE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.RESTORE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.SEARCH_BOOKING_COM;
import static com.mapswithme.util.statistics.Statistics.ParamValue.TAXI;
import static com.mapswithme.util.statistics.Statistics.ParamValue.TRAFFIC;
import static com.mapswithme.util.statistics.Statistics.ParamValue.TRANSIT;
import static com.mapswithme.util.statistics.Statistics.ParamValue.TRUE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.UNKNOWN;
import static com.mapswithme.util.statistics.Statistics.ParamValue.VEHICLE;

public enum Statistics implements Initializable<Context>
{
  INSTANCE;

  @NonNull
  public static ParameterBuilder makeInAppSuggestionParamBuilder()
  {
    return new ParameterBuilder()
        .add(EventParam.SCENARIO, BOOKING_COM)
        .add(PROVIDER, MAPSME_GUIDES);
  }

  @NonNull
  public static ParameterBuilder makeDownloaderBannerParamBuilder(@NonNull String provider,
                                                                  @NonNull String mwmId)
  {
    return new ParameterBuilder()
        .add(EventParam.FROM, ParamValue.MAP)
        .add(PROVIDER, provider)
        .add(MWM_NAME, mwmId);
  }

  @NonNull
  public static ParameterBuilder makeGuidesSubscriptionBuilder()
  {
    return new ParameterBuilder().add(EventParam.TARGET,
                                      ParamValue.GUIDES_SUBSCRIPTION);
  }

  public void trackCategoryDescChanged()
  {
    trackEditSettingsScreenOptionClick(ParamValue.ADD_DESC);
  }

  public void trackSharingOptionsClick(@NonNull String value)
  {
    ParameterBuilder builder = new ParameterBuilder().add(OPTION, value);
    trackEvent(EventName.BM_SHARING_OPTIONS_CLICK, builder);
  }

  public void trackSharingOptionsError(@NonNull String error,
                                       @NonNull NetworkErrorType value)
  {
    trackSharingOptionsError(error, value.ordinal());
  }

  public void trackSharingOptionsError(@NonNull String error, int value)
  {
    ParameterBuilder builder = new ParameterBuilder().add(EventParam.ERROR, value);
    trackEvent(error, builder);
  }

  public void trackSharingOptionsUploadSuccess(@NonNull BookmarkCategory category)
  {
    ParameterBuilder builder = new ParameterBuilder().add(EventParam.TRACKS, category.getTracksCount())
                                                     .add(EventParam.POINTS, category.getBookmarksCount());
    trackEvent(EventName.BM_SHARING_OPTIONS_UPLOAD_SUCCESS, builder);
  }

  public void trackBookmarkListSettingsClick(@NonNull Analytics analytics)
  {
    ParameterBuilder builder = ParameterBuilder.from(OPTION, analytics);
    trackEvent(EventName.BM_BOOKMARKS_LIST_SETTINGS_CLICK, builder);
  }

  @NonNull
  private static String getStatisticsSortingType(@BookmarkManager.SortingType int sortingType)
  {
    switch (sortingType)
    {
      case BookmarkManager.SORT_BY_TYPE:
        return Statistics.ParamValue.BY_TYPE;
      case BookmarkManager.SORT_BY_DISTANCE:
        return Statistics.ParamValue.BY_DISTANCE;
      case BookmarkManager.SORT_BY_TIME:
        return Statistics.ParamValue.BY_DATE;
    }
    throw new AssertionError("Invalid sorting type");
  }

  public void trackBookmarksListSort(@BookmarkManager.SortingType int sortingType)
  {
    trackBookmarksListSort(getStatisticsSortingType(sortingType));
  }

  public void trackBookmarksListResetSort()
  {
    trackBookmarksListSort(Statistics.ParamValue.BY_DEFAULT);
  }

  private void trackBookmarksListSort(@NonNull String value)
  {
    ParameterBuilder builder = new ParameterBuilder().add(EventParam.OPTION, value);
    trackEvent(EventName.BM_BOOKMARKS_LIST_SORT, builder);
  }

  public void trackBookmarksListSearch()
  {
    trackBookmarksSearch(ParamValue.BOOKMARKS_LIST);
  }

  private void trackBookmarksSearch(@NonNull String value)
  {
    ParameterBuilder builder = new ParameterBuilder().add(EventParam.FROM, value);
    trackEvent(EventName.BM_BOOKMARKS_SEARCH, builder);
  }

  public void trackBookmarksListSearchResultSelected()
  {
    trackBookmarksSearchResultSelected(ParamValue.BOOKMARKS_LIST);
  }

  private void trackBookmarksSearchResultSelected(@NonNull String value)
  {
    ParameterBuilder builder = new ParameterBuilder().add(EventParam.FROM, value);
    trackEvent(EventName.BM_BOOKMARKS_SEARCH_RESULT_SELECTED, builder);
  }

  public void trackBookmarksVisibility(@NonNull String from, @NonNull String action,
                                       @Nullable String serverId)
  {
    ParameterBuilder builder = new ParameterBuilder();
    builder.add(FROM, from).add(ACTION, action);
    if (!TextUtils.isEmpty(serverId))
      builder.add(SERVER_ID, serverId);
    trackEvent(BM_BOOKMARKS_VISIBILITY_CHANGE, builder.get());
  }

  private void trackEditSettingsScreenOptionClick(@NonNull String value)
  {
    ParameterBuilder builder = new ParameterBuilder().add(OPTION, value);
    trackEvent(EventName.BM_EDIT_SETTINGS_CLICK, builder);
  }

  public void trackEditSettingsCancel()
  {
    trackEvent(EventName.BM_EDIT_SETTINGS_CANCEL);
  }

  public void trackEditSettingsConfirm()
  {
    trackEvent(EventName.BM_EDIT_SETTINGS_CONFIRM);
  }

  public void trackEditSettingsSharingOptionsClick()
  {
    trackEditSettingsScreenOptionClick(Statistics.ParamValue.SHARING_OPTIONS);
  }

  public void trackBookmarkListSharingOptions()
  {
    trackEvent(Statistics.EventName.BM_BOOKMARKS_LIST_ITEM_SETTINGS,
               new Statistics.ParameterBuilder().add(OPTION,
                                                     Statistics.ParamValue.SHARING_OPTIONS));
  }

  public void trackSettingsDrivingOptionsChangeEvent(@NonNull String componentDescent)
  {
    boolean hasToll = RoutingOptions.hasOption(RoadType.Toll);
    boolean hasFerry = RoutingOptions.hasOption(RoadType.Ferry);
    boolean hasMoto = RoutingOptions.hasOption(RoadType.Motorway);
    boolean hasDirty = RoutingOptions.hasOption(RoadType.Dirty);

    ParameterBuilder builder = new ParameterBuilder();
    ParameterBuilder parameterBuilder = builder.add(EventParam.TOLL, hasToll ? 1 : 0)
                                               .add(EventParam.FERRY, hasFerry ? 1 : 0)
                                               .add(EventParam.MOTORWAY, hasMoto ? 1 : 0)
                                               .add(EventParam.UNPAVED, hasDirty ? 1 : 0);
    parameterBuilder.add(EventParam.FROM, componentDescent);

    trackEvent(EventName.SETTINGS_DRIVING_OPTIONS_CHANGE, parameterBuilder);
  }

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ PP_BANNER_STATE_PREVIEW, PP_BANNER_STATE_DETAILS })
  public @interface BannerState
  {
  }

  public static final int PP_BANNER_STATE_PREVIEW = 0;
  public static final int PP_BANNER_STATE_DETAILS = 1;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ STATISTICS_CHANNEL_DEFAULT, STATISTICS_CHANNEL_REALTIME })
  public @interface StatisticsChannel
  {
  }

  public static final int STATISTICS_CHANNEL_DEFAULT = org.alohalytics.Statistics.ONLY_CHANNEL;
  private static final int REALTIME_CHANNEL_INDEX = 1;
  public static final int STATISTICS_CHANNEL_REALTIME = STATISTICS_CHANNEL_DEFAULT | (1 << REALTIME_CHANNEL_INDEX);

  // Statistics counters
  private int mBookmarksCreated;
  private int mSharedTimes;

  public static class EventName
  {
    // Downloader
    public static final String DOWNLOADER_ERROR = "Downloader_Map_error";
    public static final String DOWNLOADER_ACTION = "Downloader_Map_action";
    public static final String DOWNLOADER_CANCEL = "Downloader_Cancel_downloading";
    public static final String DOWNLOADER_DIALOG_SHOW = "Downloader_OnStartScreen_show";
    public static final String DOWNLOADER_DIALOG_MANUAL_DOWNLOAD = "Downloader_OnStartScreen_manual_download";
    public static final String DOWNLOADER_DIALOG_DOWNLOAD = "Downloader_OnStartScreen_auto_download";
    public static final String DOWNLOADER_DIALOG_LATER = "Downloader_OnStartScreen_select_later";
    public static final String DOWNLOADER_DIALOG_HIDE = "Downloader_OnStartScreen_select_hide";
    public static final String DOWNLOADER_DIALOG_CANCEL = "Downloader_OnStartScreen_cancel_download";

    public static final String PLACEPAGE_DESCRIPTION_MORE = "Placepage_Description_more";
    public static final String PLACEPAGE_DESCRIPTION_OUTBOUND_CLICK = "Placepage_Description_Outbound_click";
    public static final String SETTINGS_SPEED_CAMS = "Settings. Speed_cameras";
    public static final String SETTINGS_MOBILE_INTERNET_CHANGE = "Settings_MobileInternet_change";
    public static final String SETTINGS_RECENT_TRACK_CHANGE = "Settings_RecentTrack_change";
    public static final String MOBILE_INTERNET_ALERT = "MobileInternet_alert";
    public static final String MAP_TOAST_SHOW = "Map_Toast_show";
    public static final String AUTH_ERROR = "Auth_error";
    static final String SETTINGS_TRACKING_DETAILS = "Settings_Tracking_details";
    static final String SETTINGS_TRACKING_TOGGLE = "Settings_Tracking_toggle";

    public static final String DOWNLOADER_BANNER_SHOW = "Downloader_Banner_show";
    public static final String DOWNLOADER_BANNER_CLICK = "Downloader_Banner_click";
    public static final String DOWNLOADER_FAB_CLICK = "Downloader_AddMap_click";
    public static final String DOWNLOADER_SEARCH_CLICK = "Downloader_Search_click";
    public static final String WHATS_NEW_ACTION = "WhatsNew_action";
    static final String DOWNLOADER_DIALOG_ERROR = "Downloader_OnStartScreen_error";

    // bookmarks
    public static final String BM_SHARING_OPTIONS_UPLOAD_ERROR = "Bookmarks_SharingOptions_upload_error";
    public static final String BM_SHARING_OPTIONS_ERROR = "Bookmarks_SharingOptions_error";
    public static final String BM_GROUP_CREATED = "Bookmark. Group created";
    public static final String BM_GROUP_CHANGED = "Bookmark. Group changed";
    public static final String BM_SYNC_PROPOSAL_ENABLED = "Bookmarks_SyncProposal_enabled";
    public static final String BM_SYNC_STARTED = "Bookmarks_sync_started";
    public static final String BM_EDIT_ON_WEB_CLICK = "Bookmarks_EditOnWeb_click";
    public static final String BM_RESTORE_PROPOSAL_CANCEL = "Bookmarks_RestoreProposal_cancel";
    public static final String BM_GUIDEDOWNLOADTOAST_SHOWN = "Bookmarks_GuideDownloadToast_shown";
    public static final String PP_DRIVING_OPTIONS_ACTION = "Placepage_DrivingOptions_action";
    static final String BM_SHARING_OPTIONS_CLICK = "Bookmarks_SharingOptions_click";
    static final String BM_EDIT_SETTINGS_CLICK = "Bookmarks_Bookmark_Settings_click";
    static final String BM_EDIT_SETTINGS_CANCEL = "Bookmarks_Bookmark_Settings_cancel";
    static final String BM_EDIT_SETTINGS_CONFIRM = "Bookmarks_Bookmark_Settings_confirm";
    static final String BM_BOOKMARKS_LIST_SETTINGS_CLICK = "Bookmarks_BookmarksList_settings_click";
    static final String BM_BOOKMARKS_LIST_ITEM_SETTINGS = "Bookmarks_BookmarksListItem_settings";
    static final String BM_BOOKMARKS_LIST_SORT = "Bookmarks_BookmarksList_sort";
    static final String BM_BOOKMARKS_SEARCH = "Bookmarks_Search";
    static final String BM_BOOKMARKS_SEARCH_RESULT_SELECTED = "Bookmarks_Search_result_selected";
    static final String BM_BOOKMARKS_VISIBILITY_CHANGE = "Bookmarks_Visibility_change";
    static final String BM_COLOR_CHANGED = "Bookmark. Color changed";
    static final String BM_CREATED = "Bookmark. Bookmark created";
    static final String BM_SYNC_PROPOSAL_SHOWN = "Bookmarks_SyncProposal_shown";
    static final String BM_SYNC_PROPOSAL_APPROVED = "Bookmarks_SyncProposal_approved";
    static final String BM_SYNC_PROPOSAL_ERROR = "Bookmarks_SyncProposal_error";
    static final String BM_SYNC_PROPOSAL_TOGGLE = "Settings_BookmarksSync_toggle";
    static final String BM_SYNC_ERROR = "Bookmarks_sync_error";
    static final String BM_SYNC_SUCCESS = "Bookmarks_sync_success";
    static final String BM_RESTORE_PROPOSAL_CLICK = "Bookmarks_RestoreProposal_click";
    static final String BM_RESTORE_PROPOSAL_SUCCESS = "Bookmarks_RestoreProposal_success";
    static final String BM_RESTORE_PROPOSAL_ERROR = "Bookmarks_RestoreProposal_error";
    static final String BM_TAB_CLICK = "Bookmarks_Tab_click";
    static final String BM_GUIDES_DOWNLOADDIALOGUE_CLICK = "Bookmarks_Guides_DownloadDialogue_click";
    static final String SETTINGS_DRIVING_OPTIONS_CHANGE = "Settings_Navigation_DrivingOptions_change";
    private static final String BM_SHARING_OPTIONS_UPLOAD_SUCCESS = "Bookmarks_SharingOptions_upload_success";
    private static final String BM_DOWNLOADED_CATALOGUE_OPEN = "Bookmarks_Downloaded_Catalogue_open";
    private static final String BM_DOWNLOADED_CATALOGUE_ERROR = "Bookmarks_Downloaded_Catalogue_error";
    public static final String BM_BOOKMARKS_LIST_COLLECTION_SELECT = "Bookmarks_BookmarksList_Collection_select";
    public static final String BM_BOOKMARKS_LIST_CATEGORY_SELECT = "Bookmarks_BookmarksList_Category_select";
    public static final String BM_GUIDE_VISIBILITY_CHANGE = "Bookmarks_Guide_Visibility_change";

    // search
    public static final String SEARCH_ITEM_CLICKED = "Search. Key clicked";
    public static final String SEARCH_ON_MAP_CLICKED = "Search. View on map clicked.";
    public static final String SEARCH_SPONSOR_CATEGORY_SHOWN = "Search_SponsoredCategory_shown";
    public static final String SEARCH_SPONSOR_CATEGORY_SELECTED = "Search_SponsoredCategory_selected";
    static final String SEARCH_FILTER_OPEN = "Search_Filter_Open";
    public static final String SEARCH_FILTER_CANCEL = "Search_Filter_Cancel";
    public static final String SEARCH_FILTER_RESET = "Search_Filter_Reset";
    public static final String SEARCH_FILTER_APPLY = "Search_Filter_Apply";
    static final String SEARCH_QUICKFILTER_OPEN = "Search_QuickFilter_Open";
    static final String SEARCH_QUICKFILTER_CLICK = "Search_QuickFilter_Click";
    static final String SEARCH_QUICKFILTER_APPLY = "Search_QuickFilter_Apply";
    static final String SEARCH_QUICKFILTER_CLICK_ERROR = "Search_QuickFilter_Click_error";
    static final String SEARCH_CONTEXTAREA_CLICK = "Search_ContextArea_Click";
    static final String SEARCH_CAT_CLICKED = "Search. Category clicked";
    static final String SEARCH_TAB_SELECTED = "Search_Tab_selected";
    static final String SEARCH_FILTER_CLICK = "Search_Filter_Click";

    // place page
    public static final String PP_SHARE = "PP. Share";
    public static final String PP_BOOKMARK = "PP. Bookmark";
    public static final String PP_SPONSORED_DETAILS = "Placepage_Hotel_details";
    public static final String PP_SPONSORED_OPENTABLE = "Placepage_Restaurant_book";
    public static final String PP_SPONSORED_ACTION = "Placepage_SponsoredActionButton_click";
    public static final String PP_SPONSOR_MORE_SELECTED = "Placepage_SponsoredGallery_MoreItem_selected";
    public static final String PP_DIRECTION_ARROW = "PP. DirectionArrow";
    public static final String PP_DIRECTION_ARROW_CLOSE = "PP. DirectionArrowClose";
    public static final String PP_METADATA_COPY = "PP. CopyMetadata";
    public static final String PP_BANNER_CLICK = "Placepage_Banner_click";
    public static final String PP_HOTEL_GALLERY_OPEN = "PlacePage_Hotel_Gallery_open";
    public static final String PP_HOTEL_REVIEWS_LAND = "PlacePage_Hotel_Reviews_land";
    public static final String PP_HOTEL_DESCRIPTION_LAND = "PlacePage_Hotel_Description_land";
    public static final String PP_HOTEL_FACILITIES = "PlacePage_Hotel_Facilities_open";
    public static final String PP_HOTEL_SEARCH_SIMILAR = "Placepage_Hotel_search_similar";
    static final String PP_DETAILS_OPEN = "Placepage_Details_open";
    static final String PP_SPONSORED_BOOK = "Placepage_Hotel_book";
    static final String PP_SPONSORED_OPEN = "Placepage_SponsoredGalleryPage_opened";
    static final String PP_SPONSORED_SHOWN = "Placepage_SponsoredGallery_shown";
    static final String PP_SPONSORED_ERROR = "Placepage_SponsoredGallery_error";
    static final String PP_SPONSOR_ITEM_SELECTED = "Placepage_SponsoredGallery_ProductItem_selected";
    static final String PP_SPONSORED_USER_ITEM = "Placepage_SponsoredGallery_UsersItem_shown";
    static final String PP_BANNER_SHOW = "Placepage_Banner_show";
    static final String PP_BANNER_ERROR = "Placepage_Banner_error";
    static final String PP_BANNER_BLANK = "Placepage_Banner_blank";
    static final String PP_BANNER_CLOSE = "Placepage_Banner_close";
    static final String PP_OWNERSHIP_BUTTON_CLICK = "Placepage_OwnershipButton_click";

    //elevation profile
    static final String ELEVATION_PROFILE_PAGE_OPEN = "ElevationProfilePage_open";
    static final String ELEVATION_PROFILE_PAGE_CLOSE = "ElevationProfilePage_close";

    // toolbar actions
    public static final String TOOLBAR_MY_POSITION = "Toolbar. MyPosition";
    static final String TOOLBAR_CLICK = "Toolbar_click";
    static final String TOOLBAR_MENU_CLICK = "Toolbar_Menu_click";

    // dialogs
    public static final String RATE_DIALOG_LATER = "GPlay dialog cancelled.";
    public static final String FACEBOOK_INVITE_LATER = "Facebook invites dialog cancelled.";
    public static final String FACEBOOK_INVITE_INVITED = "Facebook invites dialog accepted.";
    static final String RATE_DIALOG_RATED = "GPlay dialog. Rating set";

    // misc
    public static final String ZOOM_IN = "Zoom. In";
    public static final String ZOOM_OUT = "Zoom. Out";
    public static final String DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN = "Download country notification shown";
    public static final String TTS_FAILURE_LOCATION = "TTS failure location";
    public static final String UGC_NOT_AUTH_NOTIFICATION_SHOWN = "UGC_UnsentNotification_shown";
    public static final String UGC_NOT_AUTH_NOTIFICATION_CLICKED = "UGC_UnsentNotification_clicked";
    public static final String UGC_REVIEW_NOTIFICATION_SHOWN = "UGC_ReviewNotification_shown";
    public static final String UGC_REVIEW_NOTIFICATION_CLICKED = "UGC_ReviewNotification_clicked";
    static final String PLACE_SHARED = "Place Shared";
    static final String API_CALLED = "API called";
    static final String ACTIVE_CONNECTION = "Connection";
    static final String STATISTICS_STATUS_CHANGED = "Statistics status changed";

    // routing
    public static final String ROUTING_START_SUGGEST_REBUILD = "Routing. Suggest rebuild";
    public static final String ROUTING_CANCEL = "Routing. Cancel";
    public static final String ROUTING_VEHICLE_SET = "Routing. Set vehicle";
    public static final String ROUTING_PEDESTRIAN_SET = "Routing. Set pedestrian";
    public static final String ROUTING_BICYCLE_SET = "Routing. Set bicycle";
    public static final String ROUTING_TAXI_SET = "Routing. Set taxi";
    public static final String ROUTING_TRANSIT_SET = "Routing. Set transit";
    public static final String ROUTING_SWAP_POINTS = "Routing. Swap points";
    public static final String ROUTING_SETTINGS = "Routing. Settings";
    public static final String ROUTING_TAXI_CLICK_IN_PP = "Placepage_Taxi_click";
    public static final String ROUTING_TAXI_ROUTE_BUILT = "Routing_Build_Taxi";
    public static final String ROUTING_POINT_ADD = "Routing_Point_add";
    public static final String ROUTING_POINT_REMOVE = "Routing_Point_remove";
    public static final String ROUTING_SEARCH_CLICK = "Routing_Search_click";
    public static final String ROUTING_BOOKMARKS_CLICK = "Routing_Bookmarks_click";
    static final String ROUTING_BUILD = "Routing. Build";
    static final String ROUTING_ROUTE_START = "Routing_Route_start";
    static final String ROUTING_ROUTE_FINISH = "Routing_Route_finish";
    static final String ROUTING_TAXI_ORDER = "Routing_Taxi_order";
    static final String ROUTING_TAXI_INSTALL = "Routing_Taxi_install";
    static final String ROUTING_TAXI_SHOW = "Placepage_Taxi_show";
    static final String ROUTING_PLAN_TOOLTIP_CLICK = "Routing_PlanTooltip_click";

    // editor
    public static final String EDITOR_ADD_CLICK = "Editor_Add_click";
    public static final String EDITOR_AUTH_DECLINED = "Editor_Auth_declined_by_user";
    public static final String EDITOR_AUTH_REQUEST_RESULT = "Editor_Auth_request_result";
    public static final String EDITOR_REG_REQUEST = "Editor_Reg_request";
    public static final String EDITOR_LOST_PASSWORD = "Editor_Lost_password";
    public static final String EDITOR_SHARE_SHOW = "Editor_SecondTimeShare_show";
    public static final String EDITOR_SHARE_CLICK = "Editor_SecondTimeShare_click";
    static final String EDITOR_START_CREATE = "Editor_Add_start";
    static final String EDITOR_START_EDIT = "Editor_Edit_start";
    static final String EDITOR_SUCCESS_CREATE = "Editor_Add_success";
    static final String EDITOR_SUCCESS_EDIT = "Editor_Edit_success";
    static final String EDITOR_ERROR_CREATE = "Editor_Add_error";
    static final String EDITOR_ERROR_EDIT = "Editor_Edit_error";
    static final String EDITOR_AUTH_REQUEST = "Editor_Auth_request";

    // Cold start
    static final String APPLICATION_COLD_STARTUP_INFO = "Application_ColdStartup_info";

    // Ugc.
    public static final String UGC_REVIEW_CANCEL = "UGC_Review_cancel";
    public static final String UGC_REVIEW_SUCCESS = "UGC_Review_success";
    public static final String UGC_AUTH_SHOWN = "UGC_Auth_shown";
    public static final String UGC_AUTH_DECLINED = "UGC_Auth_declined";
    static final String UGC_REVIEW_START = "UGC_Review_start";
    static final String UGC_AUTH_EXTERNAL_REQUEST_SUCCESS = "UGC_Auth_external_request_success";
    static final String UGC_AUTH_ERROR = "UGC_Auth_error";
    static final String MAP_LAYERS_CLICK = "Map_Layers_click";

    public static final String AUTH_SHOWN = "Auth_shown";
    public static final String AUTH_DECLINED = "Auth_declined";
    public static final String AUTH_START = "Auth_start";

    // Purchases.
    public static final String INAPP_PURCHASE_PREVIEW_PAY = "InAppPurchase_Preview_pay";
    public static final String INAPP_PURCHASE_PREVIEW_CANCEL = "InAppPurchase_Preview_cancel";
    public static final String INAPP_PURCHASE_PREVIEW_RESTORE = "InAppPurchase_Preview_restore";
    public static final String INAPP_PURCHASE_STORE_SUCCESS = "InAppPurchase_Store_success";
    public static final String INAPP_PURCHASE_VALIDATION_SUCCESS = "InAppPurchase_Validation_success";
    static final String INAPP_PURCHASE_PREVIEW_SHOW = "InAppPurchase_Preview_show";
    static final String INAPP_PURCHASE_PREVIEW_SELECT = "InAppPurchase_Preview_select";
    static final String INAPP_PURCHASE_STORE_ERROR = "InAppPurchase_Store_error";
    static final String INAPP_PURCHASE_VALIDATION_ERROR = "InAppPurchase_Validation_error";
    static final String INAPP_PURCHASE_PRODUCT_DELIVERED = "InAppPurchase_Product_delivered";

    public static final String ONBOARDING_SCREEN_SHOW = "OnboardingScreen_show";
    public static final String ONBOARDING_SCREEN_ACCEPT = "OnboardingScreen_accept";
    public static final String ONBOARDING_SCREEN_DECLINE = "OnboardingScreen_decline";
    public static final String ONBOARDING_DEEPLINK_SCREEN_SHOW = "OnboardingDeeplinkScreen_show";
    public static final String ONBOARDING_DEEPLINK_SCREEN_ACCEPT = "OnboardingDeeplinkScreen_accept";
    public static final String ONBOARDING_DEEPLINK_SCREEN_DECLINE = "OnboardingDeeplinkScreen_decline";

    public static final String TIPS_TRICKS_SHOW = "TipsTricks_show";
    public static final String TIPS_TRICKS_CLICK = "TipsTricks_click";
    static final String TIPS_TRICKS_CLOSE = "TipsTricks_close";

    public static final String INAPP_SUGGESTION_SHOWN = "MapsMe_InAppSuggestion_shown";
    public static final String INAPP_SUGGESTION_CLICKED = "MapsMe_InAppSuggestion_clicked";
    public static final String INAPP_SUGGESTION_CLOSED = "MapsMe_InAppSuggestion_closed";

    public static final String MAP_SPONSORED_BUTTON_CLICK = "Map_SponsoredButton_click";
    public static final String MAP_SPONSORED_BUTTON_SHOW = "Map_SponsoredButton_show";
    static final String GUIDES_SHOWN = "Bookmarks_Downloaded_Guides_list";
    static final String GUIDES_OPEN = "Bookmarks_Downloaded_Guide_open";
    static final String GUIDES_BOOKMARK_SELECT = "Bookmarks_BookmarksList_Bookmark_select";
    static final String GUIDES_TRACK_SELECT = "Bookmarks_BookmarksList_Track_select";

    public static final String DEEPLINK_CALL = "Deeplink_call";
    public static final String DEEPLINK_CALL_MISSED = "Deeplink_call_missed";
    public static final String AUTH_EXTERNAL_REQUEST_SUCCESS = "Auth_external_request_success";
    public static final String AUTH_REQUEST_SUCCESS = "Auth_request_success";

    public static class Settings
    {
      public static final String WEB_SITE = "Setings. Go to website";
      public static final String FEEDBACK_GENERAL = "Send general feedback to android@maps.me";
      public static final String REPORT_BUG = "Settings. Bug reported";
      public static final String RATE = "Settings. Rate app called";
      public static final String TELL_FRIEND = "Settings. Tell to friend";
      public static final String FACEBOOK = "Settings. Go to FB.";
      public static final String TWITTER = "Settings. Go to twitter.";
      public static final String HELP = "Settings. Help.";
      public static final String ABOUT = "Settings. About.";
      public static final String OSM_PROFILE = "Settings. Profile.";
      public static final String COPYRIGHT = "Settings. Copyright.";
      public static final String UNITS = "Settings. Change units.";
      public static final String ZOOM = "Settings. Switch zoom.";
      public static final String MAP_STYLE = "Settings. Map style.";
      public static final String VOICE_ENABLED = "Settings. Switch voice.";
      public static final String VOICE_LANGUAGE = "Settings. Voice language.";
      static final String ENERGY_SAVING = "Settings_EnergySaving_change";

      private Settings()
      {
      }
    }

    private EventName()
    {
    }
  }

  public static class EventParam
  {
    public static final String FROM = "from";
    public static final String TO = "to";
    public static final String OPTION = "option";
    public static final String URL = "url";
    public static final String SETTINGS = "settings";
    public static final String ROUTE = "route";
    public static final String BUTTON = "button";
    public static final String SCREEN = "screen";
    public static final String VERSION = "version";
    public static final String TAB = "tab";
    public static final String ENABLED = "Enabled";
    public static final String TURN_ON = "turn_on";
    public static final String RATING = "Rating";
    public static final String LANGUAGE = "language";
    public static final String NAME = "Name";
    public static final String ACTION = "action";
    public static final String TYPE = "type";
    public static final String IS_SUCCESS = "is_success_message";
    public static final String ERR_MSG = "error_message";
    public static final String OSM = "OSM";
    public static final String FACEBOOK = "Facebook";
    public static final String PROVIDER = "provider";
    public static final String HOTEL = "hotel";
    public static final String ERROR = "error";
    public static final String VALUE = "value";
    public static final String PRICE_CATEGORY = "price_category";
    public static final String DATE = "date";
    public static final String STATUS = "status";
    public static final String SOURCE = "source";
    public static final String GUIDE_CATALOGUE = "guide_catalogue";
    public static final String SUBSCRIPTION = "subscription";
    public static final String EXPORT_BOOKMARKS = "export_bookmarks";
    public static final String BOOKMARKS_BACKUP = "bookmarks_backup";
    public static final String AFTER_SAVE_REVIEW = "after_save_review";
    public static final String ROOMS = "rooms";
    public static final String ADULTS = "adults";
    public static final String CHILDREN = "children";
    public static final String INFANTS = "infants";
    static final String BANNER = "banner";
    static final String BATTERY = "battery";
    static final String CALLER_ID = "Caller ID";
    static final String CATEGORY = "category";
    static final String CHANNEL = "Channel";
    static final String CHARGING = "charging";
    static final String CONNECTION_FAST = "Connection fast";
    static final String CONNECTION_METERED = "Connection limit";
    static final String CONNECTION_TYPE = "Connection name";
    static final String COUNT = "Count";
    static final String COUNT_LOWERCASE = "count";
    static final String DESTINATION = "destination";
    static final String ERROR_CODE = "error_code";
    static final String ERROR_MESSAGE = "error_message";
    static final String FEATURE_ID = "feature_id";
    static final String FERRY = "ferry";
    public static final String FILTER = "filter";
    static final String FILTERS = "filters";
    static final String FIRST_LAUNCH = "first_launch";
    static final String FROM_LAT = "from_lat";
    static final String FROM_LON = "from_lon";
    static final String HAS_AUTH = "has_auth";
    static final String HOTEL_LAT = "hotel_lat";
    static final String HOTEL_LON = "hotel_lon";
    static final String ID = "id";
    static final String INTERRUPTED = "interrupted";
    static final String IS_AUTHENTICATED = "is_authenticated";
    static final String IS_ONLINE = "is_online";
    static final String ITEM = "item";
    static final String MAP_DATA_SIZE = "map_data_size:";
    static final String METHOD = "method";
    static final String MODE = "mode";
    static final String MOTORWAY = "motorway";
    static final String MWM_NAME = "mwm_name";
    static final String MWM_VERSION = "mwm_version";
    static final String MY_POSITION = "my position";
    static final String NETWORK = "network";
    static final String OBJECT_LAT = "object_lat";
    static final String OBJECT_LON = "object_lon";
    static final String PLACEMENT = "placement";
    static final String POINT = "point";
    static final String POINTS = "points";
    static final String PRODUCT = "product";
    static final String PURCHASE = "purchase";
    static final String RESTAURANT = "restaurant";
    static final String RESTAURANT_LAT = "restaurant_lat";
    static final String RESTAURANT_LON = "restaurant_lon";
    static final String SCENARIO = "scenario";
    static final String SERVER_ID = "server_id";
    static final String SERVER_IDS = "server_ids";
    static final String STATE = "state";
    static final String TARGET = "target";
    static final String TO_LAT = "to_lat";
    static final String TO_LON = "to_lon";
    static final String TOLL = "toll";
    static final String TRACKS = "tracks";
    static final String TRIAL = "trial";
    static final String UNPAVED = "unpaved";
    static final String VENDOR = "vendor";

    private EventParam()
    {
    }
  }

  public static class ParamValue
  {
    public static final String BOOKING_COM = "Booking.Com";
    public static final String OSM = "OSM";
    public static final String ON = "on";
    public static final String OFF = "off";
    public static final String SHARING_OPTIONS = "sharing_options";
    public static final String EDIT_ON_WEB = "edit_on_web";
    public static final String PUBLIC = "public";
    public static final String PRIVATE = "private";
    public static final String COPY_LINK = "copy_link";
    public static final String CANCEL = "Cancel";
    public static final String ALWAYS = "always";
    public static final String NEVER = "never";
    public static final String ASK = "ask";
    public static final String TODAY = "today";
    public static final String NOT_TODAY = "not_today";
    public static final String CARD = "card";
    public static final String SPONSORED_BUTTON = "sponsored_button";
    public static final String POPUP = "popup";
    public static final String WEBVIEW = "webview";
    public static final String WHATSNEW = "whatsnew";
    public static final String ONBOARDING_GUIDES_SUBSCRIPTION = "onboarding_guides_subscription";
    public static final String PLUS = "plus";
    public static final String DOWNLOAD = "download";
    public static final String OPEN = "open";
    public static final String CLOSE = "close";
    public static final String NEXT = "next";
    public static final String NO_PRODUCTS = "no_products";
    public static final String EDIT = "edit";
    public static final String FACEBOOK = "facebook";
    public static final String CHECKIN = "check_in";
    public static final String CHECKOUT = "check_out";
    public static final String ANY = "any";
    public static final String MAPSME = "mapsme";
    public static final String UNKNOWN = "unknown";
    public static final String NO_INTERNET = "no_internet";
    public static final String MY = "my";
    public static final String DOWNLOADED = "downloaded";
    public static final String SUCCESS = "success";
    public static final String UNAVAILABLE = "unavailable";
    public final static String VIEW_ON_MAP = "view on map";
    public final static String NOT_NOW = "not now";
    public final static String CLICK_OUTSIDE = "click outside pop-up";
    public static final String SEND_AS_FILE = "send_as_file";
    public static final String MAKE_INVISIBLE_ON_MAP = "make_invisible_on_map";
    public static final String LIST_SETTINGS = "list_settings";
    public static final String DELETE_GROUP = "delete_group";
    public static final String OFFSCREEEN = "Offscreen";
    public static final String MAPSME_GUIDES = "MapsMeGuides";
    public static final String TINKOFF_INSURANCE = "Tinkoff_Insurance";
    public static final String TINKOFF_ALL_AIRLINES = "Tinkoff_AllAirlines";
    public static final String SKYENG = "Skyeng";
    public static final String MTS = "MTS";
    public static final String MASTERCARD_SBERBANK = "MastercardSberbank";
    public static final String ARSENAL_MEDIC = "Arsenal_telemed";
    public static final String ARSENAL_FLAT = "Arsenal_flat";
    public static final String ARSENAL_INSURANCE_CRIMEA = "Arsenal_crimea";
    public static final String ARSENAL_INSURANCE_RUSSIA = "Arsenal_russia";
    public static final String ARSENAL_INSURANCE_WORLD = "Arsenal_abroad";
    public static final String WIKIPEDIA = "wikipedia";
    public static final String PREVIEW = "preview";
    public static final String FULL = "full";
    public static final String MAP_GALLERY = "map_gallery";
    public static final String BOOKMARK_LIST = "bookmark_list";
    public static final String SHOW = "show";
    public static final String HIDE = "hide";
    public static final String SHOW_ALL = "show_all";
    public static final String HIDE_ALL = "hide_all";
    public static final String NEW_OBJECT = "new_object";
    public static final String EDIT_OBJECT = "edit_object";
    public static final String DATE = "date";
    public static final String ROOMS = "rooms";
    public static final String MAIN = "main";
    public static final String COLLECTION = "collection";
    public static final String CATEGORY = "category";
    static final String CRASH_REPORTS = "crash_reports";
    static final String PERSONAL_ADS = "personal_ads";
    public static final String MAP = "map";
    public static final String LIST = "list";
    public static final String MENU = "menu";
    public static final String ISOLINES = "isolines";
    public static final String NOTIFICATION = "notification";
    static final String GOOGLE = "google";
    static final String PHONE = "phone";
    static final String ADD_DESC = "add_description";
    static final String BY_DEFAULT = "Default";
    static final String BY_DATE = "Date";
    static final String BY_DISTANCE = "Distance";
    static final String BY_TYPE = "Type";
    static final String BOOKMARKS_LIST = "BookmarksList";
    static final String GUIDES_SUBSCRIPTION = "OnboardingGuidesSubscription";
    static final String SEARCH_BOOKING_COM = "Search.Booking.Com";
    static final String OPENTABLE = "OpenTable";
    static final String LOCALS_EXPERTS = "Locals.Maps.Me";
    static final String SEARCH_RESTAURANTS = "Search.Restaurants";
    static final String SEARCH_ATTRACTIONS = "Search.Attractions";
    static final String HOLIDAY = "Holiday";
    static final String ADD = "add";
    static final String AFTER_SAVE = "after_save";
    static final String PLACEPAGE_PREVIEW = "placepage_preview";
    static final String PLACEPAGE = "placepage";
    static final String NETWORK = "network";
    static final String DISK = "disk";
    static final String AUTH = "auth";
    static final String USER_INTERRUPTED = "user_interrupted";
    static final String INVALID_CALL = "invalid_call";
    static final String NO_BACKUP = "no_backup";
    static final String DISK_NO_SPACE = "disk_no_space";
    static final String BACKUP = "backup";
    static final String RESTORE = "restore";
    static final String SUBWAY = "subway";
    static final String TRAFFIC = "traffic";
    static final String PEDESTRIAN = "pedestrian";
    static final String VEHICLE = "vehicle";
    static final String BICYCLE = "bicycle";
    static final String TAXI = "taxi";
    static final String TRANSIT = "transit";
    static final String PARTNER = "Partner";
    static final String TRUE = "True";
    static final String FALSE = "False";
  }

  // Initialized once in constructor and does not change until the process restarts.
  // In this way we can correctly finish all statistics sessions and completely
  // avoid their initialization if user has disabled statistics collection.
  private boolean mEnabled;

  @Override
  public void initialize(@Nullable Context context)
  {
    mEnabled = SharedPropertiesUtils.isStatisticsEnabled(context);
    // At the moment we need special handling for Alohalytics to enable/disable logging of events in core C++ code.
    if (mEnabled)
      org.alohalytics.Statistics.enable(context);
    else
      org.alohalytics.Statistics.disable(context);
    configure(context);
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  @SuppressWarnings("NullableProblems")
  @NonNull
  private ExternalLibrariesMediator mMediator;

  public void setMediator(@NonNull ExternalLibrariesMediator mediator)
  {
    mMediator = mediator;
  }

  private void configure(Context context)
  {
    // At the moment, need to always initialize engine for correct JNI http part reusing.
    // Statistics is still enabled/disabled separately and never sent anywhere if turned off.
    // TODO (AlexZ): Remove this initialization dependency from JNI part.
    org.alohalytics.Statistics.setDebugMode(BuildConfig.DEBUG);
    org.alohalytics.Statistics.setup(new String[] { PrivateVariables.alohalyticsUrl(),
                                                    PrivateVariables.alohalyticsRealtimeUrl() }, context);
  }

  public void trackEvent(@NonNull String name)
  {
    trackEvent(name, STATISTICS_CHANNEL_DEFAULT);
  }

  public void trackEvent(@NonNull String name, @StatisticsChannel int channel)
  {
    if (mEnabled)
      org.alohalytics.Statistics.logEvent(name, channel);
    mMediator.getEventLogger().logEvent(name, Collections.emptyMap());
  }

  public void trackEvent(@NonNull String name, @NonNull Map<String, String> params)
  {
    trackEvent(name, params, STATISTICS_CHANNEL_DEFAULT);
  }

  public void trackEvent(@NonNull String name, @NonNull Map<String, String> params,
                         @StatisticsChannel int channel)
  {
    if (mEnabled)
      org.alohalytics.Statistics.logEvent(name, params, channel);

    mMediator.getEventLogger().logEvent(name, params);
  }

  public void trackEvent(@NonNull String name, @Nullable Location location,
                         @NonNull Map<String, String> params)
  {
    trackEvent(name, location, params, STATISTICS_CHANNEL_DEFAULT);
  }

  public void trackEvent(@NonNull String name, @Nullable Location location,
                         @NonNull Map<String, String> params, @StatisticsChannel int channel)
  {
    List<String> eventDictionary = new ArrayList<String>();
    for (Map.Entry<String, String> entry : params.entrySet())
    {
      eventDictionary.add(entry.getKey());
      eventDictionary.add(entry.getValue());
    }
    params.put("lat", (location == null ? "N/A" : String.valueOf(location.getLatitude())));
    params.put("lon", (location == null ? "N/A" : String.valueOf(location.getLongitude())));

    if (mEnabled)
      org.alohalytics.Statistics.logEvent(name, eventDictionary.toArray(new String[0]), location, channel);

    mMediator.getEventLogger().logEvent(name, params);
  }

  public void trackEvent(@NonNull String name, @NonNull ParameterBuilder builder)
  {
    trackEvent(name, builder.get(), STATISTICS_CHANNEL_DEFAULT);
  }

  public void trackEvent(@NonNull String name, @NonNull ParameterBuilder builder,
                         @StatisticsChannel int channel)
  {
    trackEvent(name, builder.get(), channel);
  }

  public void startActivity(Activity activity)
  {
    if (mEnabled)
    {
      AppEventsLogger.activateApp(activity.getApplication());
      org.alohalytics.Statistics.onStart(activity);
    }

    mMediator.getEventLogger().startActivity(activity);
  }

  public void stopActivity(Activity activity)
  {
    if (mEnabled)
    {
      org.alohalytics.Statistics.onStop(activity);
    }
    mMediator.getEventLogger().stopActivity(activity);
  }

  public void setStatEnabled(@NonNull Context context, boolean isEnabled)
  {
    SharedPropertiesUtils.setStatisticsEnabled(context, isEnabled);
    Config.setStatisticsEnabled(isEnabled);

    // We track if user turned on/off statistics to understand data better.
    trackEvent(EventName.STATISTICS_STATUS_CHANGED + " " + Counters.getInstallFlavor(context),
               params().add(EventParam.ENABLED, String.valueOf(isEnabled)));
  }

  public void trackSearchTabSelected(@NonNull String tab)
  {
    trackEvent(EventName.SEARCH_TAB_SELECTED, params().add(EventParam.TAB, tab));
  }

  public void trackSearchCategoryClicked(String category)
  {
    trackEvent(EventName.SEARCH_CAT_CLICKED, params().add(EventParam.CATEGORY, category));
  }

  public void trackColorChanged(String from, String to)
  {
    trackEvent(EventName.BM_COLOR_CHANGED, params().add(EventParam.FROM, from)
                                                   .add(EventParam.TO, to));
  }

  public void trackBookmarkCreated()
  {
    trackEvent(EventName.BM_CREATED, params().add(EventParam.COUNT, String.valueOf(++mBookmarksCreated)));
  }

  public void trackPlaceShared(String channel)
  {
    trackEvent(EventName.PLACE_SHARED, params().add(EventParam.CHANNEL, channel)
                                               .add(EventParam.COUNT, String.valueOf(++mSharedTimes)));
  }

  public void trackApiCall(@NonNull ParsedMwmRequest request)
  {
    trackEvent(EventName.API_CALLED, params().add(EventParam.CALLER_ID, request.getCallerInfo() == null ?
                                                                        "null" :
                                                                        request.getCallerInfo().packageName));
  }

  public void trackRatingDialog(float rating)
  {
    trackEvent(EventName.RATE_DIALOG_RATED, params().add(EventParam.RATING, String.valueOf(rating)));
  }

  public void trackConnectionState()
  {
    if (ConnectionState.INSTANCE.isConnected())
    {
      final NetworkInfo info = ConnectionState.INSTANCE.getActiveNetwork();
      boolean isConnectionMetered = false;
      //noinspection ConstantConditions
      trackEvent(EventName.ACTIVE_CONNECTION,
                 params().add(EventParam.CONNECTION_TYPE, info.getTypeName() + ":" + info.getSubtypeName())
                         .add(EventParam.CONNECTION_FAST, String.valueOf(ConnectionState.INSTANCE.isConnectionFast(info)))
                         .add(EventParam.CONNECTION_METERED, String.valueOf(isConnectionMetered)));
    }
    else
      trackEvent(EventName.ACTIVE_CONNECTION, params().add(EventParam.CONNECTION_TYPE, "Not connected."));
  }

  // FIXME Call to track map changes to MyTracker to correctly deal with preinstalls.
  public void trackMapChanged(String event)
  {
    if (mEnabled)
    {
      final ParameterBuilder params = params().add(EventParam.COUNT, String.valueOf(MapManager.nativeGetDownloadedCount()));
      trackEvent(event, params);
    }
  }

  public void trackRouteBuild(int routerType, MapObject from, MapObject to)
  {
    trackEvent(EventName.ROUTING_BUILD, params().add(EventParam.FROM, Statistics.getPointType(from))
                                                .add(EventParam.TO, Statistics.getPointType(to)));
  }

  public void trackEditorLaunch(boolean newObject, String valueOfIsAuthorized)
  {
    trackEvent(newObject ? EventName.EDITOR_START_CREATE : EventName.EDITOR_START_EDIT,
               editorMwmParams().add(EventParam.IS_AUTHENTICATED, valueOfIsAuthorized)
                                .add(EventParam.IS_ONLINE, String.valueOf(ConnectionState.INSTANCE.isConnected())));

    if (newObject)
      PushwooshHelper.nativeSendEditorAddObjectTag();
    else
      PushwooshHelper.nativeSendEditorEditObjectTag();
  }

  public void trackMapLayerClick(@NonNull Mode mode, @NonNull String from, boolean turnOn)
  {
    ParameterBuilder builder = params().add(NAME, mode.name().toLowerCase(Locale.ENGLISH))
                                       .add(FROM, from).add(TURN_ON, turnOn);
    trackEvent(EventName.MAP_LAYERS_CLICK, builder);
  }

  public void trackEditorSuccess(boolean newObject, String valueOfIsAuthorized)
  {
    trackEvent(newObject ? EventName.EDITOR_SUCCESS_CREATE : EventName.EDITOR_SUCCESS_EDIT,
               editorMwmParams().add(EventParam.IS_AUTHENTICATED, valueOfIsAuthorized)
                                .add(EventParam.IS_ONLINE, String.valueOf(ConnectionState.INSTANCE.isConnected())));
  }

  public void trackEditorError(boolean newObject, String valueOfIsAuthorized)
  {
    trackEvent(newObject ? EventName.EDITOR_ERROR_CREATE : EventName.EDITOR_ERROR_EDIT,
               editorMwmParams().add(EventParam.IS_AUTHENTICATED, valueOfIsAuthorized)
                                .add(EventParam.IS_ONLINE, String.valueOf(ConnectionState.INSTANCE.isConnected())));
  }

  public void trackNetworkUsageAlert(@NonNull String event, @NonNull String param)
  {
    trackEvent(event, Statistics.params().add(VALUE, param));
  }

  public void trackAuthRequest(OsmOAuth.AuthType type)
  {
    trackEvent(EventName.EDITOR_AUTH_REQUEST, Statistics.params()
                                                        .add(Statistics.EventParam.TYPE, type.name));
  }

  public void trackTaxiInRoutePlanning(@Nullable MapObject from, @Nullable MapObject to,
                                       @Nullable Location location, @NonNull String providerName,
                                       boolean isAppInstalled)
  {
    Statistics.ParameterBuilder params = Statistics.params();
    params.add(Statistics.EventParam.PROVIDER, providerName);

    params.add(Statistics.EventParam.FROM_LAT, from != null ? String.valueOf(from.getLat()) : "N/A")
          .add(Statistics.EventParam.FROM_LON, from != null ? String.valueOf(from.getLon()) : "N/A");

    params.add(Statistics.EventParam.TO_LAT, to != null ? String.valueOf(to.getLat()) : "N/A")
          .add(Statistics.EventParam.TO_LON, to != null ? String.valueOf(to.getLon()) : "N/A");

    String event = isAppInstalled ? Statistics.EventName.ROUTING_TAXI_ORDER
                                  : Statistics.EventName.ROUTING_TAXI_INSTALL;
    trackEvent(event, location, params.get());
  }

  public void trackTaxiEvent(@NonNull String eventName, @NonNull String providerName)
  {
    Statistics.ParameterBuilder params = Statistics.params();
    params.add(Statistics.EventParam.PROVIDER, providerName);
    trackEvent(eventName, params);
  }

  public void trackTaxiError(@Nullable TaxiType type, @NonNull TaxiManager.ErrorCode code)
  {
    Statistics.ParameterBuilder params = Statistics.params();
    if (type != null)
      params.add(Statistics.EventParam.PROVIDER, type.getProviderName());
    params.add(ERROR, code.toStatisticValue());
    trackEvent(EventName.ROUTING_TAXI_ROUTE_BUILT, params);
  }

  public void trackRestaurantEvent(@NonNull String eventName, @NonNull Sponsored restaurant,
                                   @NonNull MapObject mapObject)
  {
    String provider = restaurant.getType() == Sponsored.TYPE_OPENTABLE ? OPENTABLE : "Unknown restaurant";
    trackEvent(eventName, LocationHelper.INSTANCE.getLastKnownLocation(),
               Statistics.params().add(PROVIDER, provider)
                         .add(RESTAURANT, restaurant.getId())
                         .add(RESTAURANT_LAT, mapObject.getLat())
                         .add(RESTAURANT_LON, mapObject.getLon()).get());
  }

  public void trackHotelEvent(@NonNull String eventName, @NonNull Sponsored hotel,
                              @NonNull MapObject mapObject)
  {
    String provider = hotel.getType() == Sponsored.TYPE_BOOKING ? BOOKING_COM : "Unknown hotel";
    trackEvent(eventName, LocationHelper.INSTANCE.getLastKnownLocation(),
               Statistics.params().add(PROVIDER, provider)
                         .add(HOTEL, hotel.getId())
                         .add(HOTEL_LAT, mapObject.getLat())
                         .add(HOTEL_LON, mapObject.getLon()).get());
  }

  public void trackBookHotelEvent(@NonNull Sponsored hotel, @NonNull MapObject mapObject)
  {
    trackHotelEvent(PP_SPONSORED_BOOK, hotel, mapObject);
  }

  public void trackBookmarksTabEvent(@NonNull String param)
  {
    ParameterBuilder params = params().add(EventParam.VALUE, param);
    trackEvent(EventName.BM_TAB_CLICK, params);
  }

  public void trackOpenCatalogScreen()
  {
    trackEvent(EventName.BM_DOWNLOADED_CATALOGUE_OPEN, Collections.emptyMap());
  }

  public void trackDownloadCatalogError(@NonNull String value)
  {
    ParameterBuilder params = params().add(EventParam.ERROR, value);
    trackEvent(EventName.BM_DOWNLOADED_CATALOGUE_ERROR, params);
  }

  public void trackPPBanner(@NonNull String eventName, @NonNull MwmNativeAd ad, @BannerState int state)
  {
    trackEvent(eventName, Statistics.params()
                                    .add(BANNER, ad.getBannerId())
                                    .add(PROVIDER, ad.getProvider())
                                    .add(STATE, String.valueOf(state)));
  }

  public void trackPPBannerError(@NonNull String bannerId, @NonNull String provider,
                                 @Nullable NativeAdError error, int state)
  {
    boolean isAdBlank = error != null && error.getCode() == AdError.NO_FILL_ERROR_CODE;
    String eventName = isAdBlank ? PP_BANNER_BLANK : PP_BANNER_ERROR;
    Statistics.ParameterBuilder builder = Statistics.params();
    builder.add(BANNER, !TextUtils.isEmpty(bannerId) ? bannerId : "N/A")
           .add(ERROR_CODE, error != null ? String.valueOf(error.getCode()) : "N/A")
           .add(ERROR_MESSAGE, error != null ? error.getMessage() : "N/A")
           .add(PROVIDER, provider)
           .add(STATE, String.valueOf(state));
    trackEvent(eventName, builder.get());
  }

  public void trackBookingSearchEvent(@NonNull MapObject mapObject)
  {
    trackEvent(PP_SPONSORED_BOOK, LocationHelper.INSTANCE.getLastKnownLocation(),
               Statistics.params()
                         .add(PROVIDER, SEARCH_BOOKING_COM)
                         .add(HOTEL, "")
                         .add(HOTEL_LAT, mapObject.getLat())
                         .add(HOTEL_LON, mapObject.getLon())
                         .get());
  }

  public void trackDownloaderDialogEvent(@NonNull String eventName, long size)
  {
    trackEvent(eventName, Statistics.params()
                                    .add(MAP_DATA_SIZE, size));
  }

  public void trackDownloaderDialogError(long size, @NonNull String error)
  {
    trackEvent(DOWNLOADER_DIALOG_ERROR, Statistics.params()
                                                  .add(MAP_DATA_SIZE, size)
                                                  .add(TYPE, error));
  }

  public void trackPPOwnershipButtonClick(@NonNull MapObject mapObject)
  {
    trackEvent(PP_OWNERSHIP_BUTTON_CLICK, LocationHelper.INSTANCE.getLastKnownLocation(),
               params()
                   .add(MWM_NAME, mapObject.getFeatureId().getMwmName())
                   .add(MWM_VERSION, mapObject.getFeatureId().getMwmVersion())
                   .add(FEATURE_ID, mapObject.getFeatureId().getFeatureIndex())
                   .get());
  }

  public void trackColdStartupInfo(@NonNull Context context)
  {
    BatteryState.State state = BatteryState.getState(context);
    final String charging;
    switch (state.getChargingStatus())
    {
      case CHARGING_STATUS_UNKNOWN:
        charging = "unknown";
        break;
      case CHARGING_STATUS_PLUGGED:
        charging = ParamValue.ON;
        break;
      case CHARGING_STATUS_UNPLUGGED:
        charging = ParamValue.OFF;
        break;
      default:
        charging = "unknown";
        break;
    }

    final String network = getConnectionState();

    trackEvent(APPLICATION_COLD_STARTUP_INFO,
               params()
                   .add(BATTERY, state.getLevel())
                   .add(CHARGING, charging)
                   .add(NETWORK, network)
                   .get());
  }

  @NonNull
  private String getConnectionState()
  {
    final String network;
    if (ConnectionState.INSTANCE.isWifiConnected())
    {
      network = "wifi";
    }
    else if (ConnectionState.INSTANCE.isMobileConnected())
    {
      if (ConnectionState.INSTANCE.isInRoaming())
        network = "roaming";
      else
        network = "mobile";
    }
    else
    {
      network = "off";
    }
    return network;
  }

  public void trackSponsoredOpenEvent(@NonNull Sponsored sponsored)
  {
    Statistics.ParameterBuilder builder = Statistics.params();
    builder.add(NETWORK, getConnectionState())
           .add(PROVIDER, convertToSponsor(sponsored));
    trackEvent(PP_SPONSORED_OPEN, builder.get());
  }

  public void trackGalleryShown(@NonNull GalleryType type, @NonNull GalleryState state,
                                @NonNull GalleryPlacement placement, int itemsCount)
  {
    trackEvent(PP_SPONSORED_SHOWN, Statistics.params()
                                             .add(PROVIDER, type.getProvider())
                                             .add(PLACEMENT, placement.toString())
                                             .add(STATE, state.toString())
                                             .add(COUNT_LOWERCASE, itemsCount));
  }

  public void trackGalleryError(@NonNull GalleryType type,
                                @NonNull GalleryPlacement placement, @Nullable String code)
  {
    trackEvent(PP_SPONSORED_ERROR, Statistics.params()
                                             .add(PROVIDER, type.getProvider())
                                             .add(PLACEMENT, placement.toString())
                                             .add(ERROR, code).get());
  }

  public void trackGalleryProductItemSelected(@NonNull GalleryType type,
                                              @NonNull GalleryPlacement placement, int position,
                                              @NonNull Destination destination)
  {
    trackEvent(PP_SPONSOR_ITEM_SELECTED, Statistics.params()
                                                   .add(PROVIDER, type.getProvider())
                                                   .add(PLACEMENT, placement.toString())
                                                   .add(ITEM, position)
                                                   .add(DESTINATION, destination.toString()));
  }

  public void trackGalleryEvent(@NonNull String eventName, @NonNull GalleryType type,
                                @NonNull GalleryPlacement placement)
  {
    trackEvent(eventName, Statistics.params()
                                    .add(PROVIDER, type.getProvider())
                                    .add(PLACEMENT, placement.toString())
                                    .get());
  }

  public void trackGalleryUserItemShown(@NonNull GalleryType type, @NonNull GalleryState state,
                                        @NonNull GalleryPlacement placement, int position,
                                        @NonNull String id)
  {
    trackEvent(PP_SPONSORED_USER_ITEM, Statistics.params()
                                                 .add(PROVIDER, type.getProvider())
                                                 .add(PLACEMENT, placement.toString())
                                                 .add(STATE, state.toString())
                                                 .add(ITEM, position)
                                                 .add(ID, id), STATISTICS_CHANNEL_REALTIME);
  }

  public void trackSearchPromoCategory(@NonNull String eventName, @NonNull String provider)
  {
    trackEvent(eventName, Statistics.params().add(PROVIDER, provider).get());
  }

  public void trackSettingsToggle(boolean value)
  {
    trackEvent(EventName.SETTINGS_TRACKING_TOGGLE, Statistics.params()
                                                             .add(TYPE, ParamValue.CRASH_REPORTS)
                                                             .add(VALUE, value
                                                                         ? ParamValue.ON
                                                                         : ParamValue.OFF).get());
  }

  public void trackSettingsDetails()
  {
    trackEvent(EventName.SETTINGS_TRACKING_DETAILS,
               Statistics.params().add(TYPE, ParamValue.PERSONAL_ADS).get());
  }

  @NonNull
  private static String convertToSponsor(@NonNull Sponsored sponsored)
  {
    if (sponsored.getType() == Sponsored.TYPE_PARTNER)
      return sponsored.getPartnerName();

    return convertToSponsor(sponsored.getType());
  }

  @NonNull
  private static String convertToSponsor(@Sponsored.SponsoredType int type)
  {
    switch (type)
    {
      case Sponsored.TYPE_BOOKING:
        return BOOKING_COM;
      case Sponsored.TYPE_OPENTABLE:
        return OPENTABLE;
      case Sponsored.TYPE_HOLIDAY:
        return HOLIDAY;
      case Sponsored.TYPE_PARTNER:
        return PARTNER;
      case Sponsored.TYPE_PROMO_CATALOG_CITY:
      case Sponsored.TYPE_PROMO_CATALOG_SIGHTSEEINGS:
      case Sponsored.TYPE_PROMO_CATALOG_OUTDOOR:
        return MAPSME_GUIDES;
      case Sponsored.TYPE_NONE:
        return "N/A";
      default:
        throw new AssertionError("Unknown sponsor type: " + type);
    }
  }

  public void trackRoutingPoint(@NonNull String eventName, @RoutePointInfo.RouteMarkType int type,
                                boolean isPlanning, boolean isNavigating, boolean isMyPosition,
                                boolean isApi)
  {
    final String mode;
    if (isNavigating)
      mode = "onroute";
    else if (isPlanning)
      mode = "planning";
    else
      mode = null;

    final String method;
    if (isPlanning)
      method = "planning_pp";
    else if (isApi)
      method = "api";
    else
      method = "outside_pp";

    ParameterBuilder builder = params()
        .add(TYPE, convertRoutePointType(type))
        .add(VALUE, isMyPosition ? "gps" : "point")
        .add(METHOD, method);
    if (mode != null)
      builder.add(MODE, mode);
    trackEvent(eventName, builder.get());
  }

  public void trackRoutingEvent(@NonNull String eventName, boolean isPlanning)
  {
    trackEvent(eventName,
               params()
                   .add(MODE, isPlanning ? "planning" : "onroute")
                   .get());
  }

  public void trackRoutingStart(@Framework.RouterType int type,
                                boolean trafficEnabled)
  {
    trackEvent(ROUTING_ROUTE_START, prepareRouteParams(type, trafficEnabled));
  }

  public void trackRoutingFinish(boolean interrupted, @Framework.RouterType int type,
                                 boolean trafficEnabled)
  {
    ParameterBuilder params = prepareRouteParams(type, trafficEnabled);
    trackEvent(ROUTING_ROUTE_FINISH, params.add(INTERRUPTED, interrupted ? 1 : 0));
  }

  @NonNull
  private static ParameterBuilder prepareRouteParams(@Framework.RouterType int type,
                                                     boolean trafficEnabled)
  {
    return params().add(MODE, toRouterType(type)).add(TRAFFIC, trafficEnabled ? 1 : 0);
  }

  @NonNull
  private static String toRouterType(@Framework.RouterType int type)
  {
    switch (type)
    {
      case Framework.ROUTER_TYPE_VEHICLE:
        return VEHICLE;
      case Framework.ROUTER_TYPE_PEDESTRIAN:
        return PEDESTRIAN;
      case Framework.ROUTER_TYPE_BICYCLE:
        return BICYCLE;
      case Framework.ROUTER_TYPE_TAXI:
        return TAXI;
      case Framework.ROUTER_TYPE_TRANSIT:
        return TRANSIT;
      default:
        throw new AssertionError("Unsupported router type: " + type);
    }
  }

  public void trackRoutingTooltipEvent(@RoutePointInfo.RouteMarkType int type,
                                       boolean isPlanning)
  {
    trackEvent(ROUTING_PLAN_TOOLTIP_CLICK,
               params()
                   .add(TYPE, convertRoutePointType(type))
                   .add(MODE, isPlanning ? "planning" : "onroute")
                   .get());
  }

  public void trackSponsoredObjectEvent(@NonNull String eventName, @NonNull Sponsored sponsoredObj,
                                        @NonNull MapObject mapObject)
  {
    // Here we code category by means of rating.
    trackEvent(eventName, LocationHelper.INSTANCE.getLastKnownLocation(),
               Statistics.params().add(PROVIDER, convertToSponsor(sponsoredObj))
                         .add(CATEGORY, sponsoredObj.getRating())
                         .add(OBJECT_LAT, mapObject.getLat())
                         .add(OBJECT_LON, mapObject.getLon()).get());
  }

  @NonNull
  private static String convertRoutePointType(@RoutePointInfo.RouteMarkType int type)
  {
    switch (type)
    {
      case RoutePointInfo.ROUTE_MARK_FINISH:
        return "finish";
      case RoutePointInfo.ROUTE_MARK_INTERMEDIATE:
        return "inter";
      case RoutePointInfo.ROUTE_MARK_START:
        return "start";
      default:
        throw new AssertionError("Wrong parameter 'type'");
    }
  }

  @NonNull
  public void trackUGCStart(boolean isEdit, boolean isPPPreview, boolean isFromNotification)
  {
    trackEvent(UGC_REVIEW_START,
               params()
                   .add(EventParam.IS_AUTHENTICATED, Framework.nativeIsUserAuthenticated())
                   .add(EventParam.IS_ONLINE, ConnectionState.INSTANCE.isConnected())
                   .add(EventParam.MODE, isEdit ? ParamValue.EDIT : ParamValue.ADD)
                   .add(EventParam.FROM, isPPPreview ? ParamValue.PLACEPAGE_PREVIEW :
                                         isFromNotification ? ParamValue.NOTIFICATION : ParamValue.PLACEPAGE)
                   .get());
  }

  public void trackAuthDialogAction(@NonNull String action, @NonNull String value)
  {
    trackEvent(action, params().add(EventParam.FROM, value).get());
  }

  public void trackUGCExternalAuthSucceed(@NonNull String provider)
  {
    trackEvent(UGC_AUTH_EXTERNAL_REQUEST_SUCCESS, params().add(EventParam.PROVIDER, provider));
  }

  public void trackUGCAuthFailed(@Framework.AuthTokenType int type, @Nullable String error)
  {
    trackEvent(UGC_AUTH_ERROR, params()
        .add(EventParam.PROVIDER, getAuthProvider(type))
        .add(EventParam.ERROR, error)
        .get());
  }

  public void trackAuthError(@Framework.AuthTokenType int type, @Nullable String error)
  {
    ParameterBuilder params = params()
        .add(PROVIDER, getAuthProvider(type))
        .add(ERROR, error);
    trackEvent(AUTH_ERROR, params);
  }

  public void trackAuthExternalRequestSuccess(int type)
  {
    Map<String, String> params = Collections.singletonMap(Statistics.EventParam.PROVIDER,
                                                          Statistics.getAuthProvider(type));
    trackEvent(AUTH_EXTERNAL_REQUEST_SUCCESS, params);
  }

  public void trackAuthDeclined(int type)
  {
    ParameterBuilder params = params().add(PROVIDER, getAuthProvider(type));
    trackEvent(AUTH_DECLINED, params);
  }

  public void trackAuthRequestSuccess(int tokenType)
  {
    Map<String, String> params = Collections.singletonMap(PROVIDER,
                                                          getAuthProvider(tokenType));
    trackEvent(EventName.AUTH_REQUEST_SUCCESS, params);
  }

  public void trackOsmAuthRequestStats(@NonNull String event)
  {
    trackEvent(event, Collections.singletonMap(PROVIDER,
                                               ParamValue.OSM.toLowerCase()));
  }

  @NonNull
  public static String getAuthProvider(@Framework.AuthTokenType int type)
  {
    switch (type)
    {
      case Framework.SOCIAL_TOKEN_FACEBOOK:
        return FACEBOOK;
      case Framework.SOCIAL_TOKEN_GOOGLE:
        return GOOGLE;
      case Framework.SOCIAL_TOKEN_PHONE:
        return PHONE;
      case Framework.TOKEN_MAPSME:
        return MAPSME;
      case Framework.SOCIAL_TOKEN_INVALID:
        return UNKNOWN;
      default:
        throw new AssertionError("Unknown social token type: " + type);
    }
  }

  @NonNull
  public static String getSynchronizationType(@BookmarkManager.SynchronizationType int type)
  {
    return type == 0 ? BACKUP : RESTORE;
  }

  public void trackQuickFilterOpen(@NonNull String category)
  {
    trackEvent(SEARCH_QUICKFILTER_OPEN, params().add(CATEGORY, category)
                                                .add(NETWORK, getConnectionState()).get());
  }

  public void trackQuickFilterClick(@NonNull String category, @NonNull String name, int count)
  {
    trackEvent(SEARCH_QUICKFILTER_CLICK, params().add(CATEGORY, category).add(name, count).get());
  }

  public void trackQuickFilterApply(@NonNull String category, @Nullable String appliedFilters)
  {
    ParameterBuilder builder = params();
    builder.add(CATEGORY, category);
    if (!TextUtils.isEmpty(appliedFilters))
      builder.add(FILTERS, appliedFilters);
    trackEvent(SEARCH_QUICKFILTER_APPLY, builder.get());
  }

  public void trackQuickFilterClickError(@NonNull String category, @NonNull String filter,
                                         @NonNull String error)
  {
    trackEvent(EventName.SEARCH_QUICKFILTER_CLICK_ERROR, params().add(CATEGORY, category)
                                                                 .add(FILTER, filter)
                                                                 .add(ERROR, error));
  }

  public void trackSearchContextAreaClick(@NonNull String value)
  {
    trackEvent(EventName.SEARCH_CONTEXTAREA_CLICK, params().add(VALUE, value));
  }

  public void trackFilterEvent(@NonNull String event, @NonNull String category)
  {
    trackEvent(event, params()
        .add(EventParam.CATEGORY, category)
        .get());
  }

  public void trackFilterOpenEvent()
  {
    Statistics.INSTANCE.trackEvent(Statistics.EventName.SEARCH_FILTER_OPEN,
                                   params().add(CATEGORY, HOTEL)
                                           .add(NETWORK, getConnectionState()).get());
  }

  public void trackFilterApplyEvent(@Nullable String appliedFilters)
  {
    ParameterBuilder builder = params();
    if (!TextUtils.isEmpty(appliedFilters))
      builder.add(FILTERS, appliedFilters);
    builder.add(CATEGORY, HOTEL);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.SEARCH_FILTER_APPLY, builder.get());
  }

  public void trackFilterClick(@NonNull String category, @NonNull Pair<String, String> params)
  {
    trackEvent(SEARCH_FILTER_CLICK, params()
        .add(EventParam.CATEGORY, category)
        .add(params.first, params.second)
        .get());
  }

  public void trackBmSyncProposalShown(boolean hasAuth)
  {
    trackEvent(BM_SYNC_PROPOSAL_SHOWN, params().add(HAS_AUTH, hasAuth ? 1 : 0).get());
  }

  public void trackBmSyncProposalApproved(boolean hasAuth)
  {
    trackEvent(BM_SYNC_PROPOSAL_APPROVED, params()
        .add(HAS_AUTH, hasAuth ? 1 : 0)
        .add(NETWORK, getConnectionState())
        .get());
  }

  public void trackBmRestoreProposalClick()
  {
    trackEvent(BM_RESTORE_PROPOSAL_CLICK, params()
        .add(NETWORK, getConnectionState())
        .get());
  }

  public void trackBmSyncProposalError(@Framework.AuthTokenType int type, @Nullable String message)
  {
    trackEvent(BM_SYNC_PROPOSAL_ERROR, params()
        .add(PROVIDER, getAuthProvider(type))
        .add(ERROR, message)
        .get());
  }

  public void trackBmSettingsToggle(boolean checked)
  {
    trackEvent(BM_SYNC_PROPOSAL_TOGGLE, params()
        .add(STATE, checked ? 1 : 0)
        .get());
  }

  public void trackBmSynchronizationFinish(@BookmarkManager.SynchronizationType int type,
                                           @BookmarkManager.SynchronizationResult int result,
                                           @NonNull String errorString)
  {
    if (result == BookmarkManager.CLOUD_SUCCESS)
    {
      if (type == BookmarkManager.CLOUD_BACKUP)
        trackEvent(BM_SYNC_SUCCESS);
      else
        trackEvent(BM_RESTORE_PROPOSAL_SUCCESS);
      return;
    }

    trackEvent(type == BookmarkManager.CLOUD_BACKUP ? BM_SYNC_ERROR : BM_RESTORE_PROPOSAL_ERROR,
               params().add(TYPE, getTypeForErrorSyncResult(result)).add(ERROR, errorString));
  }

  public void trackBmRestoringRequestResult(@BookmarkManager.RestoringRequestResult int result)
  {
    if (result == BookmarkManager.CLOUD_BACKUP_EXISTS)
      return;

    trackEvent(BM_RESTORE_PROPOSAL_ERROR, params()
        .add(TYPE, getTypeForRequestRestoringError(result)));
  }

  @NonNull
  private static String getTypeForErrorSyncResult(@BookmarkManager.SynchronizationResult int result)
  {
    switch (result)
    {
      case BookmarkManager.CLOUD_AUTH_ERROR:
        return ParamValue.AUTH;
      case BookmarkManager.CLOUD_NETWORK_ERROR:
        return ParamValue.NETWORK;
      case BookmarkManager.CLOUD_DISK_ERROR:
        return ParamValue.DISK;
      case BookmarkManager.CLOUD_USER_INTERRUPTED:
        return ParamValue.USER_INTERRUPTED;
      case BookmarkManager.CLOUD_INVALID_CALL:
        return ParamValue.INVALID_CALL;
      case BookmarkManager.CLOUD_SUCCESS:
        throw new AssertionError("It's not a error result!");
      default:
        throw new AssertionError("Unsupported error type: " + result);
    }
  }

  @NonNull
  private static String getTypeForRequestRestoringError(@BookmarkManager.RestoringRequestResult int result)
  {
    switch (result)
    {
      case BookmarkManager.CLOUD_BACKUP_EXISTS:
        throw new AssertionError("It's not a error result!");
      case BookmarkManager.CLOUD_NOT_ENOUGH_DISK_SPACE:
        return DISK_NO_SPACE;
      case BookmarkManager.CLOUD_NO_BACKUP:
        return NO_BACKUP;
      default:
        throw new AssertionError("Unsupported restoring request result: " + result);
    }
  }

  public void trackToolbarClick(@NonNull MainMenu.Item button)
  {
    trackEvent(TOOLBAR_CLICK, getToolbarParams(button));
  }

  public void trackToolbarMenu(@NonNull MainMenu.Item button)
  {
    trackEvent(TOOLBAR_MENU_CLICK, getToolbarParams(button));
  }

  public void trackDownloadBookmarkDialog(@NonNull String button)
  {
    trackEvent(BM_GUIDES_DOWNLOADDIALOGUE_CLICK, params().add(ACTION, button));
  }

  @NonNull
  private static ParameterBuilder getToolbarParams(@NonNull MainMenu.Item button)
  {
    return params().add(BUTTON, button.toStatisticValue());
  }

  public void trackPPBannerClose(@BannerState int state, boolean isCross)
  {
    trackEvent(PP_BANNER_CLOSE, params().add(BANNER, state)
                                        .add(BUTTON, isCross ? 0 : 1));
  }

  public void trackPurchasePreviewShow(@NonNull String purchaseId, @NonNull String vendor,
                                       @NonNull String productId, @Nullable String from,
                                       @Nullable Boolean isTrial)
  {
    ParameterBuilder params = params().add(VENDOR, vendor)
                                      .add(PRODUCT, productId)
                                      .add(PURCHASE, purchaseId);
    if (!TextUtils.isEmpty(from))
      params.add(FROM, from);

    if (isTrial != null)
      params.add(TRIAL, isTrial);

    trackEvent(INAPP_PURCHASE_PREVIEW_SHOW, params,
               STATISTICS_CHANNEL_REALTIME);
  }

  public void trackPurchasePreviewShow(@NonNull String purchaseId, @NonNull String vendor,
                                       @NonNull String productId)
  {
    trackPurchasePreviewShow(purchaseId, vendor, productId, null, null);
  }

  public void trackPurchaseEvent(@NonNull String event, @NonNull String purchaseId)
  {
    trackPurchaseEvent(event, purchaseId, STATISTICS_CHANNEL_DEFAULT);
  }

  public void trackPurchaseEvent(@NonNull String event, @NonNull String purchaseId,
                                 @StatisticsChannel int channel)
  {
    trackEvent(event, params().add(PURCHASE, purchaseId), channel);
  }

  public void trackPurchasePreviewSelect(@NonNull String purchaseId, @NonNull String productId)
  {
    trackPurchasePreviewSelect(purchaseId, productId, null);
  }

  public void trackPurchasePreviewSelect(@NonNull String purchaseId, @NonNull String productId,
                                         @Nullable Boolean isTrial)
  {
    ParameterBuilder params = params();
    if (isTrial != null)
      params.add(TRIAL, isTrial);
    trackEvent(INAPP_PURCHASE_PREVIEW_SELECT, params.add(PRODUCT, productId)
                                                    .add(PURCHASE, purchaseId));
  }

  public void trackPurchaseStoreError(@NonNull String purchaseId,
                                      @BillingClient.BillingResponse int error)
  {
    trackEvent(INAPP_PURCHASE_STORE_ERROR, params().add(ERROR, "Billing error: " + error)
                                                   .add(PURCHASE, purchaseId));
  }

  public void trackPurchaseValidationError(@NonNull String purchaseId, @NonNull ValidationStatus status)
  {
    if (status == ValidationStatus.VERIFIED)
      return;

    int errorCode;
    switch (status)
    {
      case NOT_VERIFIED:
        errorCode = 0;
        break;
      case AUTH_ERROR:
        errorCode = 1;
        break;
      case SERVER_ERROR:
        errorCode = 2;
        break;
      default:
        throw new UnsupportedOperationException("Unsupported status: " + status);
    }

    trackEvent(INAPP_PURCHASE_VALIDATION_ERROR, params().add(ERROR_CODE, errorCode)
                                                        .add(PURCHASE, purchaseId));
  }

  public void trackPowerManagmentSchemeChanged(@PowerManagment.SchemeType int scheme)
  {
    String statisticValue = "";
    switch (scheme)
    {
      case PowerManagment.NONE:
      case PowerManagment.MEDIUM:
        throw new AssertionError("Incorrect scheme type");
      case PowerManagment.NORMAL:
        statisticValue = "never";
        break;
      case PowerManagment.AUTO:
        statisticValue = "auto";
        break;
      case PowerManagment.HIGH:
        statisticValue = "max";
        break;
    }

    trackEvent(EventName.Settings.ENERGY_SAVING, params().add(EventParam.VALUE, statisticValue));
  }

  public void trackPurchaseProductDelivered(@NonNull String purchaseId, @NonNull String vendor)
  {
    trackPurchaseProductDelivered(purchaseId, vendor, null);
  }

  public void trackPurchaseProductDelivered(@NonNull String purchaseId, @NonNull String vendor,
                                            @Nullable Boolean isTrial)
  {
    ParameterBuilder params = params();
    if (isTrial != null)
      params.add(TRIAL, isTrial);
    trackEvent(INAPP_PURCHASE_PRODUCT_DELIVERED, params.add(VENDOR, vendor)
                                                       .add(PURCHASE, purchaseId),
               STATISTICS_CHANNEL_REALTIME);
  }

  public void trackTipsEvent(@NonNull String eventName, int type)
  {
    trackEvent(eventName, params().add(TYPE, type));
  }

  public void trackTipsClose(int type)
  {
    trackEvent(TIPS_TRICKS_CLOSE, params().add(TYPE, type).add(OPTION, OFFSCREEEN));
  }

  public void trackGuidesShown(@NonNull String serverIds)
  {
    if (!serverIds.isEmpty())
      trackEvent(GUIDES_SHOWN, params().add(SERVER_IDS, serverIds), STATISTICS_CHANNEL_REALTIME);
  }

  public void trackGuideOpen(@NonNull String serverId)
  {
    trackEvent(GUIDES_OPEN, params().add(SERVER_ID, serverId), STATISTICS_CHANNEL_REALTIME);
  }

  public void trackGuideBookmarkSelect(@NonNull String serverId, long catId)
  {
    trackEvent(GUIDES_BOOKMARK_SELECT, params()
                   .add(SERVER_ID, serverId)
                   .add(FROM, getCompilationTypeById(catId)), STATISTICS_CHANNEL_REALTIME);
  }

  @NonNull
  private String getCompilationTypeById(long catId)
  {
    if (!BookmarkManager.INSTANCE.isCompilation(catId))
      return Statistics.ParamValue.MAIN;
    else if (BookmarkManager.INSTANCE.getCompilationType(catId) == BookmarkManager.CATEGORY)
      return Statistics.ParamValue.CATEGORY;
    else
      return Statistics.ParamValue.COLLECTION;
  }

  public void trackCollectionOrCategorySelect(@NonNull String serverId, @NonNull String categoryName,
                                              boolean isCategory)
  {
    String logEventName = isCategory ? BM_BOOKMARKS_LIST_CATEGORY_SELECT :
                          BM_BOOKMARKS_LIST_COLLECTION_SELECT;

    trackEvent(logEventName, params().add(SERVER_ID, serverId).add(NAME, categoryName),
               STATISTICS_CHANNEL_REALTIME);
  }

  public void trackGuideVisibilityChange(@NonNull String action, @NonNull String serverId,
                                         @NonNull String type)
  {
    trackEvent(BM_GUIDE_VISIBILITY_CHANGE, params().add(ACTION, action).add(SERVER_ID, serverId)
                                                   .add(TYPE, type), STATISTICS_CHANNEL_REALTIME);
  }

  public void trackGuideTrackSelect(@NonNull String serverId)
  {
    trackEvent(GUIDES_TRACK_SELECT, params().add(SERVER_ID, serverId), STATISTICS_CHANNEL_REALTIME);
  }

  public void trackDeeplinkEvent(@NonNull String event, @NonNull ParameterBuilder params,
                                 boolean isFirstLaunch)
  {
    trackEvent(event, params.add(FIRST_LAUNCH, isFirstLaunch ? TRUE : FALSE));
  }

  public void trackElevationProfilePageOpen(@Nullable String serverId, @NonNull String state)
  {
    trackEvent(ELEVATION_PROFILE_PAGE_OPEN, params().add(SERVER_ID, serverId).add(STATE, state));
  }

  public void trackElevationProfilePageClose(@Nullable String serverId)
  {
    trackEvent(ELEVATION_PROFILE_PAGE_CLOSE, params().add(SERVER_ID, serverId));
  }

  @NonNull
  public static ParameterBuilder makeParametersFromType(@NonNull String type)
  {
    return params().add(TYPE, type);
  }

  @NonNull
  public static ParameterBuilder makeParametersFromTypeAndUrl(@NonNull String type, @NonNull String url)
  {
    Statistics.ParameterBuilder result = params();
    try
    {
      Uri uri = Uri.parse(url);
      for (String name : uri.getQueryParameterNames())
      {
        if (name.startsWith("utm_") || name.equals("affiliate_id"))
        {
          String value = uri.getQueryParameter(name);
          result.add(name, value == null ? "" : value);
        }
      }
    }
    catch (Exception e)
    {
      CrashlyticsUtils.INSTANCE.logException(new RuntimeException("Failed to parse url: " + url, e));
    }

    result.add(TYPE, type);
    return result;
  }

  public static ParameterBuilder params()
  {
    return new ParameterBuilder();
  }

  public static ParameterBuilder editorMwmParams()
  {
    return params().add(MWM_NAME, Editor.nativeGetMwmName())
                   .add(MWM_VERSION, Editor.nativeGetMwmVersion());
  }

  public static class ParameterBuilder
  {
    private final Map<String, String> mParams = new HashMap<>();

    @NonNull
    public static ParameterBuilder from(@NonNull String key, @NonNull Analytics analytics)
    {
      return new ParameterBuilder().add(key, analytics.getName());
    }

    public ParameterBuilder add(String key, String value)
    {
      mParams.put(key, value);
      return this;
    }

    public ParameterBuilder add(String key, boolean value)
    {
      mParams.put(key, String.valueOf(value));
      return this;
    }

    public ParameterBuilder add(String key, int value)
    {
      mParams.put(key, String.valueOf(value));
      return this;
    }

    public ParameterBuilder add(String key, float value)
    {
      mParams.put(key, String.valueOf(value));
      return this;
    }

    public ParameterBuilder add(String key, double value)
    {
      mParams.put(key, String.valueOf(value));
      return this;
    }

    public Map<String, String> get()
    {
      return mParams;
    }
  }

  public static String getPointType(MapObject point)
  {
    return MapObject.isOfType(MapObject.MY_POSITION, point) ? Statistics.EventParam.MY_POSITION
                                                            : Statistics.EventParam.POINT;
  }

  public enum NetworkErrorType
  {
    NO_NETWORK,
    AUTH_FAILED
  }
}
