package com.mapswithme.maps.widget;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v7.widget.GridLayout;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.bookmarks.BookmarkActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.MapObjectType;
import com.mapswithme.maps.bookmarks.data.MapObject.Poi;
import com.mapswithme.maps.bookmarks.data.MapObject.SearchResult;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

public class MapInfoView extends LinearLayout
{
  private final Logger mLog = SimpleLogger.get("MwmMapInfoView");

  public interface OnVisibilityChangedListener
  {
    public void onHeadVisibilityChanged(boolean isVisible);
    public void onBodyVisibilityChanged(boolean isVisible);
  }

  private OnVisibilityChangedListener mVisibilityChangedListener;

  private boolean mIsHeaderVisible = true;
  private boolean mIsBodyVisible   = true;

  private final ViewGroup mHeaderGroup;
  private final ViewGroup mBodyGroup;
  private final ScrollView mBodyContainer;
  private final View mView;

  // Header
  private final TextView mTitle;;
  private final TextView mSubtitle;
  private final CheckBox mIsBookmarked;

  // Data
  private MapObject mMapObject;

  // Views
  private final LayoutInflater mInflater;

  // Body
  private final View mShareView;

  // Gestures
  private final GestureDetectorCompat mGestureDetector;
  private final OnGestureListener mGestureListener = new GestureDetector.SimpleOnGestureListener()
  {
    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
    {
      final boolean isVertical = Math.abs(distanceY) > 2*Math.abs(distanceX);
      final boolean isInRange  = Math.abs(distanceY) > 1 && Math.abs(distanceY) < 100;

      if (isVertical && isInRange)
      {

        if (distanceY < 0) // sroll down, hide
          showBody(false);
        else               // sroll up, show
          showBody(true);

        return true;
      }

      return false;
    };
  };


  private int mMaxBodyHeight = 0;
  private final OnLayoutChangeListener mLayoutChangeListener = new OnLayoutChangeListener()
  {

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight,
        int oldBottom)
    {
      calculateMaxBodyHeight(top, bottom);
    }
  };

  // We dot want to use OnCheckedChangedListener because it gets called
  // if someone calls setCheched() from code. We need only user interaction.
  private final OnClickListener mIsBookmarkedClickListener = new OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      if (v.getId() != R.id.info_box_is_bookmarked)
        throw new IllegalStateException("This listener is only for is_bookmarkded checkbox.");

      final BookmarkManager bm = BookmarkManager.getBookmarkManager();
      if (mMapObject.getType() == MapObjectType.BOOKMARK)
      {
        // Make Poi from bookmark
        final Poi p = new Poi(
            mMapObject.getName(),
            mMapObject.getLat(),
            mMapObject.getLon(),
            null); // we dont know what type it was

        // remove from bookmarks
        bm.deleteBookmark((Bookmark) mMapObject);
        setMapObject(p);
      }
      else
      {
        // Add to bookmarks
        final Bookmark newbmk = bm.getBookmark(
            bm.addNewBookmark(
              mMapObject.getName(),
              mMapObject.getLat(),
              mMapObject.getLon()));

        setMapObject(newbmk);
      }
      Framework.invalidate();
    }
  };

  public MapInfoView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs);

    mInflater =   (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    mView = mInflater.inflate(R.layout.info_box, this, true);

    mHeaderGroup = (ViewGroup) mView.findViewById(R.id.header);
    mBodyGroup = (ViewGroup) mView.findViewById(R.id.body);

    showBody(false);
    showHeader(false);

    // Header
    mTitle = (TextView) mHeaderGroup.findViewById(R.id.info_title);
    mSubtitle = (TextView) mHeaderGroup.findViewById(R.id.info_subtitle);
    mIsBookmarked = (CheckBox) mHeaderGroup.findViewById(R.id.info_box_is_bookmarked);
    mIsBookmarked.setOnClickListener(mIsBookmarkedClickListener);

    // Body
    mBodyContainer = (ScrollView) mBodyGroup.findViewById(R.id.body_container);
    mShareView = mBodyGroup.findViewById(R.id.info_box_share);
    mShareView.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        ShareAction.getAnyShare().shareMapObject((Activity) getContext(), mMapObject);
      }
    });


    // Gestures
    mGestureDetector = new GestureDetectorCompat(getContext(), mGestureListener);
    mView.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        // Friggin system does not work without this stub.
      }
    });
  }

  public MapInfoView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public MapInfoView(Context context)
  {
    this(context, null, 0);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event)
  {
    mGestureDetector.onTouchEvent(event);
    return super.onTouchEvent(event);
  }

  @Override
  protected void onAttachedToWindow()
  {
    super.onAttachedToWindow();
    ((View)getParent()).addOnLayoutChangeListener(mLayoutChangeListener);
  }

  public void showBody(final boolean show)
  {
    if (mIsBodyVisible == show)
      return; // if state is already same as we need

    final long duration = 200;

    // Calculate translate offset
    final int headHeight  = mHeaderGroup.getHeight();
    final int totalHeight = headHeight + mMaxBodyHeight;
    final float offset = headHeight/(float)totalHeight;

    if (show) // slide up
    {
      final TranslateAnimation slideUp = new TranslateAnimation(
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 1 - offset,
          Animation.RELATIVE_TO_SELF, 0);

      slideUp.setDuration(duration);
      UiUtils.show(mBodyGroup);
      mView.startAnimation(slideUp);

      if (mVisibilityChangedListener != null)
        mVisibilityChangedListener.onBodyVisibilityChanged(show);
    }
    else     // slide down
    {
      final TranslateAnimation slideDown = new TranslateAnimation(
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 1 - offset);

      slideDown.setDuration(duration);
      slideDown.setAnimationListener(new UiUtils.SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          UiUtils.hide(mBodyGroup);

          if (mVisibilityChangedListener != null)
            mVisibilityChangedListener.onBodyVisibilityChanged(show);
        }
      });


      mView.startAnimation(slideDown);
    }

    mIsBodyVisible = show;
  }

  public void showHeader(boolean show)
  {
    if (mIsHeaderVisible == show)
      return;

    UiUtils.hideIf(!show, mHeaderGroup);
    mIsHeaderVisible = show;

    if (mVisibilityChangedListener != null)
      mVisibilityChangedListener.onHeadVisibilityChanged(show);
  }

  private void setTextAndShow(final CharSequence title, final CharSequence subtitle)
  {

    mTitle.setText(title);
    mSubtitle.setText(subtitle);

    showHeader(true);
  }

  public boolean hasThatObject(MapObject mo)
  {
    return MapObject.checkSum(mo).equals(MapObject.checkSum(mMapObject));
  }

  public void setMapObject(MapObject mo)
  {
    if (!hasThatObject(mo))
    {
      if (mo != null)
      {
        mMapObject = mo;

        final String undefined = getResources().getString(R.string.placepage_unsorted);
        final String name = Utils.firstNotEmpty(mo.getName(), mo.getPoiTypeName(), undefined);
        final String type = Utils.firstNotEmpty(mo.getPoiTypeName(), undefined);
        setTextAndShow(name, type);

        switch (mo.getType())
        {
          case POI:
            mIsBookmarked.setChecked(false);
            setBodyForPOI((Poi)mo);
            break;
          case BOOKMARK:
            mIsBookmarked.setChecked(true);
            setBodyForBookmark((Bookmark)mo);
            break;
          case ADDITIONAL_LAYER:
            mIsBookmarked.setChecked(false);
            setBodyForAdditionalLayer((SearchResult)mo);
            break;
          case API_POINT:
            mIsBookmarked.setChecked(false);
            setBodyForAPI(mo);
            break;
          default:
            throw new IllegalArgumentException("Unknown MapObject type:" + mo.getType());
        }
        setUpGeoInformation(mo);
      }
      else
      {
        mMapObject = mo;
      }
    }
  }

  private void setUpGeoInformation(MapObject mo)
  {
    final GridLayout mGeoLayout = (GridLayout) mBodyContainer.findViewById(R.id.info_box_geo_ref);
    final Location lastKnown = MWMApplication.get().getLocationService().getLastKnown();

    final CharSequence distanceText = (lastKnown == null)
        ? getResources().getString(R.string.unknown_current_position)
        : Framework.getDistanceAndAzimutFromLatLon(
            mo.getLat(), mo.getLon(), lastKnown.getLatitude(),
            lastKnown.getLongitude(), 0.0).getDistance();

    UiUtils.findViewSetText(mGeoLayout, R.id.info_box_geo_distance, distanceText);
    UiUtils.findViewSetText(mGeoLayout, R.id.info_box_geo_location, UiUtils.formatLatLon(mo.getLat(), mo.getLon()));
  }

  private void setBodyForPOI(Poi poi)
  {
    mBodyContainer.removeAllViews();
    final View poiView = mInflater.inflate(R.layout.info_box_poi, null);

    final TextView addressText = (TextView) poiView.findViewById(R.id.info_box_address);
    addressText.setText(Framework.getNameAndAddress4Point(poi.getLat(), poi.getLon()));

    mBodyContainer.addView(poiView);
  }

  private void setBodyForBookmark(final Bookmark bmk)
  {
    mBodyContainer.removeAllViews();
    final View bmkView = mInflater.inflate(R.layout.info_box_bookmark, null);

    final TextView addressText = (TextView) bmkView.findViewById(R.id.info_box_address);
    addressText.setText(Framework.getNameAndAddress4Point(bmk.getLat(), bmk.getLon()));

    // Set category, pin color, set click listener TODO set size
    final Drawable pinColorDrawable = UiUtils.drawCircleForPin(bmk.getIcon().getName(), 50, getResources());
    UiUtils.findImageViewSetDrawable(bmkView, R.id.info_box_bmk_pincolor, pinColorDrawable);
    UiUtils.findViewSetText(bmkView, R.id.info_box_bmk_category, bmk.getCategoryName(getContext()));
    UiUtils.findViewSetOnClickListener(bmkView, R.id.info_box_bmk_edit, new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        BookmarkActivity.startWithBookmark(getContext(), bmk.getCategoryId(), bmk.getBookmarkId());
      }
    });


    // Description of BMK
    final WebView descritionWv = (WebView)bmkView.findViewById(R.id.info_box_bookmark_descr);
    final String descriptionTxt = bmk.getBookmarkDescription();

    if (TextUtils.isEmpty(descriptionTxt))
      UiUtils.hide(descritionWv);
    else
    {
      descritionWv.loadData(descriptionTxt, "text/html; charset=UTF-8", null);
      descritionWv.setBackgroundColor(Color.TRANSPARENT);
      UiUtils.show(descritionWv);
    }

    mBodyContainer.addView(bmkView);
  }

  private void setBodyForAdditionalLayer(SearchResult sr)
  {
    mBodyContainer.removeAllViews();

    final View addLayerView = mInflater.inflate(R.layout.info_box_additional_layer, null);

    final TextView addressText = (TextView) addLayerView.findViewById(R.id.info_box_address);
    addressText.setText(Framework.getNameAndAddress4Point(sr.getLat(), sr.getLon()));

    mBodyContainer.addView(addLayerView);
  }


  private void setBodyForAPI(MapObject mo)
  {
    mBodyContainer.removeAllViews();

    final View apiView = mInflater.inflate(R.layout.info_box_api, null);

    final TextView addressText = (TextView) apiView.findViewById(R.id.info_box_address);
    addressText.setText(Framework.getNameAndAddress4Point(mo.getLat(), mo.getLon()));

    final Button backToCallerBtn = (Button)apiView.findViewById(R.id.info_box_api_return_to_caller);

    final ParsedMmwRequest r = ParsedMmwRequest.getCurrentRequest();
    // Text
    final String btnText = Utils.firstNotEmpty(r.getCustomButtonName(),
        getResources().getString(R.string.more_info));
    backToCallerBtn.setText(btnText);

    // Icon
    final Drawable icon = UiUtils.setCompoundDrawableBounds(r.getIcon(getContext()), R.dimen.dp_x_8, getResources());
    backToCallerBtn.setCompoundDrawables(icon, null, null, null);

    // Return callback
    backToCallerBtn.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        ParsedMmwRequest.getCurrentRequest().sendResponseAndFinish((Activity) getContext(), true);
      }
    });

    mBodyContainer.addView(apiView);
  }

  public void setOnVisibilityChangedListener(OnVisibilityChangedListener listener)
  {
    mVisibilityChangedListener = listener;
  }

  private void calculateMaxBodyHeight(int top, int bottom)
  {
    mMaxBodyHeight = (bottom - top)/2;
    final ViewGroup.LayoutParams lp = mBodyGroup.getLayoutParams();
    lp.height = mMaxBodyHeight;
    mBodyGroup.setLayoutParams(lp);
    mLog.d("Max height: " + mMaxBodyHeight);
  }

  public void onResume()
  {
    if (mMapObject == null)
      return;

    // We need to check, if content of body is still valid
    if (mMapObject.getType() == MapObjectType.BOOKMARK)
    {
      final Bookmark bmk = (Bookmark)mMapObject;
      final BookmarkManager bm = BookmarkManager.getBookmarkManager();

      // Was it deleted?
      boolean deleted = false;

      if (bm.getCategoriesCount() <= bmk.getCategoryId())
        deleted = true;
      else if (bm.getCategoryById(bmk.getCategoryId()).getBookmarksCount() <= bmk.getBookmarkId())
        deleted = true;
      else if (bm.getBookmark(bmk.getCategoryId(), bmk.getBookmarkId()).getLat() != bmk.getLat())
        deleted = true;
      // We can do check above, because lat/lon cannot be changed from edit screen.

      if (deleted)
      {
     // Make Poi from bookmark
        final Poi p = new Poi(
            mMapObject.getName(),
            mMapObject.getLat(),
            mMapObject.getLon(),
            null); // we dont know what type it was

        // remove from bookmarks
        bm.deleteBookmark((Bookmark) mMapObject);
        setMapObject(p);
        // TODO how to handle the case, when bookmark moved to another group?
     }
      else
      {
        // Update data for current bookmark
        final Bookmark updatedBmk = bm.getBookmark(bmk.getCategoryId(), bmk.getBookmarkId());
        setMapObject(null);
        setMapObject(updatedBmk);
      }

    }
  }
}
