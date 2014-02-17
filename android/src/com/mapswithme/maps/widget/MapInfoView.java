package com.mapswithme.maps.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.util.UiUtils;

public class MapInfoView extends LinearLayout
{
  private boolean mIsHeaderVisible;
  private boolean mIsBodyVisible;
  private boolean mIsVisible;

  private final ViewGroup mHeaderGroup;
  private final ViewGroup mBodyGroup;
  private final View mView;

  // Header
  private final TextView mTitle;;
  private final TextView mSubtitle;

  // Data
  MapObject mMapObject;



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
    UiUtils.hideIf(!show, mBodyGroup);
    mIsBodyVisible = show;
  }

  public void showHeader(boolean show)
  {
    UiUtils.hideIf(!show, mHeaderGroup);
    mIsBodyVisible = show;
  }

  public void show(boolean show)
  {
    UiUtils.hideIf(!show, mView);
    mIsVisible = show;
  }

  private void setTextAndShow(CharSequence title, CharSequence subtitle)
  {
    mTitle.setText(title);
    mSubtitle.setText(subtitle);
    show(true);
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
        setTextAndShow(mo.getName(), mo.getType().toString());
      }
      else
      {
        mMapObject = mo;
      }
    }
  }
}
