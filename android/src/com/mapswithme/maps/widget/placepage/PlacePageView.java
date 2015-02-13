package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.ColorDrawable;
import android.location.Location;
import android.os.Build;
import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.bookmarks.IconsAdapter;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.MapObjectType;
import com.mapswithme.maps.bookmarks.data.MapObject.Poi;
import com.mapswithme.maps.bookmarks.data.MapObject.SearchResult;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;


public class PlacePageView extends LinearLayout implements View.OnClickListener, View.OnLongClickListener
{
  private static final int COLOR_CHOOSER_COLUMN_NUM = 4;

  private LayoutInflater mInflater;
  // Preview
  private TextView mTvTitle;
  private TextView mTvSubtitle;
  private TextView mTvOpened;
  private ImageView mAvDirection;
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
  private ImageView mIvColor;
  // Place page buttons
  private RelativeLayout mRlApiBack;
  private ImageView mIvBookmark;
  // Animations
  private BasePlacePageAnimationController mAnimationController;
  // Data
  private MapObject mMapObject;
  private State mCurrentState = State.HIDDEN;
  private BookmarkManager mBookmarkManager;
  private List<Icon> mIcons;
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

    mBookmarkManager = BookmarkManager.getBookmarkManager();
    mIcons = mBookmarkManager.getIcons();

    initAnimationController(attrs, defStyleAttr);
    setVisibility(View.GONE);
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
    mAvDirection = (ImageView) ppPreview.findViewById(R.id.iv__direction);
    //    mAvDirection.setDrawCircle(true);
    //    mAvDirection.setVisibility(View.GONE); // should be hidden until first compass update

    mPpDetails = (ViewGroup) mView.findViewById(R.id.pp__details);
    mLlAddress = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_name);
    mTvAddress = (TextView) mPpDetails.findViewById(R.id.tv__place_address);
    mLlPhone = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_phone);
    mTvPhone = (TextView) mPpDetails.findViewById(R.id.tv__place_phone);
    mLlWebsite = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_website);
    mTvWebsite = (TextView) mPpDetails.findViewById(R.id.tv__place_website);
    mLlLatlon = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_latlon);
    mTvLatlon = (TextView) mPpDetails.findViewById(R.id.tv__place_latlon);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
      mTvLatlon.setOnLongClickListener(this);
    mLlSchedule = (LinearLayout) mPpDetails.findViewById(R.id.ll__place_schedule);
    mTvSchedule = (TextView) mPpDetails.findViewById(R.id.tv__place_schedule);

    ViewGroup ppButtons = (ViewGroup) mView.findViewById(R.id.pp__buttons);
    mRlApiBack = (RelativeLayout) ppButtons.findViewById(R.id.rl__api_back);
    mRlApiBack.setOnClickListener(this);
    final ViewGroup bookmarkGroup = (ViewGroup) ppButtons.findViewById(R.id.rl__bookmark);
    bookmarkGroup.setOnClickListener(this);
    mIvBookmark = (ImageView) bookmarkGroup.findViewById(R.id.iv__bookmark);
    ppButtons.findViewById(R.id.rl__share).setOnClickListener(this);
    ppButtons.findViewById(R.id.rl__route).setOnClickListener(this);

    // Place Page
    mPlacePageContainer = (ScrollView) mPpDetails.findViewById(R.id.place_page_container);
  }

  private void initAnimationController(AttributeSet attrs, int defStyleAttr)
  {
    final TypedArray attrArray = getContext().obtainStyledAttributes(attrs, R.styleable.PlacePageView, defStyleAttr, 0);
    final int animationType = attrArray.getInt(R.styleable.PlacePageView_animationType, 0);
    Log.d("TEST", "AnimationType = " + animationType);
    attrArray.recycle();
    switch (animationType)
    {
    case 0:
      mAnimationController = new BottomPlacePageAnimationController(this);
      break;
    case 1:
      // FIXME
      mAnimationController = new TopPlacePageAnimationController(this);
      break;
    case 2:
      // FIXME
      mAnimationController = new TopPlacePageAnimationController(this);
      break;
    case 3:
      // FIXME
      mAnimationController = new TopPlacePageAnimationController(this);
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
    boolean res = mAnimationController.onInterceptTouchEvent(event);
    Log.d("TEST", "Intercept - " + res);
    return res;
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

        boolean isChecked = false;
        switch (mMapObject.getType())
        {
        case POI:
          fillPlacePagePoi(mMapObject);
          break;
        case BOOKMARK:
          isChecked = true;
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

        // TODO
        //        mIvBookmark.setChecked(isChecked);

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
    //    mTvOpened
    //    mTvDistance
    //    mAvDirection
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
          mTvDistance.setVisibility(View.GONE);
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
          //          mAvDirection.setAzimut(da.getAthimuth());
        }
      }
    }
  }

  private void fillPlacePagePoi(MapObject poi)
  {
//    mPlacePageContainer.removeAllViews();
//    final View poiView = mInflater.inflate(R.layout.info_box_poi, this, false);
//    mPlacePageContainer.addView(poiView);
  }

  private void fillPlacePageBookmark(final Bookmark bmk)
  {
//    mPlacePageContainer.removeAllViews();
//    final View bmkView = mInflater.inflate(R.layout.info_box_bookmark, this, false);
//    bmkView.setOnClickListener(this);

    // Description of BMK
//    final WebView descriptionWv = (WebView) bmkView.findViewById(R.id.info_box_bookmark_descr);
//    final String descriptionTxt = bmk.getBookmarkDescription();
//
//    if (TextUtils.isEmpty(descriptionTxt))
//      UiUtils.hide(descriptionWv);
//    else
//    {
//      descriptionWv.loadData(descriptionTxt, "text/html; charset=UTF-8", null);
//      descriptionWv.setBackgroundColor(Color.TRANSPARENT);
//      descriptionWv.setOnTouchListener(new OnTouchListener()
//      {
//        @Override
//        public boolean onTouch(View v, MotionEvent event)
//        {
//          switch (event.getAction())
//          {
//          case MotionEvent.ACTION_DOWN:
//            PlacePageView.this.requestDisallowInterceptTouchEvent(true);
//            break;
//          case MotionEvent.ACTION_UP:
//            PlacePageView.this.requestDisallowInterceptTouchEvent(false);
//            break;
//          }
//
//          v.onTouchEvent(event);
//          return true;
//        }
//      });
//
//      UiUtils.show(descriptionWv);
//    }
//
//    mIvColor = (ImageView) bmkView.findViewById(R.id.color_image);
//    mIvColor.setOnClickListener(this);
//    mIvColor.setVisibility(View.VISIBLE);
//    updateColorChooser(bmk.getIcon());
//
//    mPlacePageContainer.addView(bmkView);
  }

  private void fillPlacePageLayer(SearchResult sr)
  {
//    mPlacePageContainer.removeAllViews();
//    final View addLayerView = mInflater.inflate(R.layout.info_box_additional_layer, this, false);
//    mPlacePageContainer.addView(addLayerView);
  }

  private void fillPlacePageApi(MapObject mo)
  {
//    mPlacePageContainer.removeAllViews();
//    final View apiView = mInflater.inflate(R.layout.info_box_api, this, false);
//    mPlacePageContainer.addView(apiView);
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

      if (mBookmarkManager.getCategoriesCount() <= bmk.getCategoryId())
        deleted = true;
      else if (mBookmarkManager.getCategoryById(bmk.getCategoryId()).getBookmarksCount() <= bmk.getBookmarkId())
        deleted = true;
      else if (mBookmarkManager.getBookmark(bmk.getCategoryId(), bmk.getBookmarkId()).getLat() != bmk.getLat())
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
        final Bookmark updatedBmk = mBookmarkManager.getBookmark(bmk.getCategoryId(), bmk.getBookmarkId());
        setMapObject(null);
        setMapObject(updatedBmk);
      }
    }
  }

  private void showColorChooser()
  {
    final IconsAdapter adapter = new IconsAdapter(getContext(), mIcons);
    final Icon icon = ((Bookmark) mMapObject).getIcon();
    adapter.chooseItem(mIcons.indexOf(icon));

    final ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
    final int padSide = (int) getResources().getDimension(R.dimen.dp_x_8);
    final int padTopB = (int) getResources().getDimension(R.dimen.dp_x_6);

    final GridView gView = new GridView(getContext());
    gView.setAdapter(adapter);
    gView.setNumColumns(COLOR_CHOOSER_COLUMN_NUM);
    gView.setGravity(Gravity.CENTER);
    gView.setPadding(padSide, padTopB, padSide, padTopB);
    gView.setLayoutParams(params);
    gView.setSelector(new ColorDrawable(Color.TRANSPARENT));

    final Dialog dialog = new AlertDialog.Builder(getContext())
        .setTitle(R.string.bookmark_color)
        .setView(gView)
        .create();

    gView.setOnItemClickListener(new AdapterView.OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> arg0, View who, int pos, long id)
      {
        Icon icon = mIcons.get(pos);
        Bookmark bmk = (Bookmark) mMapObject;
        bmk.setParams(bmk.getName(), icon, bmk.getBookmarkDescription());
        bmk = mBookmarkManager.getBookmark(bmk.getCategoryId(), bmk.getBookmarkId());
        setMapObject(bmk);
        dialog.dismiss();
      }
    });

    dialog.show();
  }

  private void updateColorChooser(Icon icon)
  {
    final Icon oldIcon = ((Bookmark) mMapObject).getIcon();
    final String from = oldIcon.getName();
    final String to = icon.getName();
    if (!TextUtils.equals(from, to))
      Statistics.INSTANCE.trackColorChanged(from, to);
    mIvColor.setImageDrawable(UiUtils
        .drawCircleForPin(to, (int) getResources().getDimension(R.dimen.color_chooser_radius), getResources()));
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.color_image:
      showColorChooser();
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

        mBookmarkManager.deleteBookmark((Bookmark) mMapObject);
        setMapObject(p);
      }
      else
      {
        mBookmarkedMapObject = mMapObject;
        final Bookmark newBmk = mBookmarkManager.getBookmark(mBookmarkManager.addNewBookmark(
            mMapObject.getName(), mMapObject.getLat(), mMapObject.getLon()));
        setMapObject(newBmk);
      }
      Framework.invalidate();
      break;
    case R.id.info_box:
      // TODO
      // without listening this event some bug may appear.
      // check that and remove after investigation
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
    default:
      break;
    }
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
}
