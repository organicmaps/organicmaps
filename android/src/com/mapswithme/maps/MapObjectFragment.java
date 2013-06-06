package com.mapswithme.maps;

import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.mapswithme.maps.api.MWMRequest;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.util.UiUtils;

import java.io.Serializable;

public class MapObjectFragment extends Fragment
                               implements OnClickListener
{
  
  public static enum MapObjectType implements Serializable 
  {
    POI,
    API_POINT,
    BOOKMARK
  }
  
  private TextView mName;
  private TextView mGroupType;
  private TextView mLatLon;
  private TextView mDescr;

  private Button mAddToBookmarks;
  private Button mEditBmk;
  private Button mShare;
  private Button mOpenWith;
  
  public void setForBookmark(Bookmark bookmark)
  {
    UiUtils.hide(mAddToBookmarks);
    UiUtils.show(mEditBmk);
    UiUtils.hide(mOpenWith);
    UiUtils.show(mShare);
    
    setTexts(bookmark.getName(), bookmark.getCategoryName(), null, bookmark.getLat(), bookmark.getLon());
    
    @SuppressWarnings("deprecation")
    Drawable icon = new BitmapDrawable(bookmark.getIcon().getIcon());
    mName.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(icon, R.dimen.icon_size, getResources()), null, null, null);
    
    mEditBmk.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(android.R.drawable.ic_menu_edit, R.dimen.icon_size, getResources()), null, null, null);
    
    //TODO add buttons processors
  }
  
  public void setForApiPoint(MWMRequest request)
  {
    UiUtils.show(mAddToBookmarks);
    UiUtils.hide(mEditBmk);
    UiUtils.show(mOpenWith);
    UiUtils.show(mShare);
    
    setTexts(request.getName(), null, null, request.getLat(), request.getLon());
    mOpenWith.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(request.getIcon(getActivity()), R.dimen.icon_size, getResources()), null, null, null);
    
    //TODO add buttons processors
  }
  
  public void setForPoi(String name, String type, String address, double lat, double lon)
  {
   UiUtils.show(mAddToBookmarks);
   UiUtils.hide(mEditBmk);
   UiUtils.hide(mOpenWith);
   UiUtils.show(mShare);
   
   setTexts(name, type, null, lat, lon);
   
   //TODO add buttons processors
  }
  
  private void setTexts(String name, String type, String descr, double lat, double lon)
  {
    if (!TextUtils.isEmpty(name))
      mName.setText(name);
    else 
      mName.setText(R.string.dropped_pin);
    
    if (TextUtils.isEmpty(type))
      UiUtils.hide(mGroupType);
    else 
    {
      mGroupType.setText(type);
      UiUtils.show(mGroupType);
    }
    
    if (TextUtils.isEmpty(descr))
      UiUtils.hide(mDescr);
    else 
    {
      mDescr.setText(descr);
      UiUtils.show(mDescr);
    }
    
    mLatLon.setText(UiUtils.formatLatLon(lat, lon));
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    final View view = inflater.inflate(R.layout.fragment_map_object, container, false);
    // find views
    mName      = (TextView) view.findViewById(R.id.name);
    mGroupType = (TextView) view.findViewById(R.id.groupType);
    mLatLon    = (TextView) view.findViewById(R.id.latLon);
    mDescr     = (TextView) view.findViewById(R.id.descr);
    
    
    mAddToBookmarks = (Button) view.findViewById(R.id.addToBookmarks);
    mEditBmk        = (Button) view.findViewById(R.id.editBookmark);
    mShare          = (Button) view.findViewById(R.id.share);
    mOpenWith       = (Button) view.findViewById(R.id.openWith);
    
    // set up listeners
    mAddToBookmarks.setOnClickListener(this);
    mShare.setOnClickListener(this);
    mOpenWith.setOnClickListener(this);
    mEditBmk.setOnClickListener(this);
    
    mAddToBookmarks.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(R.drawable.placemark_red, R.dimen.icon_size, getResources()), null, null, null);
    
    mShare.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(android.R.drawable.ic_menu_share, R.dimen.icon_size, getResources()), null, null, null);
    
    return view;
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
  }
  
}
