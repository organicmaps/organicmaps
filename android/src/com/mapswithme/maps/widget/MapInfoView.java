package com.mapswithme.maps.widget;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.support.v4.view.GestureDetectorCompat;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.ImageSpan;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.webkit.WebView;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.TextView.BufferType;

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
import com.mapswithme.util.UiUtils.SimpleAnimationListener;
import com.mapswithme.util.Utils;

public class MapInfoView extends LinearLayout
{
  private static final int SHORT_ANIM_DURATION = 200;
  private static final int LONG_ANIM_DURATION = 400;
  private static final float SPAN_SIZE = 25;

  private final ViewGroup mPreviewGroup;
  private final ViewGroup mPlacePageGroup;
  private final ScrollView mPlacePageContainer;
  private final View mView;
  // Preview
  private final TextView mTitle;
  private final TextView mSubtitle;
  private final CheckBox mIsBookmarked;
  private final LayoutInflater mInflater;
  // Gestures
  private final GestureDetectorCompat mGestureDetector;
  // Place page
  private LinearLayout mGeoLayout;
  private View mDistanceView;
  private TextView mDistanceText;
  // Data
  private MapObject mMapObject;
  private OnVisibilityChangedListener mVisibilityChangedListener;
  private boolean mIsPreviewVisible = true;
  private boolean mIsPlacePageVisible = true;
  private State mCurrentState = State.HIDDEN;
  private int mMaxPlacePageHeight = 0;

  public MapInfoView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs);

    mInflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    mView = mInflater.inflate(R.layout.info_box, this, true);

    mPreviewGroup = (ViewGroup) mView.findViewById(R.id.preview);
    mPreviewGroup.bringToFront();
    mPlacePageGroup = (ViewGroup) mView.findViewById(R.id.place_page);

    showPlacePage(false);
    showPreview(false);

    // Preview
    mTitle = (TextView) mPreviewGroup.findViewById(R.id.info_title);
    mSubtitle = (TextView) mPreviewGroup.findViewById(R.id.info_subtitle);
    mIsBookmarked = (CheckBox) mPreviewGroup.findViewById(R.id.info_box_is_bookmarked);

    // We don't want to use OnCheckedChangedListener because it gets called
    // if someone calls setCheched() from code. We need only user interaction.
    mIsBookmarked.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        final BookmarkManager bm = BookmarkManager.getBookmarkManager();

        if (mMapObject.getType() == MapObjectType.BOOKMARK)
        {
          final MapObject p = new Poi(mMapObject.getName(), mMapObject.getLat(), mMapObject.getLon(), null);
          bm.deleteBookmark((Bookmark) mMapObject);
          setMapObject(p);
        }
        else
        {
          final Bookmark newbmk = bm.getBookmark(bm.addNewBookmark(
              mMapObject.getName(), mMapObject.getLat(), mMapObject.getLon()));
          setMapObject(newbmk);
        }
        Framework.invalidate();
      }
    });

    // Place Page
    mPlacePageContainer = (ScrollView) mPlacePageGroup.findViewById(R.id.place_page_container);

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

    mView.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v) { /* Friggin system does not work without this stub.*/ }
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
      if (mVisibilityChangedListener != null)
        mVisibilityChangedListener.onPlacePageVisibilityChanged(show);
    }
    else // slide down
    {
      slide = generateSlideAnimation(0, 0, 0, -1);

      slide.setDuration(SHORT_ANIM_DURATION);
      slide.setFillEnabled(true);
      slide.setFillBefore(true);
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
      slide.setDuration(SHORT_ANIM_DURATION);
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
      slide.setDuration(SHORT_ANIM_DURATION);

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
    mPreviewGroup.startAnimation(slide);
    mIsPreviewVisible = show;
  }

  private void slideEverytingOut()
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

  private void setTextAndShow(final CharSequence title, final CharSequence subtitle)
  {
    mTitle.setText(title);
    mSubtitle.setText(subtitle);

    showPreview(true);
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
        slideEverytingOut();
      else
        throw new IllegalStateException(String.format("Invalid transition %s - > %s", mCurrentState, state));

      mCurrentState = state;
    }
  }

  public boolean hasThatObject(MapObject mo)
  {
    if (mo == null && mMapObject == null)
      return true;
    else if (mMapObject != null)
      return mMapObject.equals(mo);

    return false;
  }

  public void setMapObject(MapObject mo)
  {
    if (!hasThatObject(mo))
    {
      mMapObject = mo;
      if (mo != null)
      {
        mo.setDefaultIfEmpty(getResources());

        setTextAndShow(mo.getName(), mo.getPoiTypeName());

        boolean isChecked = false;
        switch (mo.getType())
        {
        case POI:
          fillPlacePagePoi(mo);
          break;
        case BOOKMARK:
          isChecked = true;
          fillPlacePageBookmark((Bookmark) mo);
          break;
        case ADDITIONAL_LAYER:
          fillPlacePageLayer((SearchResult) mo);
          break;
        case API_POINT:
          fillPlacePageApi(mo);
          break;
        default:
          throw new IllegalArgumentException("Unknown MapObject type:" + mo.getType());
        }

        mIsBookmarked.setChecked(isChecked);

        setUpAddressBox();
        setUpGeoInformation();
        setUpBottomButtons();
      }
    }

    // Sometimes we have to force update view
    invalidate();
    requestLayout();
  }

  private void setUpBottomButtons()
  {
    mPlacePageGroup.findViewById(R.id.info_box_share).setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        ShareAction.getAnyShare().shareMapObject((Activity) getContext(), mMapObject);
      }
    });

    final View editBtn = mPlacePageGroup.findViewById(R.id.info_box_edit);
    final TextView returnToCallerTv = (TextView) mPlacePageGroup.findViewById(R.id.info_box_back_to_caller);
    UiUtils.hide(editBtn);
    UiUtils.hide(returnToCallerTv);

    if (mMapObject.getType() == MapObjectType.BOOKMARK)
    {
      // BOOKMARK
      final Bookmark bmk = (Bookmark) mMapObject;
      editBtn.setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          BookmarkActivity.startWithBookmark(getContext(), bmk.getCategoryId(), bmk.getBookmarkId());
        }
      });

      UiUtils.show(editBtn);
    }
    else if (mMapObject.getType() == MapObjectType.API_POINT)
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
      returnToCallerTv.setText(ss, BufferType.SPANNABLE);
      returnToCallerTv.setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          ParsedMmwRequest.getCurrentRequest().sendResponseAndFinish((Activity) getContext(), true);
        }
      });
    }
  }

  private void setUpGeoInformation()
  {
    mGeoLayout = (LinearLayout) mPlacePageContainer.findViewById(R.id.info_box_geo_ref);
    mDistanceView = mGeoLayout.findViewById(R.id.info_box_geo_container_dist);
    mDistanceText = (TextView) mGeoLayout.findViewById(R.id.info_box_geo_distance);

    final Location lastKnown = MWMApplication.get().getLocationService().getLastKnown();
    updateDistance(lastKnown);

    final TextView coord = (TextView) mGeoLayout.findViewById(R.id.info_box_geo_location);

    final double lat = mMapObject.getLat();
    final double lon = mMapObject.getLon();
    coord.setText(UiUtils.formatLatLon(lat, lon));

    // Context menu for the coordinates copying.
    if (Utils.apiEqualOrGreaterThan(11))
    {
      coord.setOnLongClickListener(new OnLongClickListener()
      {
        @Override
        public boolean onLongClick(View v)
        {
          final PopupMenu popup = new PopupMenu(getContext(), v);
          final Menu menu = popup.getMenu();

          final String copyText = getResources().getString(android.R.string.copy);
          final String arrCoord[] = {
              UiUtils.formatLatLon(lat, lon),
              UiUtils.formatLatLonToDMS(lat, lon)};

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
      });
    }
  }

  public void updateDistance(Location l)
  {
    if (mGeoLayout != null && mMapObject != null)
    {
      mDistanceView.setVisibility(l != null ? View.VISIBLE : View.GONE);

      if (l != null)
      {
        final CharSequence distanceText = Framework.getDistanceAndAzimutFromLatLon(mMapObject.getLat(),
            mMapObject.getLon(), l.getLatitude(), l.getLongitude(), 0.0).getDistance();
        mDistanceText.setText(distanceText);
      }
    }
  }

  private void setUpAddressBox()
  {
    final TextView addressText = (TextView) mPlacePageGroup.findViewById(R.id.info_box_address);
    addressText.setText(Framework.getNameAndAddress4Point(mMapObject.getLat(), mMapObject.getLon()));
  }

  private void fillPlacePagePoi(MapObject poi)
  {
    mPlacePageContainer.removeAllViews();
    final View poiView = mInflater.inflate(R.layout.info_box_poi, null);
    mPlacePageContainer.addView(poiView);
  }

  private void fillPlacePageBookmark(final Bookmark bmk)
  {
    mPlacePageContainer.removeAllViews();
    final View bmkView = mInflater.inflate(R.layout.info_box_bookmark, null);

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

    mPlacePageContainer.addView(bmkView);
  }

  private void fillPlacePageLayer(SearchResult sr)
  {
    mPlacePageContainer.removeAllViews();
    final View addLayerView = mInflater.inflate(R.layout.info_box_additional_layer, null);
    mPlacePageContainer.addView(addLayerView);
  }

  private void fillPlacePageApi(MapObject mo)
  {
    mPlacePageContainer.removeAllViews();
    final View apiView = mInflater.inflate(R.layout.info_box_api, null);
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
      mMaxPlacePageHeight = parent.getHeight() / 2;
      final ViewGroup.LayoutParams lp = mPlacePageGroup.getLayoutParams();
      if (lp != null && lp.height != mMaxPlacePageHeight)
      {
        lp.height = mMaxPlacePageHeight;
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
      final BookmarkManager bm = BookmarkManager.getBookmarkManager();

      // Was bookmark deleted?
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
        final MapObject p = new Poi(mMapObject.getName(), mMapObject.getLat(), mMapObject.getLon(), null);
        setMapObject(p);
        // TODO how to handle the case, when bookmark was moved to another group?
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

  private TranslateAnimation generateSlideAnimation(float fromX, float toX, float fromY, float toY)
  {
    return new TranslateAnimation(
        Animation.RELATIVE_TO_SELF, fromX,
        Animation.RELATIVE_TO_SELF, toX,
        Animation.RELATIVE_TO_SELF, fromY,
        Animation.RELATIVE_TO_SELF, toY);
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
