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
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.util.UiUtils;

public class MapInfoView extends LinearLayout
{
  public interface OnVisibilityChangedListener
  {
    public void onHeadVisibilityChanged(boolean isVisible);
    public void onbodyVisibilityChanged(boolean isVisible);
  }

  private boolean mIsHeaderVisible = true;
  private boolean mIsBodyVisible   = true;
  private boolean mIsVisible       = true;

  private final ViewGroup mHeaderGroup;
  private final ViewGroup mBodyGroup;
  private final View mView;

  // Header
  private final TextView mTitle;;
  private final TextView mSubtitle;

  // Body
  private final ImageView mImage;

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

    // Body
    mImage = (ImageView) mBodyGroup.findViewById(R.id.info_image);
    mImage.setImageResource(R.drawable.grump);

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

  public void showBody(boolean show)
  {
    if (mIsBodyVisible == show)
      return; // if state is already same as we need

    UiUtils.hideIf(!show, mBodyGroup);
    mIsBodyVisible = show;
  }

  public void showHeader(boolean show)
  {
    UiUtils.hideIf(!show, mHeaderGroup);
    mIsHeaderVisible = show;
  }

  public void show(boolean show)
  {
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
}
