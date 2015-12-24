package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
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
import android.widget.RatingBar;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

import com.mapswithme.maps.BuildConfig;
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
import com.mapswithme.maps.bookmarks.data.MapObject.MapObjectType;
import com.mapswithme.maps.bookmarks.data.MapObject.Poi;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.widget.ArrowView;
import com.mapswithme.maps.widget.BaseShadowController;
import com.mapswithme.maps.widget.ObservableScrollView;
import com.mapswithme.maps.widget.ScrollViewShadowController;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.StringUtils;
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

  // Preview
  private TextView mTvTitle;
  private Toolbar mToolbar;
  private TextView mTvSubtitle;
  private TextView mTvOpened;
  private ArrowView mAvDirection;
  private TextView mTvDistance;
  private RatingBar mRbStars;
  private TextView mTvElevation;
  // Place page details
  private ScrollView mPpDetails;
  private RelativeLayout mAddress;
  private TextView mTvAddress;
  private LinearLayout mPhone;
  private TextView mTvPhone;
  private LinearLayout mWebsite;
  private TextView mTvWebsite;
  private LinearLayout mLatlon;
  private TextView mTvLatlon;
  private LinearLayout mSchedule;
  private TextView mTvSchedule;
  private LinearLayout mWifi;
  private LinearLayout mEmail;
  private TextView mTvEmail;
  private LinearLayout mOperator;
  private TextView mTvOperator;
  private LinearLayout mCuisine;
  private TextView mTvCuisine;
  private LinearLayout mWiki;
  private TextView mTvWiki;
  private LinearLayout mEntrance;
  private TextView mTvEntrance;
  // Bookmark
  private ImageView mIvColor;
  private EditText mEtBookmarkName;
  private TextView mTvNotes;
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

  private MapObject mBookmarkedMapObject;
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

    ViewGroup ppPreview = (ViewGroup) findViewById(R.id.pp__preview);
    mTvTitle = (TextView) ppPreview.findViewById(R.id.tv__title);
    mToolbar = (Toolbar) findViewById(R.id.toolbar);
    mTvSubtitle = (TextView) ppPreview.findViewById(R.id.tv__subtitle);
    mTvOpened = (TextView) ppPreview.findViewById(R.id.tv__opened_till);
    mTvDistance = (TextView) ppPreview.findViewById(R.id.tv__straight_distance);
    mAvDirection = (ArrowView) ppPreview.findViewById(R.id.av__direction);
    mAvDirection.setOnClickListener(this);
    mAvDirection.setImageResource(R.drawable.direction);
    mRbStars = (RatingBar) ppPreview.findViewById(R.id.rb__stars);
    mTvElevation = (TextView) ppPreview.findViewById(R.id.tv__peak_elevation);

    mPpDetails = (ScrollView) findViewById(R.id.pp__details);
    mAddress = (RelativeLayout) mPpDetails.findViewById(R.id.ll__place_name);
    mTvAddress = (TextView) mPpDetails.findViewById(R.id.tv__place_address);
    mPhone = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_phone);
    mPhone.setOnClickListener(this);
    mTvPhone = (TextView) mPpDetails.findViewById(R.id.tv__place_phone);
    mWebsite = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_website);
    mWebsite.setOnClickListener(this);
    mTvWebsite = (TextView) mPpDetails.findViewById(R.id.tv__place_website);
    mLatlon = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_latlon);
    mLatlon.setOnClickListener(this);
    mTvLatlon = (TextView) mPpDetails.findViewById(R.id.tv__place_latlon);
    mSchedule = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_schedule);
    mTvSchedule = (TextView) mPpDetails.findViewById(R.id.tv__place_schedule);
    mWifi = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_wifi);
    mIvColor = (ImageView) mPpDetails.findViewById(R.id.iv__bookmark_color);
    mIvColor.setOnClickListener(this);
    mEmail = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_email);
    mEmail.setOnClickListener(this);
    mTvEmail = (TextView) mEmail.findViewById(R.id.tv__place_email);
    mOperator = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_operator);
    mOperator.setOnClickListener(this);
    mTvOperator = (TextView) mOperator.findViewById(R.id.tv__place_operator);
    mCuisine = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_cuisine);
    mTvCuisine = (TextView) mCuisine.findViewById(R.id.tv__place_cuisine);
    mWiki = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_wiki);
    mWiki.setOnClickListener(this);
    mEntrance = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_entrance);
    mTvEntrance = (TextView) mEntrance.findViewById(R.id.tv__place_entrance);
    mLatlon.setOnLongClickListener(this);
    mAddress.setOnLongClickListener(this);
    mPhone.setOnLongClickListener(this);
    mWebsite.setOnLongClickListener(this);
    mSchedule.setOnLongClickListener(this);
    mEmail.setOnLongClickListener(this);
    mOperator.setOnLongClickListener(this);
    mWiki.setOnLongClickListener(this);

    mEtBookmarkName = (EditText) mPpDetails.findViewById(R.id.et__bookmark_name);
    mEtBookmarkName.setOnEditorActionListener(new TextView.OnEditorActionListener()
    {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
      {
        if (actionId == EditorInfo.IME_ACTION_DONE)
        {
          saveBookmarkNameIfUpdated();
          refreshPreview();
        }

        return false;
      }
    });

    mTvNotes = (TextView) mPpDetails.findViewById(R.id.tv__bookmark_notes);
    mTvNotes.setOnClickListener(this);

    mTvBookmarkGroup = (TextView) mPpDetails.findViewById(R.id.tv__bookmark_group);
    mTvBookmarkGroup.setOnClickListener(this);
    mWvDescription = (WebView) mPpDetails.findViewById(R.id.wv__description);
    mTvDescription = (TextView) mPpDetails.findViewById(R.id.tv__description);
    mTvDescription.setOnClickListener(this);
    mBtnEditHtmlDescription = (Button) mPpDetails.findViewById(R.id.btn__edit_html_bookmark);
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

    mShadowController = new ScrollViewShadowController((ObservableScrollView) mPpDetails)
                            .addBottomShadow()
                            .attach();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      setElevation(UiUtils.dimen(R.dimen.placepage_elevation));

    if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE)
      mPpDetails.setBackgroundResource(0);
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

    mPpDetails.scrollTo(0, 0);

    if (mMapObject != null)
      mAnimationController.setState(state, mMapObject.getType());
  }

  public MapObject getMapObject()
  {
    saveBookmarkNameIfUpdated();
    return mMapObject;
  }

  public void setMapObject(MapObject mapObject)
  {
    if (hasMapObject(mapObject))
      return;

    if (!(mapObject instanceof Bookmark))
      saveBookmarkNameIfUpdated();

    mMapObject = mapObject;
    refreshViews();
  }

  public boolean hasMapObject(MapObject mo)
  {
    if (mo == null && mMapObject == null)
      return true;
    else if (mMapObject != null)
      return mMapObject.sameAs(mo);

    return false;
  }

  public void refreshViews()
  {
    if (mMapObject == null)
      return;

    mMapObject.setDefaultIfEmpty();

    refreshPreview();
    refreshDetails();
    final Location loc = LocationHelper.INSTANCE.getLastLocation();

    switch (mMapObject.getType())
    {
    case BOOKMARK:
      refreshDistanceToObject(loc);
      showBookmarkDetails();
      refreshButtons(false, true);
      break;
    case POI:
    case ADDITIONAL_LAYER:
      refreshDistanceToObject(loc);
      hideBookmarkDetails();
      refreshButtons(false, true);
      break;
    case API_POINT:
      refreshDistanceToObject(loc);
      hideBookmarkDetails();
      refreshButtons(true, true);
      break;
    case MY_POSITION:
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
    mTvTitle.setText(mMapObject.getName());
    if (mToolbar != null)
      mToolbar.setTitle(mMapObject.getName());
    String subtitle = mMapObject.getPoiTypeName();
    final String cuisine = mMapObject.getMetadata(Metadata.MetadataType.FMD_CUISINE);
    if (cuisine != null)
      subtitle += ", " + translateCuisine(cuisine);
    mTvSubtitle.setText(subtitle);
    mAvDirection.setVisibility(View.GONE);
    // TODO show/hide mTvOpened after schedule fill be parsed
  }

  public String translateCuisine(String cuisine)
  {
    if (TextUtils.isEmpty(cuisine))
      return cuisine;

    // cuisines translations can contain unsupported symbols, and res ids
    // replace them with supported "_"( so ', ' and ' ' are replaced with underlines)
    final String[] cuisines = cuisine.split(";");
    String result = "";
    // search translations for each cuisine
    for (String cuisineRaw : cuisines)
    {
      final String cuisineKey = cuisineRaw.replace(", ", "_").replace(' ', '_').toLowerCase();
      int resId = getResources().getIdentifier("cuisine_" + cuisineKey, "string", BuildConfig.APPLICATION_ID);
      result += resId == 0 ? cuisineRaw : getResources().getString(resId);
    }
    return result;
  }

  private void refreshDetails()
  {
    refreshLatLon();
    final String website = mMapObject.getMetadata(Metadata.MetadataType.FMD_WEBSITE);
    refreshMetadataOrHide(TextUtils.isEmpty(website) ? mMapObject.getMetadata(Metadata.MetadataType.FMD_URL) : website, mWebsite, mTvWebsite);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER), mPhone, mTvPhone);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_EMAIL), mEmail, mTvEmail);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_OPERATOR), mOperator, mTvOperator);
    refreshMetadataOrHide(translateCuisine(mMapObject.getMetadata(Metadata.MetadataType.FMD_CUISINE)), mCuisine, mTvCuisine);
    // TODO @yunikkk uncomment wiki display when data with correct wiki representation(urlencoded once) will be ready
//    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA), mWiki, null);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_INTERNET), mWifi, null);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_FLATS), mEntrance, mTvEntrance);
    // TODO throw away parsing hack when data will be parsed correctly in core
    final String rawSchedule = mMapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    refreshMetadataOrHide(TextUtils.isEmpty(rawSchedule) ? null : rawSchedule.replace("; ", "\n").replace(';', '\n'), mSchedule, mTvSchedule);
    refreshMetadataStars(mMapObject.getMetadata(Metadata.MetadataType.FMD_STARS));
    UiUtils.setTextAndHideIfEmpty(mTvElevation, mMapObject.getMetadata(Metadata.MetadataType.FMD_ELE));
  }

  private void hideBookmarkDetails()
  {
    mIvBookmark.setImageDrawable(Graphics.tint(getContext(), R.drawable.ic_bookmarks_off, R.attr.iconTint));
  }

  private void showBookmarkDetails()
  {
    final Bookmark bookmark = (Bookmark) mMapObject;
    mEtBookmarkName.setText(bookmark.getName());
    mTvBookmarkGroup.setText(bookmark.getCategoryName(getContext()));
    mIvColor.setImageResource(bookmark.getIcon().getSelectedResId());
    mIvBookmark.setImageResource(R.drawable.ic_bookmarks_on);
    final String notes = bookmark.getBookmarkDescription();
    if (notes.isEmpty())
      UiUtils.hide(mWvDescription, mBtnEditHtmlDescription, mTvDescription);
    else if (StringUtils.isHtml(notes))
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
      UiUtils.hide(mGeneralButtonsFrame);
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

    if (mMapObject.getType() == MapObjectType.MY_POSITION)
      refreshMyPosition(l);
    else
      refreshDistanceToObject(l);
  }

  private void refreshMyPosition(Location l)
  {
    mTvDistance.setVisibility(View.GONE);

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
      final DistanceAndAzimut distanceAndAzimuth = Framework.nativeGetDistanceAndAzimutFromLatLon(
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

  private static void refreshMetadataOrHide(String metadata, LinearLayout metaLayout, TextView metaTv)
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

  private void refreshMetadataStars(String stars)
  {
    if (TextUtils.isEmpty(stars))
    {
      mRbStars.setVisibility(View.GONE);
      return;
    }

    try
    {
      mRbStars.setRating(Float.parseFloat(stars));
      mRbStars.setVisibility(View.VISIBLE);
    } catch (NumberFormatException e)
    {
      mRbStars.setVisibility(View.GONE);
    }
  }

  public void refreshAzimuth(double northAzimuth)
  {
    if (getState() == State.HIDDEN || mMapObject == null || mMapObject.getType() == MapObjectType.MY_POSITION)
      return;

    final Location location = LocationHelper.INSTANCE.getLastLocation();
    if (location == null)
      return;

    final double azimuth = Framework.nativeGetDistanceAndAzimutFromLatLon(mMapObject.getLat(), mMapObject.getLon(),
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

  public void onResume()
  {
    if (mMapObject == null)
      return;

    checkBookmarkWasDeleted();
    checkApiWasCanceled();
  }

  // TODO remove that method completely. host activity should check that itself
  private void checkApiWasCanceled()
  {
    if ((mMapObject.getType() == MapObjectType.API_POINT) && !ParsedMwmRequest.hasRequest())
      setMapObject(null);
  }

  // TODO refactor processing of bookmarks.
  private void checkBookmarkWasDeleted()
  {
    // We need to check, if content of body is still valid
    if (mMapObject.getType() == MapObjectType.BOOKMARK)
    {
      final Bookmark bmk = (Bookmark) mMapObject;
      boolean deleted = false;

      if (BookmarkManager.INSTANCE.getCategoriesCount() <= bmk.getCategoryId())
        deleted = true;
      else if (BookmarkManager.INSTANCE.getCategoryById(bmk.getCategoryId()).getBookmarksCount() <= bmk.getBookmarkId())
        deleted = true;
      else if (BookmarkManager.INSTANCE.getBookmark(bmk.getCategoryId(), bmk.getBookmarkId()).getLat() != bmk.getLat())
        deleted = true;
      // We can do check above, because lat/lon cannot be changed from edit screen.

      if (deleted)
      {
        // Make Poi from bookmark
        final MapObject p = new Poi(mMapObject.getName(), mMapObject.getLat(), mMapObject.getLon(), null);
        setMapObject(p);
        // TODO how to handle the case, when bookmark was moved to another group?
      }
      else
      {
        // Update data for current bookmark
        final Bookmark updatedBmk = BookmarkManager.INSTANCE.getBookmark(bmk.getCategoryId(), bmk.getBookmarkId());
        setMapObject(null);
        setMapObject(updatedBmk);
      }
    }
  }

  private void saveBookmarkNameIfUpdated()
  {
    // Can't save bookmark name if current object is not bookmark.
    if (mMapObject == null || !(mMapObject instanceof Bookmark))
      return;

    final Bookmark bookmark = (Bookmark) mMapObject;
    final String name = mEtBookmarkName.getText().toString();
    bookmark.setParams(name, null, bookmark.getBookmarkDescription());
  }

  /**
   * Adds listener to {@link EditDescriptionFragment} to catch notification about bookmark description edit is complete.
   * <br/>When the user rotates device screen the listener is lost, so we must re-subscribe again.
   * @param fragment if specified - explicitely subscribe to this fragment. Otherwise try to find the fragment by hands.
   */
  private void subscribeBookmarkEditFragment(@Nullable EditDescriptionFragment fragment)
  {
    if (fragment == null)
    {
      FragmentManager fm = ((FragmentActivity)getContext()).getSupportFragmentManager();
      fragment = (EditDescriptionFragment)fm.findFragmentByTag(EditDescriptionFragment.class.getName());
    }

    if (fragment == null)
      return;

    fragment.setSaveDescriptionListener(new EditDescriptionFragment.OnDescriptionSavedListener()
    {
      @Override
      public void onSaved(Bookmark bookmark)
      {
        final Bookmark updatedBookmark = BookmarkManager.INSTANCE.getBookmark(bookmark.getCategoryId(), bookmark.getBookmarkId());
        setMapObject(updatedBookmark);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.BMK_DESCRIPTION_CHANGED);
      }
    });
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.iv__bookmark_color:
      saveBookmarkNameIfUpdated();
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
          request.setPointData(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getName(), "");
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
      saveBookmarkNameIfUpdated();
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
      saveBookmarkNameIfUpdated();
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
    if (mMapObject.getType() == MapObjectType.BOOKMARK)
    {
      final Bookmark currentBookmark = (Bookmark) mMapObject;
      MapObject p;
      if (mBookmarkedMapObject != null && LocationUtils.areLatLonEqual(mMapObject, mBookmarkedMapObject))
        // use cached POI of bookmark, if it corresponds to current object
        p = mBookmarkedMapObject;
      else
        p = Framework.nativeGetMapObjectForPoint(mMapObject.getLat(), mMapObject.getLon());

      setMapObject(p);
      setState(State.DETAILS);
      BookmarkManager.INSTANCE.deleteBookmark(currentBookmark);
    }
    else
    {
      mBookmarkedMapObject = mMapObject;
      final Bookmark newBmk = BookmarkManager.INSTANCE.getBookmark(BookmarkManager.INSTANCE.addNewBookmark(
          mMapObject.getName(), mMapObject.getLat(), mMapObject.getLon()));
      setMapObject(newBmk);
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
    final ChooseBookmarkCategoryFragment fragment = (ChooseBookmarkCategoryFragment) Fragment.instantiate(activity, ChooseBookmarkCategoryFragment.class.getName(), args);
    fragment.show(activity.getSupportFragmentManager(), null);
  }

  private void selectBookmarkColor()
  {
    final Bundle args = new Bundle();
    args.putString(BookmarkColorDialogFragment.ICON_TYPE, ((Bookmark) mMapObject).getIcon().getType());
    final BookmarkColorDialogFragment dialogFragment = (BookmarkColorDialogFragment) BookmarkColorDialogFragment.
        instantiate(getContext(), BookmarkColorDialogFragment.class.getName(), args);

    dialogFragment.setOnColorSetListener(new BookmarkColorDialogFragment.OnBookmarkColorChangeListener()
    {
      @Override
      public void onBookmarkColorSet(int colorPos)
      {
        Bookmark bmk = (Bookmark) mMapObject;
        final Icon newIcon = BookmarkManager.getIcons().get(colorPos);
        final String from = bmk.getIcon().getName();
        final String to = newIcon.getName();
        if (!TextUtils.equals(from, to))
          Statistics.INSTANCE.trackColorChanged(from, to);

        bmk.setParams(bmk.getName(), newIcon, bmk.getBookmarkDescription());
        bmk = BookmarkManager.INSTANCE.getBookmark(bmk.getCategoryId(), bmk.getBookmarkId());
        setMapObject(bmk);
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
      items.add(mTvSchedule.getText().toString());
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
