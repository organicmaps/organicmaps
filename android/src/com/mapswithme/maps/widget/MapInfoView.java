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
import android.view.GestureDetector.OnGestureListener;
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
  private final ViewGroup mHeaderGroup;
  private final ViewGroup mBodyGroup;
  private final ScrollView mBodyContainer;
  private final View mView;
  // Header
  private final TextView mTitle;
  private final TextView mSubtitle;
  private final CheckBox mIsBookmarked;
  // Views
  private final LayoutInflater mInflater;
  // Gestures
  private final GestureDetectorCompat mGestureDetector;
  private OnVisibilityChangedListener mVisibilityChangedListener;
  private boolean mIsHeaderVisible = true;
  private boolean mIsBodyVisible = true;
  private State mCurrentState = State.COLLAPSED;
  // Data
  private MapObject mMapObject;
  private int mMaxBodyHeight = 0;
  private LinearLayout mGeoLayout = null;
  private View mDistanceView;
  private TextView mDistanceText;

  public MapInfoView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs);

    mInflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    mView = mInflater.inflate(R.layout.info_box, this, true);

    mHeaderGroup = (ViewGroup) mView.findViewById(R.id.header);
    mBodyGroup = (ViewGroup) mView.findViewById(R.id.body);

    showBody(false);
    showHeader(false);

    // Header
    mTitle = (TextView) mHeaderGroup.findViewById(R.id.info_title);
    mSubtitle = (TextView) mHeaderGroup.findViewById(R.id.info_subtitle);
    mIsBookmarked = (CheckBox) mHeaderGroup.findViewById(R.id.info_box_is_bookmarked);

    // We don't want to use OnCheckedChangedListener because it gets called
    // if someone calls setChecked() from code. We need only user interaction.
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
          final Bookmark newBookmark = bm.getBookmark(bm.addNewBookmark(mMapObject.getName(), mMapObject.getLat(), mMapObject.getLon()));
          setMapObject(newBookmark);
        }
        Framework.invalidate();
      }
    });

    // Body
    mBodyContainer = (ScrollView) mBodyGroup.findViewById(R.id.body_container);

    // Gestures
    OnGestureListener mGestureListener = new GestureDetector.SimpleOnGestureListener()
    {
      @Override
      public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
      {
        final boolean isVertical = Math.abs(distanceY) > 2 * Math.abs(distanceX);
        final boolean isInRange = Math.abs(distanceY) > 1 && Math.abs(distanceY) < 100;

        if (isVertical && isInRange)
        {

          if (distanceY < 0)
            setState(State.HEAD);
          else
            setState(State.FULL);

          return true;
        }
        return false;
      }

      @Override
      public boolean onSingleTapConfirmed(MotionEvent e)
      {
        if (mCurrentState == State.FULL)
          setState(State.HEAD);
        else
          setState(State.FULL);

        return true;
      }
    };
    mGestureDetector = new GestureDetectorCompat(getContext(), mGestureListener);
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
    calculateMaxBodyHeight();
  }

  private void showBody(final boolean show)
  {
    calculateMaxBodyHeight();

    if (mIsBodyVisible == show)
    {
      return; // if state is already same as we need
    }

    final long duration = 400;

    // Calculate translate offset
    final int headHeight = mHeaderGroup.getHeight();
    final int totalHeight = headHeight + mMaxBodyHeight;
    final float offset = headHeight / (float) totalHeight;

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
      {
        mVisibilityChangedListener.onPPVisibilityChanged(show);
      }
    }
    else // slide down
    {
      final TranslateAnimation slideDown = new TranslateAnimation(
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 1 - offset);

      slideDown.setDuration(duration);
      slideDown.setFillEnabled(true);
      slideDown.setFillBefore(true);
      slideDown.setAnimationListener(new UiUtils.SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          UiUtils.hide(mBodyGroup);

          if (mVisibilityChangedListener != null)
          {
            mVisibilityChangedListener.onPPVisibilityChanged(show);
          }
        }
      });

      mView.startAnimation(slideDown);
    }

    mIsBodyVisible = show;
  }

  private void showHeader(final boolean show)
  {
    if (mIsHeaderVisible == show)
    {
      return;
    }

    final int duration = 200;

    if (show)
    {
      final TranslateAnimation slideUp = new TranslateAnimation(
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 1,
          Animation.RELATIVE_TO_SELF, 0);
      slideUp.setDuration(duration);

      UiUtils.show(mHeaderGroup);
      slideUp.setAnimationListener(new SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          if (mVisibilityChangedListener != null)
          {
            mVisibilityChangedListener.onPPPVisibilityChanged(show);
          }
        }
      });

      mHeaderGroup.startAnimation(slideUp);
    }
    else
    {
      final TranslateAnimation slideDown = new TranslateAnimation(
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 1);
      slideDown.setDuration(duration);

      slideDown.setAnimationListener(new SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          UiUtils.hide(mHeaderGroup);
          if (mVisibilityChangedListener != null)
            mVisibilityChangedListener.onPPPVisibilityChanged(show);
        }
      });

      mHeaderGroup.startAnimation(slideDown);
    }

    mIsHeaderVisible = show;
  }

  private void setTextAndShow(final CharSequence title, final CharSequence subtitle)
  {
    mTitle.setText(title);
    mSubtitle.setText(subtitle);

    showHeader(true);
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
      if (mCurrentState == State.COLLAPSED && state == State.HEAD)
        showHeader(true);
      else if (mCurrentState == State.HEAD && state == State.FULL)
        showBody(true);
      else if (mCurrentState == State.HEAD && state == State.COLLAPSED)
        showHeader(false);
      else if (mCurrentState == State.FULL && state == State.HEAD)
        showBody(false);
      else if (mCurrentState == State.FULL && state == State.COLLAPSED)
        slideEverythingDown();
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
      if (mo != null)
      {
        mo.setDefaultIfEmpty(getResources());

        setTextAndShow(mo.getName(), mo.getPoiTypeName());

        mMapObject = mo;

        boolean isChecked = false;
        switch (mo.getType())
        {
        case POI:
          setBodyForPOI(mo);
          break;
        case BOOKMARK:
          isChecked = true;
          setBodyForBookmark((Bookmark) mo);
          break;
        case ADDITIONAL_LAYER:
          setBodyForAdditionalLayer((SearchResult) mo);
          break;
        case API_POINT:
          setBodyForAPI(mo);
          break;
        default:
          throw new IllegalArgumentException("Unknown MapObject type:" + mo.getType());
        }

        mIsBookmarked.setChecked(isChecked);

        setUpAddressBox();
        setUpGeoInformation();
        setUpBottomButtons();
      }
      else
      {
        mMapObject = mo;
      }
    }

    // Sometimes we have to force update view
    invalidate();
    requestLayout();
  }

  private void setUpBottomButtons()
  {
    View mShareView = mBodyGroup.findViewById(R.id.info_box_share);
    mShareView.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        ShareAction.getAnyShare().shareMapObject((Activity) getContext(), mMapObject);
      }
    });

    View mEditBtn = mBodyGroup.findViewById(R.id.info_box_edit);
    TextView mReturnToCallerBtn = (TextView) mBodyGroup.findViewById(R.id.info_box_back_to_caller);
    UiUtils.hide(mEditBtn);
    UiUtils.hide(mReturnToCallerBtn);

    if (mMapObject.getType() == MapObjectType.BOOKMARK)
    {
      // BOOKMARK
      final Bookmark bmk = (Bookmark) mMapObject;
      mEditBtn.setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          BookmarkActivity.startWithBookmark(getContext(), bmk.getCategoryId(), bmk.getBookmarkId());
        }
      });

      UiUtils.show(mEditBtn);
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
      final int spanSize = (int) (25 * getResources().getDisplayMetrics().density);
      icon.setBounds(0, 0, spanSize, spanSize);
      final ImageSpan callerIconSpan = new ImageSpan(icon, ImageSpan.ALIGN_BOTTOM);

      final SpannableString ss = new SpannableString(txtPattern);
      ss.setSpan(callerIconSpan, spanStart, spanEnd, Spannable.SPAN_INCLUSIVE_EXCLUSIVE);

      UiUtils.show(mReturnToCallerBtn);
      mReturnToCallerBtn.setText(ss, BufferType.SPANNABLE);
      mReturnToCallerBtn.setOnClickListener(new OnClickListener()
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
    mGeoLayout = (LinearLayout) mBodyContainer.findViewById(R.id.info_box_geo_ref);
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

  private void setBodyForPOI(MapObject poi)
  {
    mBodyContainer.removeAllViews();
    final View poiView = mInflater.inflate(R.layout.info_box_poi, null);
    mBodyContainer.addView(poiView);
  }

  private void setUpAddressBox()
  {
    final TextView addressText = (TextView) mBodyGroup.findViewById(R.id.info_box_address);
    addressText.setText(Framework.getNameAndAddress4Point(mMapObject.getLat(), mMapObject.getLon()));
  }

  private void setBodyForBookmark(final Bookmark bmk)
  {
    mBodyContainer.removeAllViews();
    final View bmkView = mInflater.inflate(R.layout.info_box_bookmark, null);

    // Description of BMK
    final WebView descriptionWv = (WebView) bmkView.findViewById(R.id.info_box_bookmark_descr);
    final String descriptionTxt = bmk.getBookmarkDescription();

    if (TextUtils.isEmpty(descriptionTxt))
    {
      UiUtils.hide(descriptionWv);
    }
    else
    {
      descriptionWv.loadData(descriptionTxt, "text/html; charset=UTF-8", null);
      descriptionWv.setBackgroundColor(Color.TRANSPARENT);
      UiUtils.show(descriptionWv);
    }

    mBodyContainer.addView(bmkView);
  }

  private void setBodyForAdditionalLayer(SearchResult sr)
  {
    mBodyContainer.removeAllViews();
    final View addLayerView = mInflater.inflate(R.layout.info_box_additional_layer, null);
    mBodyContainer.addView(addLayerView);
  }

  private void setBodyForAPI(MapObject mo)
  {
    mBodyContainer.removeAllViews();
    final View apiView = mInflater.inflate(R.layout.info_box_api, null);
    mBodyContainer.addView(apiView);
  }

  public void setOnVisibilityChangedListener(OnVisibilityChangedListener listener)
  {
    mVisibilityChangedListener = listener;
  }

  private void calculateMaxBodyHeight()
  {
    final View parent = (View) getParent();
    if (parent != null)
    {
      mMaxBodyHeight = parent.getHeight() / 2;
      final ViewGroup.LayoutParams lp = mBodyGroup.getLayoutParams();
      if (lp != null)
      {
        lp.height = mMaxBodyHeight;
        mBodyGroup.setLayoutParams(lp);
      }
    }
    requestLayout();
    invalidate();
  }

  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b)
  {
    super.onLayout(changed, l, t, r, b);
    calculateMaxBodyHeight();
  }

  public void onResume()
  {
    if (mMapObject == null)
    {
      return;
    }

    checkBookmarkWasDeleted();
    checkApiWasCanceled();
  }

  private void checkApiWasCanceled()
  {
    if ((mMapObject.getType() == MapObjectType.API_POINT) && !ParsedMmwRequest.hasRequest())
    {
      setMapObject(null);

      showBody(false);
      showHeader(false);
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

  private void slideEverythingDown()
  {
    final TranslateAnimation slideDown = new TranslateAnimation(
        Animation.RELATIVE_TO_SELF, 0,
        Animation.RELATIVE_TO_SELF, 0,
        Animation.RELATIVE_TO_SELF, 0,
        Animation.RELATIVE_TO_SELF, 1);
    slideDown.setDuration(400);

    slideDown.setAnimationListener(new SimpleAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animation animation)
      {
        mIsBodyVisible = false;
        mIsHeaderVisible = false;

        UiUtils.hide(mHeaderGroup);
        UiUtils.hide(mBodyGroup);
        if (mVisibilityChangedListener != null)
        {
          mVisibilityChangedListener.onPPPVisibilityChanged(false);
          mVisibilityChangedListener.onPPVisibilityChanged(false);
        }
      }
    });

    mView.startAnimation(slideDown);
  }

  public static enum State
  {
    COLLAPSED,
    HEAD,
    FULL
  }

  public interface OnVisibilityChangedListener
  {
    public void onPPPVisibilityChanged(boolean isVisible);

    public void onPPVisibilityChanged(boolean isVisible);
  }
}
