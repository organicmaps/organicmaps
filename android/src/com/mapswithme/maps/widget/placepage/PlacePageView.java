package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.ColorInt;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.bookmarks.ChooseBookmarkCategoryFragment;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.editor.OpeningHours;
import com.mapswithme.maps.editor.data.TimeFormatUtils;
import com.mapswithme.maps.editor.data.Timetable;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.widget.ArrowView;
import com.mapswithme.maps.widget.BaseShadowController;
import com.mapswithme.maps.widget.ObservableScrollView;
import com.mapswithme.maps.widget.ScrollViewShadowController;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;


public class PlacePageView extends RelativeLayout implements View.OnClickListener, View.OnLongClickListener
{
  private static final String PREF_USE_DMS = "use_dms";

  private boolean mIsDocked;
  private boolean mIsFloating;

  // Preview.
  private ViewGroup mPreview;
  private Toolbar mToolbar;
  private TextView mTvTitle;
  private TextView mTvSubtitle;
  private ArrowView mAvDirection;
  private TextView mTvDistance;
  private TextView mTvAddress;
  // Details.
  private ScrollView mDetails;
  private View mPhone;
  private TextView mTvPhone;
  private View mWebsite;
  private TextView mTvWebsite;
  private TextView mTvLatlon;
  private View mOpeningHours;
  private TextView mFullOpeningHours;
  private TextView mTodayOpeningHours;
  private View mWifi;
  private View mEmail;
  private TextView mTvEmail;
  private View mOperator;
  private TextView mTvOperator;
  private View mCuisine;
  private TextView mTvCuisine;
  private View mWiki;
  private View mEntrance;
  private TextView mTvEntrance;
  private View mEditPlace;
  private View mAddOrganisation;
  private View mAddPlace;
  // Bookmark
  private ImageView mIvColor;
  private EditText mEtBookmarkName;
  private WebView mWvDescription;
  private TextView mTvDescription;
  private Button mBtnEditHtmlDescription;
  private TextView mTvBookmarkGroup;
  // Place page buttons
  private View mGeneralButtonsFrame;
  private View mRouteButtonsFrame;
  private View mApiBack;
  private ImageView mIvBookmark;
  private View mRoutingButton;
  // Animations
  private BaseShadowController mShadowController;
  private BasePlacePageAnimationController mAnimationController;
  private MwmActivity.LeftAnimationTrackListener mLeftAnimationTrackListener;
  // Data
  private MapObject mMapObject;
  private boolean mIsLatLonDms;

  public enum State
  {
    HIDDEN,
    PREVIEW,
    BOOKMARK,
    DETAILS
  }

  public PlacePageView(Context context)
  {
    this(context, null, 0);
  }

  public PlacePageView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public PlacePageView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs);

    mIsLatLonDms = MwmApplication.prefs().getBoolean(PREF_USE_DMS, false);

    init(attrs, defStyleAttr);
  }

  private void initViews()
  {
    LayoutInflater.from(getContext()).inflate(R.layout.place_page, this);

    mPreview = (ViewGroup) findViewById(R.id.pp__preview);
    mTvTitle = (TextView) mPreview.findViewById(R.id.tv__title);
    mToolbar = (Toolbar) findViewById(R.id.toolbar);
    mTvSubtitle = (TextView) mPreview.findViewById(R.id.tv__subtitle);
    mTvDistance = (TextView) mPreview.findViewById(R.id.tv__straight_distance);
    mAvDirection = (ArrowView) mPreview.findViewById(R.id.av__direction);
    mAvDirection.setOnClickListener(this);
    mTvAddress = (TextView) mPreview.findViewById(R.id.tv__address);

    mDetails = (ScrollView) findViewById(R.id.pp__details);
    RelativeLayout address = (RelativeLayout) mDetails.findViewById(R.id.ll__place_name);
    mPhone = mDetails.findViewById(R.id.ll__place_phone);
    mPhone.setOnClickListener(this);
    mTvPhone = (TextView) mDetails.findViewById(R.id.tv__place_phone);
    mWebsite = mDetails.findViewById(R.id.ll__place_website);
    mWebsite.setOnClickListener(this);
    mTvWebsite = (TextView) mDetails.findViewById(R.id.tv__place_website);
    LinearLayout latlon = (LinearLayout) mDetails.findViewById(R.id.ll__place_latlon);
    latlon.setOnClickListener(this);
    mTvLatlon = (TextView) mDetails.findViewById(R.id.tv__place_latlon);
    mOpeningHours = mDetails.findViewById(R.id.ll__place_schedule);
    mFullOpeningHours = (TextView) mDetails.findViewById(R.id.opening_hours);
    mTodayOpeningHours = (TextView) mDetails.findViewById(R.id.today_opening_hours);
    mWifi = mDetails.findViewById(R.id.ll__place_wifi);
    mIvColor = (ImageView) mDetails.findViewById(R.id.iv__bookmark_color);
    mIvColor.setOnClickListener(this);
    mEmail = mDetails.findViewById(R.id.ll__place_email);
    mEmail.setOnClickListener(this);
    mTvEmail = (TextView) mEmail.findViewById(R.id.tv__place_email);
    mOperator = mDetails.findViewById(R.id.ll__place_operator);
    mOperator.setOnClickListener(this);
    mTvOperator = (TextView) mOperator.findViewById(R.id.tv__place_operator);
    mCuisine = mDetails.findViewById(R.id.ll__place_cuisine);
    mTvCuisine = (TextView) mCuisine.findViewById(R.id.tv__place_cuisine);
    mWiki = mDetails.findViewById(R.id.ll__place_wiki);
    mWiki.setOnClickListener(this);
    mEntrance = mDetails.findViewById(R.id.ll__place_entrance);
    mTvEntrance = (TextView) mEntrance.findViewById(R.id.tv__place_entrance);
    mEditPlace = mDetails.findViewById(R.id.ll__place_editor);
    mEditPlace.setOnClickListener(this);
    mAddOrganisation = mDetails.findViewById(R.id.ll__add_organisation);
    mAddOrganisation.setOnClickListener(this);
    mAddPlace = mDetails.findViewById(R.id.ll__place_add);
    mAddPlace.setOnClickListener(this);
    latlon.setOnLongClickListener(this);
    address.setOnLongClickListener(this);
    mPhone.setOnLongClickListener(this);
    mWebsite.setOnLongClickListener(this);
    mOpeningHours.setOnLongClickListener(this);
    mEmail.setOnLongClickListener(this);
    mOperator.setOnLongClickListener(this);
    mWiki.setOnLongClickListener(this);

    mEtBookmarkName = (EditText) mDetails.findViewById(R.id.et__bookmark_name);
    mEtBookmarkName.setOnEditorActionListener(new TextView.OnEditorActionListener()
    {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
      {
        if (actionId == EditorInfo.IME_ACTION_DONE)
        {
          saveBookmarkTitle();
          refreshPreview();
        }

        return false;
      }
    });

    TextView tvNotes = (TextView) mDetails.findViewById(R.id.tv__bookmark_notes);
    tvNotes.setOnClickListener(this);

    mTvBookmarkGroup = (TextView) mDetails.findViewById(R.id.tv__bookmark_group);
    mTvBookmarkGroup.setOnClickListener(this);
    mWvDescription = (WebView) mDetails.findViewById(R.id.wv__description);
    mTvDescription = (TextView) mDetails.findViewById(R.id.tv__description);
    mTvDescription.setOnClickListener(this);
    mBtnEditHtmlDescription = (Button) mDetails.findViewById(R.id.btn__edit_html_bookmark);
    mBtnEditHtmlDescription.setOnClickListener(this);

    ViewGroup ppButtons = (ViewGroup) findViewById(R.id.pp__buttons);

    mGeneralButtonsFrame = ppButtons.findViewById(R.id.general);
    mApiBack = mGeneralButtonsFrame.findViewById(R.id.ll__api_back);
    mApiBack.setOnClickListener(this);
    final View bookmarkGroup = mGeneralButtonsFrame.findViewById(R.id.ll__bookmark);
    bookmarkGroup.setOnClickListener(this);
    mIvBookmark = (ImageView) bookmarkGroup.findViewById(R.id.iv__bookmark);
    mGeneralButtonsFrame.findViewById(R.id.ll__share).setOnClickListener(this);
    mRoutingButton = mGeneralButtonsFrame.findViewById(R.id.ll__route);

    mRouteButtonsFrame = ppButtons.findViewById(R.id.routing);
    mRouteButtonsFrame.findViewById(R.id.from).setOnClickListener(this);
    mRouteButtonsFrame.findViewById(R.id.to).setOnClickListener(this);

    mShadowController = new ScrollViewShadowController((ObservableScrollView) mDetails)
                            .addBottomShadow()
                            .attach();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      setElevation(UiUtils.dimen(R.dimen.placepage_elevation));

    if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE)
      mDetails.setBackgroundResource(0);
  }

  private void init(AttributeSet attrs, int defStyleAttr)
  {
    initViews();

    if (isInEditMode())
      return;

    final TypedArray attrArray = getContext().obtainStyledAttributes(attrs, R.styleable.PlacePageView, defStyleAttr, 0);
    final int animationType = attrArray.getInt(R.styleable.PlacePageView_animationType, 0);
    mIsDocked = attrArray.getBoolean(R.styleable.PlacePageView_docked, false);
    mIsFloating = attrArray.getBoolean(R.styleable.PlacePageView_floating, false);
    attrArray.recycle();

    // switch with values from "animationType" from attrs.xml
    switch (animationType)
    {
    case 0:
      mAnimationController = new PlacePageBottomAnimationController(this);
      break;

    case 1:
      mAnimationController = new PlacePageLeftAnimationController(this);
      break;
    }

    mAnimationController.initialHide();
  }

  public void restore()
  {
    if (mMapObject != null)
      subscribeBookmarkEditFragment(null);
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    return mAnimationController.onTouchEvent(event);
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent event)
  {
    return mAnimationController.onInterceptTouchEvent(event);
  }

  public boolean isDocked()
  {
    return mIsDocked;
  }

  public boolean isFloating()
  {
    return mIsFloating;
  }

  public State getState()
  {
    return mAnimationController.getState();
  }

  public void setState(State state)
  {
    InputUtils.hideKeyboard(mEtBookmarkName);

    mDetails.scrollTo(0, 0);

    if (mMapObject != null)
      mAnimationController.setState(state, mMapObject.getMapObjectType());

    if (!mIsDocked && !mIsFloating)
    {
      // After ninepatch background is set from code, all paddings are lost, so we need to restore it later.
      int bottom = mPreview.getPaddingBottom();
      int left = mPreview.getPaddingLeft();
      int right = mPreview.getPaddingRight();
      int top = mPreview.getPaddingTop();
      mPreview.setBackgroundResource(ThemeUtils.getResource(getContext(), state == State.PREVIEW ? R.attr.ppPreviewHeadClosed
                                                                                                 : R.attr.ppPreviewHeadOpen));
      mPreview.setPadding(left, top, right, bottom);
    }
  }

  public MapObject getMapObject()
  {
    return mMapObject;
  }

  /**
   * @param mapObject new MapObject
   * @param force if true, new object'll be set without comparison with the old one
   */
  public void setMapObject(MapObject mapObject, boolean force)
  {
    if (!force && MapObject.same(mMapObject, mapObject))
      return;

    mMapObject = mapObject;
    refreshViews();
  }

  public void refreshViews()
  {
    if (mMapObject == null)
      return;

    refreshPreview();
    refreshDetails();
    final Location loc = LocationHelper.INSTANCE.getSavedLocation();

    switch (mMapObject.getMapObjectType())
    {
    case MapObject.BOOKMARK:
      refreshDistanceToObject(loc);
      showBookmarkDetails();
      refreshButtons(false, true);
      break;
    case MapObject.POI:
    case MapObject.SEARCH:
      refreshDistanceToObject(loc);
      hideBookmarkDetails();
      refreshButtons(false, true);
      break;
    case MapObject.API_POINT:
      refreshDistanceToObject(loc);
      hideBookmarkDetails();
      refreshButtons(true, true);
      break;
    case MapObject.MY_POSITION:
      refreshMyPosition(loc);
      hideBookmarkDetails();
      refreshButtons(false, false);
      break;
    }

    UiThread.runLater(new Runnable()
    {
      @Override
      public void run()
      {
        mShadowController.updateShadows();
        requestLayout();
      }
    });
  }

  private void refreshPreview()
  {
    UiUtils.setTextAndHideIfEmpty(mTvTitle, mMapObject.getTitle());
    if (mToolbar != null)
      mToolbar.setTitle(mMapObject.getTitle());
    UiUtils.setTextAndHideIfEmpty(mTvSubtitle, mMapObject.getSubtitle());
    UiUtils.hide(mAvDirection);
    UiUtils.setTextAndHideIfEmpty(mTvAddress, mMapObject.getAddress());
  }

  private void refreshDetails()
  {
    refreshLatLon();
    final String website = mMapObject.getMetadata(Metadata.MetadataType.FMD_WEBSITE);
    refreshMetadataOrHide(TextUtils.isEmpty(website) ? mMapObject.getMetadata(Metadata.MetadataType.FMD_URL) : website, mWebsite, mTvWebsite);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER), mPhone, mTvPhone);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_EMAIL), mEmail, mTvEmail);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_OPERATOR), mOperator, mTvOperator);
    refreshMetadataOrHide(Framework.nativeGetActiveObjectFormattedCuisine(), mCuisine, mTvCuisine);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA), mWiki, null);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_INTERNET), mWifi, null);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_FLATS), mEntrance, mTvEntrance);
    refreshOpeningHours();

    if (RoutingController.get().isNavigating() || MapManager.nativeIsLegacyMode())
    {
      UiUtils.hide(mEditPlace, mAddOrganisation, mAddPlace);
    }
    else
    {
      UiUtils.showIf(Editor.nativeIsFeatureEditable(), mEditPlace);
      UiUtils.showIf(Framework.nativeIsActiveObjectABuilding(), mAddOrganisation);
      UiUtils.showIf(Framework.nativeCanAddPlaceFromPlacePage(), mAddPlace);
    }
  }

  private void refreshOpeningHours()
  {
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(mMapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS));
    if (timetables == null || timetables.length == 0)
    {
      UiUtils.hide(mOpeningHours);
      return;
    }

    UiUtils.show(mOpeningHours);

    final Resources resources = getResources();
    if (timetables[0].isFullWeek())
    {
      refreshTodayOpeningHours((timetables[0].isFullday ? resources.getString(R.string.twentyfour_seven)
                                                        : resources.getString(R.string.daily) + " " + timetables[0].workingTimespan),
                               ThemeUtils.getColor(getContext(), android.R.attr.textColorPrimary));
      UiUtils.hide(mFullOpeningHours);
      return;
    }

    boolean containsCurrentWeekday = false;
    final int currentDay = Calendar.getInstance().get(Calendar.DAY_OF_WEEK);
    for (Timetable tt : timetables)
    {
      if (tt.containsWeekday(currentDay))
      {
        containsCurrentWeekday = true;
        refreshTodayOpeningHours(resources.getString(R.string.today) + " " + tt.workingTimespan,
                                 ThemeUtils.getColor(getContext(), android.R.attr.textColorPrimary));
        break;
      }
    }

    UiUtils.setTextAndShow(mFullOpeningHours, TimeFormatUtils.formatTimetables(timetables));
    if (!containsCurrentWeekday)
      refreshTodayOpeningHours(resources.getString(R.string.day_off_today), resources.getColor(R.color.base_red));
  }

  private void refreshTodayOpeningHours(String text, @ColorInt int color)
  {
    UiUtils.setTextAndShow(mTodayOpeningHours, text);
    mTodayOpeningHours.setTextColor(color);
  }

  private void hideBookmarkDetails()
  {
    mIvBookmark.setImageDrawable(Graphics.tint(getContext(), R.drawable.ic_bookmarks_off, R.attr.iconTint));
  }

  private void showBookmarkDetails()
  {
    final Bookmark bookmark = (Bookmark) mMapObject;
    mEtBookmarkName.setText(bookmark.getTitle());
    mTvBookmarkGroup.setText(bookmark.getCategoryName());
    mIvColor.setImageResource(bookmark.getIcon().getSelectedResId());
    mIvBookmark.setImageResource(R.drawable.ic_bookmarks_on);
    final String notes = bookmark.getBookmarkDescription();
    if (notes.isEmpty())
      UiUtils.hide(mWvDescription, mBtnEditHtmlDescription, mTvDescription);
    else if (StringUtils.nativeIsHtml(notes))
    {
      mWvDescription.loadData(notes, "text/html; charset=utf-8", null);
      UiUtils.show(mWvDescription, mBtnEditHtmlDescription);
      UiUtils.hide(mTvDescription);
    }
    else
    {
      UiUtils.hide(mWvDescription, mBtnEditHtmlDescription);
      UiUtils.setTextAndShow(mTvDescription, notes);
    }
  }

  private void refreshButtons(boolean showBackButton, boolean showRoutingButton)
  {
    if (RoutingController.get().isPlanning())
    {
      UiUtils.show(mRouteButtonsFrame);
      UiUtils.hide(mGeneralButtonsFrame, mEditPlace);
    }
    else
    {
      UiUtils.show(mGeneralButtonsFrame);
      UiUtils.hide(mRouteButtonsFrame);

      UiUtils.showIf(showBackButton || ParsedMwmRequest.isPickPointMode(), mApiBack);
      UiUtils.showIf(showRoutingButton, mRoutingButton);
    }
  }

  public void refreshLocation(Location l)
  {
    if (mMapObject == null)
      return;

    if (MapObject.isOfType(MapObject.MY_POSITION, mMapObject))
      refreshMyPosition(l);
    else
      refreshDistanceToObject(l);
  }

  private void refreshMyPosition(Location l)
  {
    UiUtils.hide(mTvDistance);

    if (l == null)
      return;

    final StringBuilder builder = new StringBuilder();
    if (l.hasAltitude())
      builder.append(Framework.nativeFormatAltitude(l.getAltitude()));
    if (l.hasSpeed())
      builder.append("   ")
             .append(Framework.nativeFormatSpeed(l.getSpeed()));
    mTvSubtitle.setText(builder.toString());

    mMapObject.setLat(l.getLatitude());
    mMapObject.setLon(l.getLongitude());
    refreshLatLon();
  }

  private void refreshDistanceToObject(Location l)
  {
    if (l != null)
    {
      mTvDistance.setVisibility(View.VISIBLE);
      final DistanceAndAzimut distanceAndAzimuth = Framework.nativeGetDistanceAndAzimuthFromLatLon(
          mMapObject.getLat(), mMapObject.getLon(),
          l.getLatitude(), l.getLongitude(), 0.0);
      mTvDistance.setText(distanceAndAzimuth.getDistance());
    }
    else
      mTvDistance.setVisibility(View.GONE);
  }

  private void refreshLatLon()
  {
    final double lat = mMapObject.getLat();
    final double lon = mMapObject.getLon();
    final String[] latLon = Framework.nativeFormatLatLonToArr(lat, lon, mIsLatLonDms);
    if (latLon.length == 2)
      mTvLatlon.setText(latLon[0] + ", " + latLon[1]);
  }

  private static void refreshMetadataOrHide(String metadata, View metaLayout, TextView metaTv)
  {
    if (!TextUtils.isEmpty(metadata))
    {
      metaLayout.setVisibility(View.VISIBLE);
      if (metaTv != null)
        metaTv.setText(metadata);
    }
    else
      metaLayout.setVisibility(View.GONE);
  }

  public void refreshAzimuth(double northAzimuth)
  {
    if (getState() == State.HIDDEN ||
        mMapObject == null ||
        MapObject.isOfType(MapObject.MY_POSITION, mMapObject))
      return;

    final Location location = LocationHelper.INSTANCE.getSavedLocation();
    if (location == null)
      return;

    final double azimuth = Framework.nativeGetDistanceAndAzimuthFromLatLon(mMapObject.getLat(), mMapObject.getLon(),
                                                                           location.getLatitude(), location.getLongitude(),
                                                                           northAzimuth)
                                    .getAzimuth();
    if (azimuth >= 0)
    {
      UiUtils.show(mAvDirection);
      mAvDirection.setAzimuth(azimuth);
    }
  }

  public void setOnVisibilityChangedListener(BasePlacePageAnimationController.OnVisibilityChangedListener listener)
  {
    mAnimationController.setOnVisibilityChangedListener(listener);
  }

  public void saveBookmarkTitle()
  {
    if (mMapObject == null || !(mMapObject instanceof Bookmark))
      return;

    final Bookmark bookmark = (Bookmark) mMapObject;
    final String title = mEtBookmarkName.getText().toString();
    bookmark.setParams(title, null, bookmark.getBookmarkDescription());
  }

  /**
   * Adds listener to {@link EditDescriptionFragment} to catch notification about bookmark description edit is complete.
   * <br/>When the user rotates device screen the listener is lost, so we must re-subscribe again.
   *
   * @param fragment if specified - explicitly subscribe to this fragment. Otherwise try to find the fragment by hands.
   */
  private void subscribeBookmarkEditFragment(@Nullable EditDescriptionFragment fragment)
  {
    if (fragment == null)
    {
      FragmentManager fm = ((FragmentActivity) getContext()).getSupportFragmentManager();
      fragment = (EditDescriptionFragment) fm.findFragmentByTag(EditDescriptionFragment.class.getName());
    }

    if (fragment == null)
      return;

    fragment.setSaveDescriptionListener(new EditDescriptionFragment.OnDescriptionSavedListener()
    {
      @Override
      public void onSaved(Bookmark bookmark)
      {
        final Bookmark updatedBookmark = BookmarkManager.INSTANCE.getBookmark(bookmark.getCategoryId(), bookmark.getBookmarkId());
        setMapObject(updatedBookmark, true);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.BMK_DESCRIPTION_CHANGED);
      }
    });
  }

  private void showEditor()
  {
    ((MwmActivity) getContext()).showEditor();
  }

  private void addOrganisation()
  {
    Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_ADD_CLICK,
                                   Statistics.params().add(Statistics.EventParam.FROM, "placepage"));
    ((MwmActivity) getContext()).showPositionChooser(true, false);
  }

  private void addPlace()
  {
    // TODO add statistics
    ((MwmActivity) getContext()).showPositionChooser(false, true);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.ll__place_editor:
      showEditor();
      break;
    case R.id.ll__add_organisation:
      addOrganisation();
      break;
    case R.id.ll__place_add:
      addPlace();
      break;
    case R.id.iv__bookmark_color:
      saveBookmarkTitle();
      selectBookmarkColor();
      break;
    case R.id.ll__bookmark:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_BOOKMARK);
      AlohaHelper.logClick(AlohaHelper.PP_BOOKMARK);
      toggleIsBookmark();
      break;
    case R.id.ll__share:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_SHARE);
      AlohaHelper.logClick(AlohaHelper.PP_SHARE);
      ShareOption.ANY.shareMapObject((Activity) getContext(), mMapObject);
      break;
    case R.id.ll__api_back:
      final Activity activity = (Activity) getContext();
      if (ParsedMwmRequest.hasRequest())
      {
        final ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
        if (ParsedMwmRequest.isPickPointMode())
          request.setPointData(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getTitle(), "");
        request.sendResponseAndFinish(activity, true);
      }
      else
        activity.finish();
      break;
    case R.id.ll__place_latlon:
      mIsLatLonDms = !mIsLatLonDms;
      MwmApplication.prefs().edit().putBoolean(PREF_USE_DMS, mIsLatLonDms).commit();
      refreshLatLon();
      break;
    case R.id.ll__place_phone:
      Intent intent = new Intent(Intent.ACTION_DIAL);
      intent.setData(Uri.parse("tel:" + mTvPhone.getText()));
      try
      {
        getContext().startActivity(intent);
      } catch (ActivityNotFoundException e)
      {
        AlohaHelper.logException(e);
      }
      break;
    case R.id.ll__place_website:
      followUrl(mTvWebsite.getText().toString());
      break;
    case R.id.ll__place_wiki:
      // TODO: Refactor and use separate getters for Wiki and all other PP meta info too.
      followUrl(mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA));
      break;
    case R.id.tv__bookmark_group:
      saveBookmarkTitle();
      selectBookmarkSet();
      break;
    case R.id.av__direction:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_DIRECTION_ARROW);
      AlohaHelper.logClick(AlohaHelper.PP_DIRECTION_ARROW);
      showBigDirection();
      break;
    case R.id.ll__place_email:
      intent = new Intent(Intent.ACTION_SENDTO);
      intent.setData(Utils.buildMailUri(mTvEmail.getText().toString(), "", ""));
      getContext().startActivity(intent);
      break;
    case R.id.tv__bookmark_notes:
    case R.id.tv__description:
    case R.id.btn__edit_html_bookmark:
      saveBookmarkTitle();
      final Bundle args = new Bundle();
      args.putParcelable(EditDescriptionFragment.EXTRA_BOOKMARK, mMapObject);
      String name = EditDescriptionFragment.class.getName();
      final EditDescriptionFragment fragment = (EditDescriptionFragment) Fragment.instantiate(getContext(), name, args);
      fragment.setArguments(args);
      fragment.show(((FragmentActivity) getContext()).getSupportFragmentManager(), name);
      subscribeBookmarkEditFragment(fragment);
      break;
    case R.id.from:
      if (RoutingController.get().setStartPoint(mMapObject))
        hide();
      break;
    case R.id.to:
      if (RoutingController.get().setEndPoint(mMapObject))
        hide();
      break;
    }
  }

  private void followUrl(String url)
  {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    if (!url.startsWith("http://") && !url.startsWith("https://"))
      url = "http://" + url;
    intent.setData(Uri.parse(url));
    getContext().startActivity(intent);
  }

  private void toggleIsBookmark()
  {
    if (mMapObject == null)
      return;
    // TODO(yunikkk): this can be done by querying place_page::Info::IsBookmark(), without passing any
    // specific Bookmark object instance.
    if (MapObject.isOfType(MapObject.BOOKMARK, mMapObject))
    {
      setMapObject(Framework.nativeDeleteBookmarkFromMapObject(), true);
      setState(State.DETAILS);
    }
    else
    {
      setMapObject(BookmarkManager.INSTANCE.addNewBookmark(BookmarkManager.nativeFormatNewBookmarkName(), mMapObject.getLat(), mMapObject.getLon()), true);
      // FIXME this hack is necessary to get correct views height in animation controller. remove after further investigation.
      post(new Runnable()
      {
        @Override
        public void run()
        {
          setState(State.BOOKMARK);
        }
      });
    }
  }

  private void selectBookmarkSet()
  {
    final FragmentActivity activity = (FragmentActivity) getContext();
    final Bookmark bookmark = (Bookmark) mMapObject;

    final Bundle args = new Bundle();
    args.putInt(ChooseBookmarkCategoryFragment.CATEGORY_ID, bookmark.getCategoryId());
    args.putInt(ChooseBookmarkCategoryFragment.BOOKMARK_ID, bookmark.getBookmarkId());
    final ChooseBookmarkCategoryFragment fragment =
        (ChooseBookmarkCategoryFragment) Fragment.instantiate(activity, ChooseBookmarkCategoryFragment.class.getName(), args);
    fragment.show(activity.getSupportFragmentManager(), null);
  }

  private void selectBookmarkColor()
  {
    final Bundle args = new Bundle();
    args.putString(BookmarkColorDialogFragment.ICON_TYPE, ((Bookmark) mMapObject).getIcon().getType());
    final BookmarkColorDialogFragment dialogFragment =
        (BookmarkColorDialogFragment) Fragment.instantiate(getContext(), BookmarkColorDialogFragment.class.getName(), args);

    dialogFragment.setOnColorSetListener(new BookmarkColorDialogFragment.OnBookmarkColorChangeListener()
    {
      @Override
      public void onBookmarkColorSet(int colorPos)
      {
        Bookmark bmk = (Bookmark) mMapObject;
        final Icon newIcon = BookmarkManager.ICONS.get(colorPos);
        final String from = bmk.getIcon().getName();
        final String to = newIcon.getName();
        if (TextUtils.equals(from, to))
          return;

        Statistics.INSTANCE.trackColorChanged(from, to);
        bmk.setParams(bmk.getTitle(), newIcon, bmk.getBookmarkDescription());
        bmk = BookmarkManager.INSTANCE.getBookmark(bmk.getCategoryId(), bmk.getBookmarkId());
        setMapObject(bmk, true);
      }
    });

    dialogFragment.show(((FragmentActivity) getContext()).getSupportFragmentManager(), null);
  }

  private void showBigDirection()
  {
    final FragmentActivity hostActivity = (FragmentActivity) getContext();
    final DirectionFragment fragment = (DirectionFragment) Fragment.instantiate(hostActivity, DirectionFragment.class.getName(), null);
    fragment.setMapObject(mMapObject);
    fragment.show(hostActivity.getSupportFragmentManager(), null);
  }

  @Override
  public boolean onLongClick(View v)
  {
    final Object tag = v.getTag();
    final String tagStr = tag == null ? "" : tag.toString();
    AlohaHelper.logLongClick(tagStr);

    final PopupMenu popup = new PopupMenu(getContext(), v);
    final Menu menu = popup.getMenu();
    final List<String> items = new ArrayList<>();
    switch (v.getId())
    {
    case R.id.ll__place_latlon:
      final double lat = mMapObject.getLat();
      final double lon = mMapObject.getLon();
      items.add(Framework.nativeFormatLatLon(lat, lon, false));
      items.add(Framework.nativeFormatLatLon(lat, lon, true));
      break;
    case R.id.ll__place_website:
      items.add(mTvWebsite.getText().toString());
      break;
    case R.id.ll__place_email:
      items.add(mTvEmail.getText().toString());
      break;
    case R.id.ll__place_phone:
      items.add(mTvPhone.getText().toString());
      break;
    case R.id.ll__place_schedule:
      items.add(mFullOpeningHours.getText().toString());
      break;
    case R.id.ll__place_operator:
      items.add(mTvOperator.getText().toString());
      break;
    case R.id.ll__place_wiki:
      items.add(mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA));
      break;
    }

    final String copyText = getResources().getString(android.R.string.copy);
    for (int i = 0; i < items.size(); i++)
      menu.add(Menu.NONE, i, i, String.format("%s %s", copyText, items.get(i)));

    popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener()
    {
      @Override
      public boolean onMenuItemClick(MenuItem item)
      {
        final int id = item.getItemId();
        final Context ctx = getContext();
        Utils.copyTextToClipboard(ctx, items.get(id));
        Utils.toastShortcut(ctx, ctx.getString(R.string.copied_to_clipboard, items.get(id)));
        Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_METADATA_COPY + ":" + tagStr);
        AlohaHelper.logClick(AlohaHelper.PP_METADATA_COPY + ":" + tagStr);
        return true;
      }
    });

    popup.show();
    return true;
  }

  public int getDockedWidth()
  {
    int res = getWidth();
    return (res == 0 ? getLayoutParams().width : res);
  }

  public MwmActivity.LeftAnimationTrackListener getLeftAnimationTrackListener()
  {
    return mLeftAnimationTrackListener;
  }

  public void setLeftAnimationTrackListener(MwmActivity.LeftAnimationTrackListener listener)
  {
    mLeftAnimationTrackListener = listener;
  }

  public void hide()
  {
    setState(State.HIDDEN);
  }

  @SuppressWarnings("SimplifiableIfStatement")
  public boolean hideOnTouch()
  {
    if (mIsDocked || mIsFloating)
      return false;

    if (getState() == State.BOOKMARK || getState() == State.DETAILS)
    {
      hide();
      return true;
    }

    return false;
  }
}
