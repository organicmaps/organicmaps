package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.widget.placepage.EditBookmarkFragment;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;

public class BookmarksListFragment extends BaseMwmListFragment
                                implements AdapterView.OnItemLongClickListener,
                                           MenuItem.OnMenuItemClickListener
{
  public static final String TAG = BookmarksListFragment.class.getSimpleName();

  private BookmarkCategory mCategory;
  private int mCategoryIndex;
  private int mSelectedPosition;
  @Nullable
  private BookmarkListAdapter mAdapter;

  @CallSuper
  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mCategoryIndex = getArguments().getInt(ChooseBookmarkCategoryFragment.CATEGORY_ID, -1);
    mCategory = BookmarkManager.INSTANCE.getCategory(mCategoryIndex);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.simple_list, container, false);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    initList();
    setHasOptionsMenu(true);
    ActionBar bar = ((AppCompatActivity) getActivity()).getSupportActionBar();
    if (bar != null)
      bar.setTitle(mCategory.getName());
  }

  @Override
  public void onResume()
  {
    super.onResume();
    if (mAdapter == null)
      return;

    mAdapter.startLocationUpdate();
    mAdapter.notifyDataSetChanged();
  }

  @Override
  public void onPause()
  {
    super.onPause();

    if (mAdapter != null)
      mAdapter.stopLocationUpdate();
  }

  private void initList()
  {
    mAdapter = new BookmarkListAdapter(getActivity(), mCategory);
    mAdapter.startLocationUpdate();
    setListAdapter(mAdapter);
    getListView().setOnItemLongClickListener(this);
  }

  @Override
  public void onListItemClick(ListView l, View v, int position, long id)
  {
    final Intent i = new Intent(getActivity(), MwmActivity.class);

    if (mAdapter != null)
    {
      switch (mAdapter.getItemViewType(position))
      {
        case BookmarkListAdapter.TYPE_SECTION:
          return;
        case BookmarkListAdapter.TYPE_BOOKMARK:
          final Bookmark bookmark = (Bookmark) mAdapter.getItem(position);
          i.putExtra(MwmActivity.EXTRA_TASK,
                     new MwmActivity.ShowBookmarkTask(mCategoryIndex, bookmark.getBookmarkId()));
          break;
        case BookmarkListAdapter.TYPE_TRACK:
          final Track track = (Track) mAdapter.getItem(position);
          i.putExtra(MwmActivity.EXTRA_TASK,
                     new MwmActivity.ShowTrackTask(track.getCategoryId(), track.getTrackId()));
          break;
      }
    }

    i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    startActivity(i);
  }

  @Override
  public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id)
  {
    if (mAdapter == null)
      return false;

    mSelectedPosition = position;
    final Object item = mAdapter.getItem(mSelectedPosition);
    int type = mAdapter.getItemViewType(mSelectedPosition);

    switch (type)
    {
    case BookmarkListAdapter.TYPE_SECTION:
      // Do nothing here?
      break;

    case BookmarkListAdapter.TYPE_BOOKMARK:
      BottomSheetHelper.Builder bs = BottomSheetHelper.create(getActivity(), ((Bookmark) item).getTitle())
                                                      .sheet(R.menu.menu_bookmarks)
                                                      .listener(this);
      if (!ShareOption.SMS.isSupported(getActivity()))
        bs.getMenu().removeItem(R.id.share_message);

      if (!ShareOption.EMAIL.isSupported(getActivity()))
        bs.getMenu().removeItem(R.id.share_email);

      bs.tint().show();
      break;

    case BookmarkListAdapter.TYPE_TRACK:
      BottomSheetHelper.create(getActivity(), ((Track) item).getName())
                       .sheet(Menu.NONE, R.drawable.ic_delete, R.string.delete)
                       .listener(new MenuItem.OnMenuItemClickListener()
                       {
                         @Override
                         public boolean onMenuItemClick(MenuItem menuItem)
                         {
                           BookmarkManager.INSTANCE.deleteTrack((Track) item);
                           mAdapter.notifyDataSetChanged();
                           return false;
                         }
                       }).tint().show();
      break;
    }

    return true;
  }

  @Override
  public boolean onMenuItemClick(MenuItem menuItem)
  {
    if (mAdapter == null)
      return false;

    Bookmark item = (Bookmark) mAdapter.getItem(mSelectedPosition);

    switch (menuItem.getItemId())
    {
    case R.id.share_message:
      ShareOption.SMS.shareMapObject(getActivity(), item, Sponsored.nativeGetCurrent());
      break;

    case R.id.share_email:
      ShareOption.EMAIL.shareMapObject(getActivity(), item, Sponsored.nativeGetCurrent());
      break;

    case R.id.share:
      ShareOption.ANY.shareMapObject(getActivity(), item, Sponsored.nativeGetCurrent());
      break;

    case R.id.edit:
      EditBookmarkFragment.editBookmark(mCategory.getId(), item.getBookmarkId(), getActivity(),
                                        getChildFragmentManager(), new EditBookmarkFragment.EditBookmarkListener()
          {
            @Override
            public void onBookmarkSaved(int categoryId, int bookmarkId)
            {
              mAdapter.notifyDataSetChanged();
            }
          });
      break;

    case R.id.delete:
      BookmarkManager.INSTANCE.deleteBookmark(item);
      mAdapter.notifyDataSetChanged();
      break;
    }
    return false;
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    inflater.inflate(R.menu.option_menu_bookmarks, menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.set_share)
    {
      SharingHelper.shareBookmarksCategory(getActivity(), mCategory.getId());
      return true;
    }

    return super.onOptionsItemSelected(item);
  }
}
