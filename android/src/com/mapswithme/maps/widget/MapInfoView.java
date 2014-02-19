package com.mapswithme.maps.widget;

import android.content.Context;
import android.support.v4.view.GestureDetectorCompat;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.Poi;
import com.mapswithme.maps.bookmarks.data.MapObject.SearchResult;
import com.mapswithme.util.UiUtils;
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
  private boolean mIsVisible       = true;

  private final ViewGroup mHeaderGroup;
  private final ViewGroup mBodyGroup;
  private final View mView;

  // Header
  private final TextView mTitle;;
  private final TextView mSubtitle;

  // Data
  private MapObject mMapObject;

  // Gestures
  private final GestureDetectorCompat mGestureDetector;
  private final OnGestureListener mGestureListener = new GestureDetector.SimpleOnGestureListener()
  {
    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
    {
      Log.e("MwmGest", String.format("Scroll: dx=%f dy=%f", distanceX, distanceY));

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
      mMaxBodyHeight = (bottom - top)/2;
      final ViewGroup.LayoutParams lp = mBodyGroup.getLayoutParams();
      lp.height = mMaxBodyHeight;
      mBodyGroup.setLayoutParams(lp);
      mLog.d("Max height: " + mMaxBodyHeight);
    }
  };

  public MapInfoView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);

    final LayoutInflater li =   (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    mView = li.inflate(R.layout.info_box, this, true);

    mHeaderGroup = (ViewGroup) mView.findViewById(R.id.header);
    mBodyGroup = (ViewGroup) mView.findViewById(R.id.body);

    showBody(false);
    showHeader(false);
    show(false);

    // Header
    mTitle = (TextView) mHeaderGroup.findViewById(R.id.info_title);
    mSubtitle = (TextView) mHeaderGroup.findViewById(R.id.info_subtitle);

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

  @Override
  public boolean onTouchEvent(MotionEvent event)
  {
    mGestureDetector.onTouchEvent(event);
    return super.onTouchEvent(event);
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
  protected void onAttachedToWindow()
  {
    super.onAttachedToWindow();
    ((View)getParent()).addOnLayoutChangeListener(mLayoutChangeListener);
  }

  public void showBody(final boolean show)
  {
    if (mIsBodyVisible == show)
      return; // if state is already same as we need

    final long duration = 250;
    if (show) // slide up
    {
      final TranslateAnimation slideUp = new TranslateAnimation(
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 0,
          Animation.RELATIVE_TO_SELF, 1,
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
          Animation.RELATIVE_TO_SELF, 1);

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

  public void show(boolean show)
  {
    if (mIsVisible == show)
      return;

    UiUtils.hideIf(!show, mView);
    mIsVisible = show;
  }

  private void setTextAndShow(final CharSequence title, final CharSequence subtitle)
  {
    // We need 2 animations: hide, show, and one listener
    final long duration = 250;
    final Animation fadeOut = new AlphaAnimation(1, 0);
    fadeOut.setDuration(duration);
    final Animation fadeIn = new AlphaAnimation(0, 1);
    fadeIn.setDuration(duration);

    fadeOut.setAnimationListener(new UiUtils.SimpleAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animation animation)
      {
        mTitle.setText(title);
        mSubtitle.setText(subtitle);
        mHeaderGroup.startAnimation(fadeIn);
      };
    });

    show(true);
    showHeader(true);

    mHeaderGroup.startAnimation(fadeOut);
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
        setTextAndShow(mo.getName(), mo.getType().toString());
      }
      else
      {
        mMapObject = mo;
      }
    }
  }

  private void setBodyForPOI(Poi poi)
  {
    mBodyGroup.removeAllViews();
  }

  private void setBodyForBookmark(Bookmark bmk)
  {
    mBodyGroup.removeAllViews();
  }

  private void setBodyForAdditionalLayer(SearchResult sr)
  {
    mBodyGroup.removeAllViews();
  }


  private void setBodyForAPI(MapObject mo)
  {
    mBodyGroup.removeAllViews();
    // TODO
  }

  public void setOnVisibilityChangedListener(OnVisibilityChangedListener listener)
  {
    mVisibilityChangedListener = listener;
  }
}
