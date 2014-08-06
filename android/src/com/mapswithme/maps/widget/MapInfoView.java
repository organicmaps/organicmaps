package com.mapswithme.maps.widget;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.os.Build;
import android.support.v4.view.GestureDetectorCompat;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.ImageSpan;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.webkit.WebView;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.GridView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.bookmarks.BookmarkActivity;
import com.mapswithme.maps.bookmarks.IconsAdapter;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.MapObjectType;
import com.mapswithme.maps.bookmarks.data.MapObject.Poi;
import com.mapswithme.maps.bookmarks.data.MapObject.SearchResult;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.UiUtils.SimpleAnimationListener;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;


public class MapInfoView extends LinearLayout implements View.OnClickListener
{
  private static final int SHORT_ANIM_DURATION = 200;
  private static final int LONG_ANIM_DURATION = 400;
  private static final float SPAN_SIZE = 25;
  private static final int COLOR_CHOOSER_COLUMN_NUM = 4;

  private final ViewGroup mPreviewGroup;
  private final ViewGroup mPlacePageGroup;
  private final ScrollView mPlacePageContainer;
  private final View mView;
  // Preview
  private final TextView mTitle;
  private final TextView mSubtitle;
  private final CheckBox mIsBookmarked;
  private final ImageButton mEditBtn;
  private final LayoutInflater mInflater;
  private final Button mRouteStartBtn;
  private final Button mRouteEndBtn;
  private final View mArrow;
  // Place page
  private RelativeLayout mGeoLayout;
  private TextView mTvLat;
  private TextView mTvLon;
  private ArrowView mAvDirection;
  private TextView mDistanceText;
  private ImageView mColorImage;
  // Gestures
  private GestureDetectorCompat mGestureDetector;
  // Data
  private MapObject mMapObject;
  private OnVisibilityChangedListener mVisibilityChangedListener;
  private boolean mIsPreviewVisible = true;
  private boolean mIsPlacePageVisible = true;
  private State mCurrentState = State.HIDDEN;
  private BookmarkManager mBookmarkManager;
  private List<Icon> mIcons;
  private MapObject mBookmarkedMapObject;

  private boolean mIsLatLonDms;
  private static final String PREF_USE_DMS = "use_dms";

  private OnLongClickListener mLatLonLongClickListener = new OnLongClickListener()
  {
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
  };

  public MapInfoView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs);

    mIsLatLonDms = context.getSharedPreferences(context.getString(R.string.pref_file_name),
        Context.MODE_PRIVATE).getBoolean(PREF_USE_DMS, false);

    mInflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    mView = mInflater.inflate(R.layout.info_box, this, true);

    mPreviewGroup = (ViewGroup) mView.findViewById(R.id.preview);
    mPreviewGroup.bringToFront();
    mPlacePageGroup = (ViewGroup) mView.findViewById(R.id.place_page);

    // Preview
    mTitle = (TextView) mPreviewGroup.findViewById(R.id.info_title);
    mSubtitle = (TextView) mPreviewGroup.findViewById(R.id.info_subtitle);
    mIsBookmarked = (CheckBox) mPreviewGroup.findViewById(R.id.info_box_is_bookmarked);
    mArrow = mPreviewGroup.findViewById(R.id.iv_arrow);
    // We don't want to use OnCheckedChangedListener because it gets called
    // if someone calls setChecked() from code. We need only user interaction.
    mIsBookmarked.setOnClickListener(this);
    mEditBtn = (ImageButton) mPreviewGroup.findViewById(R.id.btn_edit_title);
    mRouteStartBtn = (Button) mPreviewGroup.findViewById(R.id.btn_route_from);
    mRouteStartBtn.setOnClickListener(this);
    mRouteEndBtn = (Button) mPreviewGroup.findViewById(R.id.btn_route_to);
    mRouteEndBtn.setOnClickListener(this);

    // Place Page
    mPlacePageContainer = (ScrollView) mPlacePageGroup.findViewById(R.id.place_page_container);

    mBookmarkManager = BookmarkManager.getBookmarkManager();
    mIcons = mBookmarkManager.getIcons();

    showPlacePage(false);
    showPreview(false);

    initGestureDetector();
    mView.setOnClickListener(this);
  }

  public MapInfoView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public MapInfoView(Context context)
  {
    this(context, null, 0);
  }

  private void initGestureDetector()
  {
    // Gestures
    mGestureDetector = new GestureDetectorCompat(getContext(), new GestureDetector.SimpleOnGestureListener()
    {
      private static final int Y_MIN = 1;
      private static final int Y_MAX = 100;
      private static final int X_TO_Y_SCROLL_RATIO = 2;

      @Override
      public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
      {
        final boolean isVertical = Math.abs(distanceY) > X_TO_Y_SCROLL_RATIO * Math.abs(distanceX);
        final boolean isInRange = Math.abs(distanceY) > Y_MIN && Math.abs(distanceY) < Y_MAX;

        if (isVertical && isInRange)
        {
          if (distanceY < 0)
            setState(State.PREVIEW_ONLY);
          else
            setState(State.FULL_PLACEPAGE);

          return true;
        }
        return false;
      }

      @Override
      public boolean onSingleTapConfirmed(MotionEvent e)
      {
        if (mCurrentState == State.FULL_PLACEPAGE)
          setState(State.PREVIEW_ONLY);
        else
          setState(State.FULL_PLACEPAGE);

        return true;
      }
    });
  }

  @Override
  public boolean onTouchEvent(MotionEvent event)
  {
    mGestureDetector.onTouchEvent(event);
    return super.onTouchEvent(event);
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);
    calculateMaxPlacePageHeight();
  }

  private void showPlacePage(final boolean show)
  {
    calculateMaxPlacePageHeight();

    if (mIsPlacePageVisible == show)
      return; // if state is already same as we need

    TranslateAnimation slide;
    if (show) // slide up
    {
      slide = generateSlideAnimation(0, 0, -1, 0);
      slide.setDuration(SHORT_ANIM_DURATION);
      UiUtils.show(mPlacePageGroup);
      UiUtils.hide(mArrow);
      if (mVisibilityChangedListener != null)
        mVisibilityChangedListener.onPlacePageVisibilityChanged(show);
    }
    else // slide down
    {
      slide = generateSlideAnimation(0, 0, 0, -1);

      slide.setDuration(SHORT_ANIM_DURATION);
      slide.setFillEnabled(true);
      slide.setFillBefore(true);
      UiUtils.show(mArrow);
      slide.setAnimationListener(new UiUtils.SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          UiUtils.hide(mPlacePageGroup);

          if (mVisibilityChangedListener != null)
            mVisibilityChangedListener.onPlacePageVisibilityChanged(show);
        }
      });
    }
    mPlacePageGroup.startAnimation(slide);

    mIsPlacePageVisible = show;
  }

  private void showPreview(final boolean show)
  {
    if (mIsPreviewVisible == show)
      return;

    TranslateAnimation slide;
    if (show)
    {
      slide = generateSlideAnimation(0, 0, -1, 0);
      UiUtils.show(mPreviewGroup);
      slide.setAnimationListener(new SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          if (mVisibilityChangedListener != null)
            mVisibilityChangedListener.onPreviewVisibilityChanged(show);
        }
      });
    }
    else
    {
      slide = generateSlideAnimation(0, 0, 0, -1);
      slide.setAnimationListener(new SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          UiUtils.hide(mPreviewGroup);
          if (mVisibilityChangedListener != null)
            mVisibilityChangedListener.onPreviewVisibilityChanged(show);
        }
      });
    }
    slide.setDuration(SHORT_ANIM_DURATION);
    mPreviewGroup.startAnimation(slide);
    mIsPreviewVisible = show;
    UiUtils.show(mArrow);
  }

  private void hideEverything()
  {
    final TranslateAnimation slideDown = generateSlideAnimation(0, 0, 0, -1);
    slideDown.setDuration(LONG_ANIM_DURATION);

    slideDown.setAnimationListener(new SimpleAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animation animation)
      {
        mIsPlacePageVisible = false;
        mIsPreviewVisible = false;

        UiUtils.hide(mPreviewGroup);
        UiUtils.hide(mPlacePageGroup);
        if (mVisibilityChangedListener != null)
        {
          mVisibilityChangedListener.onPreviewVisibilityChanged(false);
          mVisibilityChangedListener.onPlacePageVisibilityChanged(false);
        }
      }
    });

    mView.startAnimation(slideDown);
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
        showPreview(true);
      else if (mCurrentState == State.PREVIEW_ONLY && state == State.FULL_PLACEPAGE)
        showPlacePage(true);
      else if (mCurrentState == State.PREVIEW_ONLY && state == State.HIDDEN)
        showPreview(false);
      else if (mCurrentState == State.FULL_PLACEPAGE && state == State.PREVIEW_ONLY)
        showPlacePage(false);
      else if (mCurrentState == State.FULL_PLACEPAGE && state == State.HIDDEN)
        hideEverything();
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

        mIsBookmarked.setChecked(isChecked);

        setUpPreview();
        setUpGeoInformation();
        setUpBottomButtons();
        setUpRoutingButtons();
      }
    }
  }

  private void setUpPreview()
  {
    mTitle.setText(mMapObject.getName());
    mSubtitle.setText(mMapObject.getPoiTypeName());

    if (mMapObject.getType() == MapObjectType.BOOKMARK)
    {
      mEditBtn.setOnClickListener(this);
      UiUtils.show(mEditBtn);
    }
    else
    {
      UiUtils.hide(mEditBtn);
    }

    showPreview(true);
  }

  private void setUpBottomButtons()
  {
    mPlacePageGroup.findViewById(R.id.info_box_share).setOnClickListener(this);

    final TextView returnToCallerTv = (TextView) mPlacePageGroup.findViewById(R.id.info_box_back_to_caller);
    UiUtils.hide(returnToCallerTv);

    if (mMapObject.getType() == MapObjectType.API_POINT)
    {
      // API
      final ParsedMmwRequest r = ParsedMmwRequest.getCurrentRequest();

      // Text and icon
      final String txtPattern = String
          .format("%s {icon} %s", getResources().getString(R.string.more_info), r.getCallerName(getContext()));

      final int spanStart = txtPattern.indexOf("{icon}");
      final int spanEnd = spanStart + "{icon}".length();

      final Drawable icon = r.getIcon(getContext());
      final int spanSize = (int) (SPAN_SIZE * getResources().getDisplayMetrics().density);
      icon.setBounds(0, 0, spanSize, spanSize);
      final ImageSpan callerIconSpan = new ImageSpan(icon, ImageSpan.ALIGN_BOTTOM);

      final SpannableString ss = new SpannableString(txtPattern);
      ss.setSpan(callerIconSpan, spanStart, spanEnd, Spannable.SPAN_INCLUSIVE_EXCLUSIVE);

      UiUtils.show(returnToCallerTv);
      returnToCallerTv.setText(ss, TextView.BufferType.SPANNABLE);
      returnToCallerTv.setOnClickListener(this);
    }
  }

  private void setUpGeoInformation()
  {
    mGeoLayout = (RelativeLayout) mPlacePageContainer.findViewById(R.id.info_box_geo_ref);
    mDistanceText = (TextView) mGeoLayout.findViewById(R.id.info_box_geo_distance);
    mAvDirection = (ArrowView) mGeoLayout.findViewById(R.id.av_direction);
    mAvDirection.setDrawCircle(true);
    mAvDirection.setVisibility(View.GONE); // should be hidden until first compass update
    mTvLat = (TextView) mGeoLayout.findViewById(R.id.info_box_lat);
    mTvLat.setOnClickListener(this);
    mTvLon = (TextView) mGeoLayout.findViewById(R.id.info_box_lon);
    mTvLon.setOnClickListener(this);

    final Location lastKnown = MWMApplication.get().getLocationService().getLastKnown();
    updateDistanceAndAzimut(lastKnown);

    updateCoords();
    // Context menu for the coordinates copying.
    if (Utils.apiEqualOrGreaterThan(Build.VERSION_CODES.HONEYCOMB))
    {
      mTvLat.setOnLongClickListener(mLatLonLongClickListener);
      mTvLon.setOnLongClickListener(mLatLonLongClickListener);
    }
  }

  private void setUpRoutingButtons()
  {
    if (Framework.nativeIsRoutingEnabled())
      UiUtils.show(mRouteStartBtn, mRouteEndBtn);
    else
      UiUtils.hide(mRouteStartBtn, mRouteEndBtn);
  }

  public void updateDistanceAndAzimut(Location l)
  {
    if (mGeoLayout != null && mMapObject != null)
    {
      if (mMapObject.getType() == MapObjectType.MY_POSITION)
      {
        final StringBuilder builder = new StringBuilder();
        if (l.hasAltitude())
          builder.append(Framework.nativeFormatAltitude(l.getAltitude()));
        if (l.hasSpeed())
          builder.append("   ").
              append(Framework.nativeFormatSpeed(l.getSpeed()));
        mSubtitle.setText(builder.toString());

        mAvDirection.setVisibility(View.GONE);
        mDistanceText.setVisibility(View.GONE);
      }
      else
      {
        final int visibility = l != null ? View.VISIBLE : View.GONE;
        mAvDirection.setVisibility(visibility);
        mDistanceText.setVisibility(visibility);
      }


      if (l != null)
      {
        final DistanceAndAzimut distanceAndAzimuth = Framework.nativeGetDistanceAndAzimutFromLatLon(mMapObject.getLat(),
            mMapObject.getLon(), l.getLatitude(), l.getLongitude(), 0.0);
        mDistanceText.setText(distanceAndAzimuth.getDistance());
      }
    }
  }

  private void updateCoords()
  {
    final double lat = mMapObject.getLat();
    final double lon = mMapObject.getLon();
    final String[] latLon = Framework.nativeFormatLatLonToArr(lat, lon, mIsLatLonDms);
    if (latLon.length == 2)
    {
      mTvLat.setText(latLon[0]);
      mTvLon.setText(latLon[1]);
    }
  }

  public void updateAzimuth(double northAzimuth)
  {
    if (mGeoLayout != null && mMapObject != null)
    {
      final Location l = MWMApplication.get().getLocationService().getLastKnown();
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
    mPlacePageContainer.removeAllViews();
    final View poiView = mInflater.inflate(R.layout.info_box_poi, this, false);
    mPlacePageContainer.addView(poiView);
  }

  private void fillPlacePageBookmark(final Bookmark bmk)
  {
    mPlacePageContainer.removeAllViews();
    final View bmkView = mInflater.inflate(R.layout.info_box_bookmark, this, false);
    bmkView.setOnClickListener(this);

    // Description of BMK
    final WebView descritionWv = (WebView) bmkView.findViewById(R.id.info_box_bookmark_descr);
    final String descriptionTxt = bmk.getBookmarkDescription();

    if (TextUtils.isEmpty(descriptionTxt))
      UiUtils.hide(descritionWv);
    else
    {
      descritionWv.loadData(descriptionTxt, "text/html; charset=UTF-8", null);
      descritionWv.setBackgroundColor(Color.TRANSPARENT);
      UiUtils.show(descritionWv);
    }

    mColorImage = (ImageView) bmkView.findViewById(R.id.color_image);
    mColorImage.setOnClickListener(this);
    mColorImage.setVisibility(View.VISIBLE);
    updateColorChooser(bmk.getIcon());

    mPlacePageContainer.addView(bmkView);
  }

  private void fillPlacePageLayer(SearchResult sr)
  {
    mPlacePageContainer.removeAllViews();
    final View addLayerView = mInflater.inflate(R.layout.info_box_additional_layer, this, false);
    mPlacePageContainer.addView(addLayerView);
  }

  private void fillPlacePageApi(MapObject mo)
  {
    mPlacePageContainer.removeAllViews();
    final View apiView = mInflater.inflate(R.layout.info_box_api, this, false);
    mPlacePageContainer.addView(apiView);
  }

  public void setOnVisibilityChangedListener(OnVisibilityChangedListener listener)
  {
    mVisibilityChangedListener = listener;
  }

  private void calculateMaxPlacePageHeight()
  {
    final View parent = (View) getParent();
    if (parent != null)
    {
      int maxPlacePageHeight = parent.getHeight() / 2;
      final ViewGroup.LayoutParams lp = mPlacePageGroup.getLayoutParams();
      if (lp != null && lp.height > maxPlacePageHeight)
      {
        lp.height = maxPlacePageHeight;
        mPlacePageGroup.setLayoutParams(lp);
        requestLayout();
        invalidate();
      }
    }
  }

  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b)
  {
    super.onLayout(changed, l, t, r, b);
    calculateMaxPlacePageHeight();
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

      showPlacePage(false);
      showPreview(false);
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
      Statistics.INSTANCE.trackColorChanged(getContext(), from, to);
    mColorImage.setImageDrawable(UiUtils
        .drawCircleForPin(to, (int) getResources().getDimension(R.dimen.color_chooser_radius), getResources()));
  }

  private TranslateAnimation generateSlideAnimation(float fromX, float toX, float fromY, float toY)
  {
    return new TranslateAnimation(
        Animation.RELATIVE_TO_SELF, fromX,
        Animation.RELATIVE_TO_SELF, toX,
        Animation.RELATIVE_TO_SELF, fromY,
        Animation.RELATIVE_TO_SELF, toY);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.color_image:
      showColorChooser();
      break;
    case R.id.info_box_is_bookmarked:
      if (!MWMApplication.get().hasBookmarks())
      {
        mIsBookmarked.setChecked(false);
        UiUtils.showBuyProDialog((Activity) getContext(), getResources().getString(R.string.bookmarks_in_pro_version));
        return;
      }
      if (mMapObject == null)
      {
        return;
      }
      if (mMapObject.getType() == MapObjectType.BOOKMARK)
      {
        MapObject p;
        if (mBookmarkedMapObject != null &&
            mBookmarkedMapObject.getLat() == mMapObject.getLat() &&
            mBookmarkedMapObject.getLon() == mMapObject.getLon()) // use cached POI of bookmark, if it corresponds to current object
        {
          p = mBookmarkedMapObject;
        }
        else
        {
          p = Framework.nativeGetMapObjectForPoint(mMapObject.getLat(), mMapObject.getLon());
        }

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
    case R.id.info_box_share:
      ShareAction.getAnyShare().shareMapObject((Activity) getContext(), mMapObject);
      break;
    case R.id.info_box_back_to_caller:
      ParsedMmwRequest.getCurrentRequest().sendResponseAndFinish((Activity) getContext(), true);
      break;
    case R.id.btn_edit_title:
      Bookmark bmk = (Bookmark) mMapObject;
      BookmarkActivity.startWithBookmark((Activity) getContext(), bmk.getCategoryId(), bmk.getBookmarkId());
      break;
    case R.id.btn_route_from:
      Framework.nativeSetRouteStart(mMapObject.getLat(), mMapObject.getLon());
      break;
    case R.id.btn_route_to:
      Framework.nativeSetRouteEnd(mMapObject.getLat(), mMapObject.getLon());
      break;
    case R.id.info_box_lat:
    case R.id.info_box_lon:
      mIsLatLonDms = !mIsLatLonDms;
      getContext().getSharedPreferences(getContext().getString(R.string.pref_file_name),
          Context.MODE_PRIVATE).edit().putBoolean(PREF_USE_DMS, mIsLatLonDms).commit();
      updateCoords();
      break;
    default:
      break;
    }
  }

  public static enum State
  {
    HIDDEN,
    PREVIEW_ONLY,
    FULL_PLACEPAGE
  }

  public interface OnVisibilityChangedListener
  {
    public void onPreviewVisibilityChanged(boolean isVisible);

    public void onPlacePageVisibilityChanged(boolean isVisible);
  }
}
