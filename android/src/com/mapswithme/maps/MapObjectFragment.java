package com.mapswithme.maps;

import android.annotation.TargetApi;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.util.Pair;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.PopupMenu;
import android.widget.PopupMenu.OnMenuItemClickListener;
import android.widget.TextView;

import com.mapswithme.maps.api.MWMRequest;
import com.mapswithme.maps.bookmarks.BookmarkActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.state.SuppotedState;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import java.io.Serializable;

public class MapObjectFragment extends Fragment
                               implements OnClickListener
{
  
  public static enum MapObjectType implements Serializable 
  {
    POI,
    API_POINT,
    BOOKMARK,
    MY_POSITION
  }
  
  private static final int MENU_SHARE = 0x100;
  
  private TextView mNameTV;
  private TextView mGroupTypeTV;
  private TextView mLatLonTV;
  private TextView mDescrTV;

  private Button mAddToBookmarks;
  private Button mEditBmk;
  private Button mShare;
  private Button mOpenWith;
  
  
  //POI, API 
  private double mLat;
  private double mLon;
  private String mName;
  
  //Bookmark
  private int mCategory;
  private int mBmkIndex;
  private double mScale = MapObject.DEF_SCALE;
  
  // General
  MapObjectType mType;
  
  public void setForBookmark(Bookmark bookmark)
  {
    UiUtils.hide(mAddToBookmarks);
    UiUtils.show(mEditBmk);
    UiUtils.hide(mOpenWith);
    
    setTexts(bookmark.getName(), bookmark.getCategoryName(), bookmark.getBookmarkDescription(), bookmark.getLat(), bookmark.getLon());
    
    @SuppressWarnings("deprecation")
    Drawable icon = new BitmapDrawable(bookmark.getIcon().getIcon());
    mNameTV.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(icon, R.dimen.icon_size, getResources()), null, null, null);
    
    mEditBmk.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(R.drawable.edit_bookmark, R.dimen.icon_size, getResources()), null, null, null);
    
    mCategory = bookmark.getCategoryId();
    mBmkIndex = bookmark.getBookmarkId();
    mScale = bookmark.getScale();
    
    mType = MapObjectType.BOOKMARK;
  }
  
  public void setForApiPoint(String name, double lat, double lon)
  {
    UiUtils.show(mAddToBookmarks);
    UiUtils.hide(mEditBmk);
    
    if (MWMRequest.hasRequest())
    {
      UiUtils.show(mOpenWith);
      mOpenWith.setCompoundDrawables(UiUtils
          .setCompoundDrawableBounds(MWMRequest.getCurrentRequest().getIcon(getActivity()), R.dimen.icon_size, getResources()), null, null, null);
    }
    else 
      UiUtils.hide(mOpenWith);
    
    setTexts(name, null, null, lat, lon);
    
    mType = MapObjectType.API_POINT;
  }
  
  public void setForPoi(String name, String type, String address, double lat, double lon)
  {
   UiUtils.show(mAddToBookmarks);
   UiUtils.hide(mEditBmk);
   UiUtils.hide(mOpenWith);
   
   setTexts(name, type, null, lat, lon);
   
   mType = MapObjectType.POI;
  }
  
  public void setForMyPosition(double lat, double lon)
  {
   UiUtils.show(mAddToBookmarks);
   UiUtils.hide(mEditBmk);
   UiUtils.hide(mOpenWith);
   
   final String name = getString(R.string.my_position);
   setTexts(name, "", null, lat, lon);
   
   mType = MapObjectType.MY_POSITION;
  }
  
  private void setTexts(String name, String type, String descr, double lat, double lon)
  {
    if (!TextUtils.isEmpty(name))
      mName = name;
    else 
      mName = getString(R.string.dropped_pin);
    
    mNameTV.setText(mName);
    
    if (TextUtils.isEmpty(type))
      UiUtils.hide(mGroupTypeTV);
    else 
    {
      mGroupTypeTV.setText(type);
      UiUtils.show(mGroupTypeTV);
    }
    
    if (TextUtils.isEmpty(descr))
      UiUtils.hide(mDescrTV);
    else 
    {
      mDescrTV.setText(descr);
      UiUtils.show(mDescrTV);
    }
    
    mLatLonTV.setText(UiUtils.formatLatLon(lat, lon));
    mLat = lat;
    mLon = lon;
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    final View view = inflater.inflate(R.layout.fragment_map_object, container, false);
    // find views
    mNameTV      = (TextView) view.findViewById(R.id.name);
    mGroupTypeTV = (TextView) view.findViewById(R.id.groupType);
    mLatLonTV    = (TextView) view.findViewById(R.id.latLon);
    mDescrTV     = (TextView) view.findViewById(R.id.descr);
    
    
    mAddToBookmarks = (Button) view.findViewById(R.id.addToBookmarks);
    mEditBmk        = (Button) view.findViewById(R.id.editBookmark);
    mOpenWith       = (Button) view.findViewById(R.id.openWith);
    mShare          = (Button) view.findViewById(R.id.share);
    
    // set up listeners, drawables, visibility
    mAddToBookmarks.setOnClickListener(this);
    mShare.setOnClickListener(this);
    mOpenWith.setOnClickListener(this);
    mEditBmk.setOnClickListener(this);
    
    mAddToBookmarks.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(R.drawable.placemark_red, R.dimen.icon_size, getResources()), null, null, null);
    
    if (Utils.apiEqualOrGreaterThan(11))
      UiUtils.hide(mShare);
    else 
    {
      mShare.setCompoundDrawables(UiUtils
          .setCompoundDrawableBounds(R.drawable.share, R.dimen.icon_size, getResources()), null, null, null);
      UiUtils.show(mShare);
    }
    
    return view;
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setHasOptionsMenu(true);
  }
  
  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    
    if (id == R.id.addToBookmarks)
      onAddBookmarkClicked();
    if (id == R.id.editBookmark)
      onEditBookmarkClicked();
    if (id == R.id.openWith)
      onOpenWithClicked();
    if (id == R.id.share)
      showShareContextMenu();
  }
  
  private void onAddBookmarkClicked()
  {
    //TODO add normal PRO check
    if ( !((MWMApplication)getActivity().getApplication()).isProVersion())
    {
      // TODO this cast if safe, but style is bad
      MapObjectActivity activity = (MapObjectActivity) getActivity();
      activity.showProVersionBanner(getString(R.string.bookmarks_in_pro_version));
    }
    else
    {
      final Pair<Integer, Integer> bmkAndCat = BookmarkManager.getBookmarkManager(getActivity()).addNewBookmark(mName, mLat, mLon);
      BookmarkActivity.startWithBookmark(getActivity(), bmkAndCat.first, bmkAndCat.second);
      // for now finish 
      getActivity().finish();
    }
  }
  
  private void onEditBookmarkClicked()
  {
    BookmarkActivity.startWithBookmark(getActivity(), mCategory, mBmkIndex);
    getActivity().finish();
  }
  
  private void onOpenWithClicked()
  {
    MWMRequest.getCurrentRequest().sendResponse(getActivity(), true);
    ((MWMApplication)getActivity().getApplication()).getAppStateManager().transitionTo(SuppotedState.DEFAULT_MAP);
    getActivity().finish();
  }
  
  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    super.onCreateOptionsMenu(menu, inflater);
    
    final MenuItem shareMenu = menu.add(Menu.NONE, MENU_SHARE, 0, R.string.share);
    shareMenu.setIcon(R.drawable.share);
    if (Utils.apiEqualOrGreaterThan(11))
      shareMenu.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
  }
  
  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    final int itemId = item.getItemId();
    
    if (itemId == MENU_SHARE)
    {
      if (Utils.apiEqualOrGreaterThan(11))
        onShareActionClicked(getActivity().findViewById(MENU_SHARE));
      else 
        showShareContextMenu();
      
      return true;
    }
    else if (ShareAction.ACTIONS.containsKey(itemId))
    {
      ShareAction.ACTIONS.get(itemId).shareMapObject(getActivity(), createMapObject());
      return true;
    }
    
    return super.onOptionsItemSelected(item);
  }
  
  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    final int itemId = item.getItemId(); 
    
    if (ShareAction.ACTIONS.containsKey(itemId))
    {
      ShareAction.ACTIONS.get(itemId).shareMapObject(getActivity(), createMapObject());
      return true;
    }
    return super.onContextItemSelected(item);
  }
  
  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    super.onCreateContextMenu(menu, v, menuInfo);
    
    ShareAction.getSmsShare().addToMenuIfSupported(getActivity(), menu, false);
    ShareAction.getEmailShare().addToMenuIfSupported(getActivity(), menu, false);
    ShareAction.getAnyShare().addToMenuIfSupported(getActivity(), menu, false);
  }
  
  private void onShareActionClicked(View anchor)
  {
    final PopupMenu popUpMenu = new PopupMenu(getActivity(), anchor);
    final Menu menu = popUpMenu.getMenu();
    
    ShareAction.getSmsShare().addToMenuIfSupported(getActivity(), menu, false);
    ShareAction.getEmailShare().addToMenuIfSupported(getActivity(), menu, false);
    ShareAction.getAnyShare().addToMenuIfSupported(getActivity(), menu, false);
    
    popUpMenu.setOnMenuItemClickListener(new OnMenuItemClickListener()
    {
      
      @Override
      public boolean onMenuItemClick(MenuItem item)
      {
        return onOptionsItemSelected(item);
      }
    });
    
    popUpMenu.show();
  }
  
  private void showShareContextMenu()
  {
    registerForContextMenu(mShare);
    mShare.showContextMenu();
    unregisterForContextMenu(mShare);
  }
  
  private MapObject createMapObject()
  {
    return new MapObject()
    {
      
      @Override
      public String getName()  { return mName; }
      
      @Override
      public double getLon()   { return mLon; }
      
      @Override
      public double getLat()   { return mLat; }
      
      @Override
      public double getScale() { return mScale; }

      @Override
      public MapObjectType getType() { return mType; }
    };
  }
  
}
