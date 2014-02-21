package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.graphics.Color;
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
import android.webkit.WebView;
import android.widget.Button;
import android.widget.PopupMenu;
import android.widget.PopupMenu.OnMenuItemClickListener;
import android.widget.TextView;

import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.bookmarks.BookmarkActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.MapObjectType;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.statistics.Statistics;

@SuppressLint("NewApi")
public class MapObjectFragment extends Fragment
                               implements OnClickListener
{
  private static final int MENU_ADD   = 0x01;
  private static final int MENU_EDIT  = 0x02;
  private static final int MENU_SHARE = 0x10;
  private static final int MENU_P2B = 0xFF;

  private TextView mNameTV;
  private TextView mGroupTV;
  private TextView mTypeTV;
  private WebView mDescrWv;

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
  /// -1 means not initialized (not a bookmark) - C++ code takes Framework::GetDrawScale() for sharing.
  private double mScale = -1.0;

  // General
  MapObjectType mType;

  public void setForBookmark(Bookmark bookmark)
  {
    UiUtils.hide(mAddToBookmarks);
    UiUtils.show(mEditBmk);
    UiUtils.hide(mOpenWith);

    setTexts(bookmark.getName(), null , bookmark.getCategoryName(getActivity()), bookmark.getBookmarkDescription(), bookmark.getLat(), bookmark.getLon());

    final int circleSize = (int) (getResources().getDimension(R.dimen.circle_size) + .5);
    final Drawable icon = UiUtils.drawCircleForPin(bookmark.getIcon().getType(), circleSize, getResources());

    mGroupTV.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(icon, R.dimen.dp_x_4, getResources()), null, null, null);

    mEditBmk.setCompoundDrawables(UiUtils
        .setCompoundDrawableBounds(R.drawable.edit_bookmark, R.dimen.icon_size, getResources()), null, null, null);

    mCategory = bookmark.getCategoryId();
    mBmkIndex = bookmark.getBookmarkId();
    mScale    = bookmark.getScale();

    mType = MapObjectType.BOOKMARK;
  }

  public void setForApiPoint(String name, double lat, double lon)
  {
    UiUtils.show(mAddToBookmarks);
    UiUtils.hide(mEditBmk);

    final ParsedMmwRequest request = ParsedMmwRequest.getCurrentRequest();
    if (request != null  && request.hasPendingIntent())
    {

      mOpenWith.setText(request.hasCustomButtonName()
                          ? request.getCustomButtonName()
                          : getString(R.string.more_info));

      UiUtils.show(mOpenWith);
      mOpenWith.setCompoundDrawables(UiUtils
          .setCompoundDrawableBounds(request.getIcon(getActivity()), R.dimen.icon_size, getResources()), null, null, null);
    }
    else
      UiUtils.hide(mOpenWith);

    setTexts(name, null, null, null, lat, lon);

    mType = MapObjectType.API_POINT;
  }

  public void setForPoi(String name, String type, String address, double lat, double lon)
  {
   UiUtils.show(mAddToBookmarks);
   UiUtils.hide(mEditBmk);
   UiUtils.hide(mOpenWith);

   setTexts(name, type, null, null, lat, lon);

   mType = MapObjectType.POI;
  }

  public void setForMyPosition(double lat, double lon)
  {
   UiUtils.show(mAddToBookmarks);
   UiUtils.hide(mEditBmk);
   UiUtils.hide(mOpenWith);

   final String name = getString(R.string.my_position);
   setTexts(name, null, null, null, lat, lon);

   mType = MapObjectType.MY_POSITION;
  }

  private void setTexts(String name, String type, String group,String descr, double lat, double lon)
  {
    if (!TextUtils.isEmpty(name))
      mName = name;
    else
      mName = getString(R.string.dropped_pin);

    mNameTV.setText(mName);

    // Type of POI
    if (TextUtils.isEmpty(type))
      UiUtils.hide(mTypeTV);
    else
    {
      mTypeTV.setText(type);
      UiUtils.show(mTypeTV);
    }

    // Group of BMK
    if (TextUtils.isEmpty(group))
      UiUtils.hide(mGroupTV);
    else
    {
      mGroupTV.setText(group);
      UiUtils.show(mGroupTV);
    }

    // Description of BMK
    if (TextUtils.isEmpty(descr))
      UiUtils.hide(mDescrWv);
    else
    {
      mDescrWv.loadData(descr, "text/html; charset=UTF-8", null);
      mDescrWv.setBackgroundColor(Color.TRANSPARENT);
      UiUtils.show(mDescrWv);
    }

    mLat = lat;
    mLon = lon;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    final View view = inflater.inflate(R.layout.fragment_map_object, container, false);
    // find views
    mNameTV      = (TextView) view.findViewById(R.id.name);
    mDescrWv     = (WebView) view.findViewById(R.id.descr);
    mGroupTV     = (TextView) view.findViewById(R.id.group);
    mTypeTV      = (TextView) view.findViewById(R.id.type);


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
        .setCompoundDrawableBounds(R.drawable.add_bookmark, R.dimen.icon_size, getResources()), null, null, null);

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
  public void onResume()
  {
    super.onResume();

    adaptUI();

    setUpPickPoint();
  }

  private void setUpPickPoint()
  {
    if (ParsedMmwRequest.hasRequest())
    {
      final ParsedMmwRequest request = ParsedMmwRequest.getCurrentRequest();
      if (request.isPickPointMode())
      {
        mOpenWith.setText(request.hasCustomButtonName()
            ? request.getCustomButtonName()
            : getString(R.string.more_info));

        UiUtils.show(mOpenWith);
        mOpenWith.setCompoundDrawables(UiUtils
            .setCompoundDrawableBounds(request.getIcon(getActivity()), R.dimen.icon_size, getResources()), null, null, null);
        mOpenWith.setOnClickListener(new OnClickListener()
        {
          @Override
          public void onClick(View v)
          {
            onOpenWithClicked();
          }
        });

        request.setPointData(mLat, mLon, mName, "");
        return;
      }
    }
    mOpenWith.setOnClickListener(this);
  }

  private void adaptUI()
  {
    if (Utils.apiEqualOrGreaterThan(11))
    {
      getActivity().invalidateOptionsMenu();
      UiUtils.hide(mAddToBookmarks, mEditBmk);
    }
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
    if (!MWMApplication.get().hasBookmarks())
    {
      final MapObjectActivity activity = (MapObjectActivity) getActivity();
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
    ParsedMmwRequest.getCurrentRequest()
    .sendResponseAndFinish(getActivity(), true);
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    super.onCreateOptionsMenu(menu, inflater);

    if (MapObjectType.BOOKMARK.equals(mType))
      Utils.addMenuCompat(menu, MENU_EDIT, MENU_EDIT, R.string.edit, R.drawable.edit_bookmark);
    else
      Utils.addMenuCompat(menu, MENU_ADD, MENU_ADD, R.string.add_to_bookmarks, R.drawable.add_bookmark);

    Utils.addMenuCompat(menu, MENU_SHARE, MENU_SHARE, R.string.share, R.drawable.share);

    yotaSetup(menu);
  }

  private void yotaSetup(Menu menu)
  {
    if (Yota.isYota())
    {
      menu.add(Menu.NONE, MENU_P2B, MENU_P2B, getString(R.string.show_on_backscreen))
        .setIcon(R.drawable.ic_ptb_gray)
        .setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
    }
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
    else if (itemId == MENU_ADD)
      onAddBookmarkClicked();
    else if (itemId == MENU_EDIT)
      onEditBookmarkClicked();
    else if (itemId == MENU_P2B)
    {
      final boolean addLastKnown = MWMApplication.get().getLocationState().hasPosition();
      Yota.showMap(getActivity(), mLat, mLon, Framework.getDrawScale(),
          mType == MapObjectType.MY_POSITION ? null : mName, addLastKnown);

      Statistics.INSTANCE.trackBackscreenCall(getActivity(), "PlacePage");
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

      @Override
      public String getPoiTypeName()
      {
        return null;
      }
    };
  }

}
