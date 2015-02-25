package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.TypedArray;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.FragmentActivity;
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
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.MapObjectType;
import com.mapswithme.maps.bookmarks.data.MapObject.Poi;
import com.mapswithme.maps.bookmarks.data.MapObject.SearchResult;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.widget.ArrowView;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;


public class PlacePageView extends LinearLayout implements View.OnClickListener, View.OnLongClickListener, TextView.OnEditorActionListener
{
  private LayoutInflater mInflater;
  // Preview
  private TextView mTvTitle;
  private TextView mTvSubtitle;
  private TextView mTvOpened;
  private ArrowView mAvDirection;
  private TextView mTvDistance;
  // Place page details
  private ViewGroup mPpDetails;
  private ScrollView mPlacePageContainer;
  private LinearLayout mLlAddress;
  private TextView mTvAddress;
  private LinearLayout mLlPhone;
  private TextView mTvPhone;
  private LinearLayout mLlWebsite;
  private TextView mTvWebsite;
  private LinearLayout mLlLatlon;
  private TextView mTvLatlon;
  private LinearLayout mLlSchedule;
  private TextView mTvSchedule;
  // Bookmark
  private RelativeLayout mRlBookmarkDetails;
  private ImageView mIvColor;
  private EditText mEtBookmarkName;
  private EditText mEtBookmarkNotes;
  private TextView mTvBookmarkGroup;
  // Place page buttons
  private RelativeLayout mRlApiBack;
  private ImageView mIvBookmark;
  // Animations
  private BasePlacePageAnimationController mAnimationController;
  // Data
  private MapObject mMapObject;
  private State mCurrentState = State.HIDDEN;

  private MapObject mBookmarkedMapObject;
  private boolean mIsLatLonDms;
  private static final String PREF_USE_DMS = "use_dms";

  public static enum State
  {
    HIDDEN,
    PREVIEW_ONLY,
    FULL_PLACEPAGE
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
    super(context, attrs, defStyleAttr);

    mIsLatLonDms = context.getSharedPreferences(context.getString(R.string.pref_file_name),
        Context.MODE_PRIVATE).getBoolean(PREF_USE_DMS, false);

    mInflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    initViews();

    initAnimationController(attrs, defStyleAttr);
    setVisibility(View.INVISIBLE);
  }

  private void initViews()
  {
    View mView = mInflater.inflate(R.layout.place_page, this, true);
    mView.setOnClickListener(this);

    ViewGroup ppPreview = (ViewGroup) mView.findViewById(R.id.pp__preview);
    mTvTitle = (TextView) ppPreview.findViewById(R.id.tv__title);
    mTvSubtitle = (TextView) ppPreview.findViewById(R.id.tv__subtitle);
    mTvOpened = (TextView) ppPreview.findViewById(R.id.tv__opened_till);
    mTvDistance = (TextView) ppPreview.findViewById(R.id.tv__straight_distance);
    mAvDirection = (ArrowView) ppPreview.findViewById(R.id.av__direction);
    // TODO direction arrow & screen

    mPpDetails = (ViewGroup) mView.findViewById(R.id.pp__details);
    mLlAddress = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_name);
    mTvAddress = (TextView) mPpDetails.findViewById(R.id.tv__place_address);
    mLlPhone = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_phone);
    mLlPhone.setOnClickListener(this);
    mTvPhone = (TextView) mPpDetails.findViewById(R.id.tv__place_phone);
    mLlWebsite = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_website);
    mLlWebsite.setOnClickListener(this);
    mTvWebsite = (TextView) mPpDetails.findViewById(R.id.tv__place_website);
    mLlLatlon = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_latlon);
    mTvLatlon = (TextView) mPpDetails.findViewById(R.id.tv__place_latlon);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
      mLlLatlon.setOnLongClickListener(this);
    mLlLatlon.setOnClickListener(this);
    mLlSchedule = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_schedule);
    mLlSchedule.setOnClickListener(this);
    mTvSchedule = (TextView) mPpDetails.findViewById(R.id.tv__place_schedule);
    mIvColor = (ImageView) mPpDetails.findViewById(R.id.iv__bookmark_color);
    mIvColor.setOnClickListener(this);

    mRlBookmarkDetails = (RelativeLayout) mPpDetails.findViewById(R.id.rl__bookmark_details);
    mEtBookmarkName = (EditText) mPpDetails.findViewById(R.id.et__bookmark_name);
    mEtBookmarkName.setOnEditorActionListener(this);
    mEtBookmarkNotes = (EditText) mPpDetails.findViewById(R.id.et__bookmark_notes);
    mEtBookmarkNotes.setOnEditorActionListener(this);
    mTvBookmarkGroup = (TextView) mPpDetails.findViewById(R.id.tv__bookmark_group);

    ViewGroup ppButtons = (ViewGroup) mView.findViewById(R.id.pp__buttons);
    mRlApiBack = (RelativeLayout) ppButtons.findViewById(R.id.rl__api_back);
    mRlApiBack.setOnClickListener(this);
    final ViewGroup bookmarkGroup = (ViewGroup) ppButtons.findViewById(R.id.rl__bookmark);
    bookmarkGroup.setOnClickListener(this);
    mIvBookmark = (ImageView) bookmarkGroup.findViewById(R.id.iv__bookmark);
    ppButtons.findViewById(R.id.rl__share).setOnClickListener(this);

    // Place Page
    mPlacePageContainer = (ScrollView) mPpDetails.findViewById(R.id.place_page_container);
  }

  private void initAnimationController(AttributeSet attrs, int defStyleAttr)
  {
    final TypedArray attrArray = getContext().obtainStyledAttributes(attrs, R.styleable.PlacePageView, defStyleAttr, 0);
    final int animationType = attrArray.getInt(R.styleable.PlacePageView_animationType, 0);
    attrArray.recycle();
    // switch with values from "animationType" from attrs.xml
    switch (animationType)
    {
    case 0:
      mAnimationController = new BottomPlacePageAnimationController(this);
      break;
    case 1:
      mAnimationController = new TopPlacePageAnimationController(this);
      break;
    case 2:
      mAnimationController = new LeftFloatPlacePageAnimationController(this);
      break;
    case 3:
      mAnimationController = new LeftFullAnimationController(this);
      break;
    }
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

  public State getState()
  {
    return mCurrentState;
  }

  public void setState(State state)
  {
    if (mCurrentState != state)
    {
      // Do some transitions
      if (mCurrentState == State.HIDDEN && state == State.PREVIEW_ONLY)
        mAnimationController.showPreview(true);
      else if (mCurrentState == State.PREVIEW_ONLY && state == State.FULL_PLACEPAGE)
        mAnimationController.showPlacePage(true);
      else if (mCurrentState == State.PREVIEW_ONLY && state == State.HIDDEN)
        mAnimationController.showPreview(false);
      else if (mCurrentState == State.FULL_PLACEPAGE && state == State.PREVIEW_ONLY)
        mAnimationController.showPlacePage(false);
      else if (mCurrentState == State.FULL_PLACEPAGE && state == State.HIDDEN)
        mAnimationController.hidePlacePage();
      else
        return;

      mCurrentState = state;
    }
  }

  public boolean hasMapObject(MapObject mo)
  {
    if (mo == null && mMapObject == null)
      return true;
    else if (mMapObject != null)
      return mMapObject.equals(mo);

    return false;
  }

  public void setMapObject(MapObject mo)
  {
    if (!hasMapObject(mo))
    {
      mMapObject = mo;
      if (mMapObject != null)
      {
        mMapObject.setDefaultIfEmpty(getResources());

        switch (mMapObject.getType())
        {
        case POI:
          fillPlacePagePoi(mMapObject);
          break;
        case BOOKMARK:
          fillPlacePageBookmark((Bookmark) mMapObject);
          break;
        case ADDITIONAL_LAYER:
          fillPlacePageLayer((SearchResult) mMapObject);
          break;
        case API_POINT:
        case MY_POSITION:
          fillPlacePageApi(mo);
          break;
        default:
          throw new IllegalArgumentException("Unknown MapObject type:" + mo.getType());
        }

        refreshPreview();
        refreshDetails();
        refreshButtons();
      }
    }
  }

  public MapObject getMapObject()
  {
    return mMapObject;
  }

  private void refreshPreview()
  {
    mTvTitle.setText(mMapObject.getName());
    mTvSubtitle.setText(mMapObject.getPoiTypeName());
    // TODO
    //    mTvOpened
  }

  private void refreshDetails()
  {
    refreshLocation(LocationHelper.INSTANCE.getLastLocation());
    refreshLatLon();
    // TODO refresh full details
  }

  private void refreshButtons()
  {
    if (mMapObject.getType() == MapObjectType.API_POINT ||
        (ParsedMmwRequest.hasRequest() && ParsedMmwRequest.getCurrentRequest().isPickPointMode()))
      mRlApiBack.setVisibility(View.VISIBLE);
    else
      mRlApiBack.setVisibility(View.GONE);
  }

  public void refreshLocation(Location l)
  {
    if (mMapObject != null)
    {
      if (mMapObject.getType() == MapObjectType.MY_POSITION)
      {
        final StringBuilder builder = new StringBuilder();
        if (l.hasAltitude())
          builder.append(Framework.nativeFormatAltitude(l.getAltitude()));
        if (l.hasSpeed())
          builder.append("   ").
              append(Framework.nativeFormatSpeed(l.getSpeed()));
        mTvSubtitle.setText(builder.toString());

        mTvDistance.setVisibility(View.GONE);

        mMapObject.setLat(l.getLatitude());
        mMapObject.setLon(l.getLongitude());
        refreshLatLon();
        mAvDirection.setVisibility(View.GONE);
      }
      else
      {
        if (l != null)
        {
          mTvDistance.setVisibility(View.VISIBLE);
          final DistanceAndAzimut distanceAndAzimuth = Framework.nativeGetDistanceAndAzimutFromLatLon(mMapObject.getLat(),
              mMapObject.getLon(), l.getLatitude(), l.getLongitude(), 0.0);
          mTvDistance.setText(distanceAndAzimuth.getDistance());
        }
        else
        {
          mAvDirection.setVisibility(View.GONE);
          mTvDistance.setVisibility(View.GONE);
        }
      }
    }
  }

  private void refreshLatLon()
  {
    final double lat = mMapObject.getLat();
    final double lon = mMapObject.getLon();
    final String[] latLon = Framework.nativeFormatLatLonToArr(lat, lon, mIsLatLonDms);
    if (latLon.length == 2)
      mTvLatlon.setText(latLon[0] + ", " + latLon[1]);
  }

  public void refreshAzimuth(double northAzimuth)
  {
    if (mMapObject != null && mMapObject.getType() != MapObjectType.MY_POSITION)
    {
      final Location l = LocationHelper.INSTANCE.getLastLocation();
      if (l != null)
      {
        final DistanceAndAzimut da = Framework.nativeGetDistanceAndAzimutFromLatLon(
            mMapObject.getLat(), mMapObject.getLon(),
            l.getLatitude(), l.getLongitude(), northAzimuth);

        if (da.getAthimuth() >= 0)
        {
          mAvDirection.setVisibility(View.VISIBLE);
          mAvDirection.setAzimut(da.getAthimuth());
        }
      }
    }
  }

  private void fillPlacePagePoi(MapObject poi)
  {
    mRlBookmarkDetails.setVisibility(View.GONE);
    // TODO
    mIvBookmark.setImageResource(R.drawable.ic_bookmark_off);
  }

  private void fillPlacePageBookmark(final Bookmark bmk)
  {
    mRlBookmarkDetails.setVisibility(View.VISIBLE);
    mEtBookmarkName.setText(bmk.getName());
    mEtBookmarkNotes.setText(bmk.getBookmarkDescription());
    // TODO add HTML & webview support

    mIvColor.setImageResource(bmk.getIcon().getSelectedResId());
    mIvBookmark.setImageResource(R.drawable.ic_bookmark_on);
  }

  private void fillPlacePageLayer(SearchResult sr)
  {
    mRlBookmarkDetails.setVisibility(View.GONE);
    // TODO
    mIvBookmark.setImageResource(R.drawable.ic_bookmark_off);
  }

  private void fillPlacePageApi(MapObject mo)
  {
    mRlBookmarkDetails.setVisibility(View.GONE);
    // TODO
    mIvBookmark.setImageResource(R.drawable.ic_bookmark_off);
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

  private void checkApiWasCanceled()
  {
    if ((mMapObject.getType() == MapObjectType.API_POINT) && !ParsedMmwRequest.hasRequest())
    {
      setMapObject(null);

      mAnimationController.hidePlacePage();
    }
  }

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

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.iv__bookmark_color:
      selectBookmarkColor();
      break;
    case R.id.rl__bookmark:
      if (mMapObject == null)
        return;
      if (mMapObject.getType() == MapObjectType.BOOKMARK)
      {
        MapObject p;
        if (mBookmarkedMapObject != null &&
            mBookmarkedMapObject.getLat() == mMapObject.getLat() &&
            mBookmarkedMapObject.getLon() == mMapObject.getLon()) // use cached POI of bookmark, if it corresponds to current object
          p = mBookmarkedMapObject;
        else
          p = Framework.nativeGetMapObjectForPoint(mMapObject.getLat(), mMapObject.getLon());

        BookmarkManager.INSTANCE.deleteBookmark((Bookmark) mMapObject);
        setMapObject(p);
      }
      else
      {
        mBookmarkedMapObject = mMapObject;
        final Bookmark newBmk = BookmarkManager.INSTANCE.getBookmark(BookmarkManager.INSTANCE.addNewBookmark(
            mMapObject.getName(), mMapObject.getLat(), mMapObject.getLon()));
        setMapObject(newBmk);
      }
      Framework.invalidate();
      break;
    case R.id.rl__share:
      ShareAction.getAnyShare().shareMapObject((Activity) getContext(), mMapObject);
      break;
    case R.id.rl__api_back:
      if (ParsedMmwRequest.hasRequest() && ParsedMmwRequest.getCurrentRequest().isPickPointMode())
        ParsedMmwRequest.getCurrentRequest().setPointData(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getName(), "");
      ParsedMmwRequest.getCurrentRequest().sendResponseAndFinish((Activity) getContext(), true);
      break;
    case R.id.ll__place_latlon:
      mIsLatLonDms = !mIsLatLonDms;
      getContext().getSharedPreferences(getContext().getString(R.string.pref_file_name),
          Context.MODE_PRIVATE).edit().putBoolean(PREF_USE_DMS, mIsLatLonDms).commit();
      refreshLatLon();
      break;
    case R.id.ll__place_phone:
      Intent intent = new Intent(Intent.ACTION_DIAL);
      intent.setData(Uri.parse("tel:" + mTvPhone.getText()));
      getContext().startActivity(intent);
      break;
    case R.id.ll__place_website:
      intent = new Intent(Intent.ACTION_VIEW);
      String website = mTvWebsite.getText().toString();
      if (!website.startsWith("http://") && !website.startsWith("https://"))
        website = "http://" + website;
      intent.setData(Uri.parse(website));
      getContext().startActivity(intent);
      break;
    case R.id.ll__place_schedule:
      // TODO expand/collapse schedule if needed
      break;
    default:
      break;
    }
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
        final Icon newIcon = BookmarkManager.INSTANCE.getIcons().get(colorPos);
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

  @Override
  public boolean onLongClick(View v)
  {
    final PopupMenu popup = new PopupMenu(getContext(), v);
    final Menu menu = popup.getMenu();
    final double lat = mMapObject.getLat();
    final double lon = mMapObject.getLon();

    final String copyText = getResources().getString(android.R.string.copy);
    final String arrCoord[] = {
        Framework.nativeFormatLatLon(lat, lon, false),
        Framework.nativeFormatLatLon(lat, lon, true)};

    menu.add(Menu.NONE, 0, 0, String.format("%s %s", copyText, arrCoord[0]));
    menu.add(Menu.NONE, 1, 1, String.format("%s %s", copyText, arrCoord[1]));
    menu.add(Menu.NONE, 2, 2, android.R.string.cancel);

    popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener()
    {
      @Override
      public boolean onMenuItemClick(MenuItem item)
      {
        final int id = item.getItemId();
        if (id >= 0 && id < 2)
        {
          final Context ctx = getContext();
          Utils.copyTextToClipboard(ctx, arrCoord[id]);
          Utils.toastShortcut(ctx, ctx.getString(R.string.copied_to_clipboard, arrCoord[id]));
        }
        return true;
      }
    });

    popup.show();
    return true;
  }

  @Override
  public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
  {
    if (actionId == EditorInfo.IME_ACTION_DONE)
    {
      switch (v.getId())
      {
      case R.id.et__bookmark_name:
        Bookmark bookmark = (Bookmark) mMapObject;
        final String name = mEtBookmarkName.getText().toString().trim();
        bookmark.setParams(name, null, bookmark.getBookmarkDescription());
        break;
      case R.id.et__bookmark_notes:
        // TODO multiline notes & webview
        bookmark = (Bookmark) mMapObject;
        final String notes = mEtBookmarkNotes.getText().toString().trim();
        final String oldNotes = bookmark.getBookmarkDescription().trim();

        if (!TextUtils.equals(notes, oldNotes))
          Statistics.INSTANCE.trackDescriptionChanged();
        bookmark.setParams(mEtBookmarkName.getText().toString(), null, notes);
        break;
      }
    }
    return false;
  }
}
