package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.widget.placepage.EditBookmarkFragment;
import com.mapswithme.util.ShareAction;

public class BookmarksListFragment extends BaseMwmListFragment
{
  public static final String TAG = BookmarksListFragment.class.getSimpleName();
  private static final int MENU_DELETE_TRACK = 0x42;

  private BookmarkCategory mCategory;
  private int mCategoryIndex;
  private int mSelectedPosition;
  private BookmarkListAdapter mAdapter;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    mCategoryIndex = getArguments().getInt(ChooseBookmarkCategoryActivity.BOOKMARK_SET, -1);
    mCategory = BookmarkManager.INSTANCE.getCategoryById(mCategoryIndex);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_bookmarks_list, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    initListAdapter();
    setListAdapter(mAdapter);
    registerForContextMenu(getListView());
    setHasOptionsMenu(true);
    ((AppCompatActivity) getActivity()).getSupportActionBar().setTitle(mCategory.getName());
  }

  @Override
  public void onResume()
  {
    super.onResume();

    mAdapter.startLocationUpdate();
    mAdapter.notifyDataSetChanged();
  }

  @Override
  public void onPause()
  {
    super.onPause();

    mAdapter.stopLocationUpdate();
  }

  private void initListAdapter()
  {
    mAdapter = new BookmarkListAdapter(getActivity(), mCategory);
    mAdapter.startLocationUpdate();
  }

  @Override
  public void onListItemClick(ListView l, View v, int position, long id)
  {
    switch (mAdapter.getItemViewType(position))
    {
    case BookmarkListAdapter.TYPE_SECTION:
      return;
    case BookmarkListAdapter.TYPE_BOOKMARK:
      final Bookmark bookmark = (Bookmark) mAdapter.getItem(position);
      BookmarkManager.INSTANCE.showBookmarkOnMap(mCategoryIndex, bookmark.getBookmarkId());
      break;
    case BookmarkListAdapter.TYPE_TRACK:
      final Track track = (Track) mAdapter.getItem(position);
      Framework.nativeShowTrackRect(track.getCategoryId(), track.getTrackId());
      break;
    }

    final Intent i = new Intent(getActivity(), MWMActivity.class);
    i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    startActivity(i);
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo)
  {
    if (menuInfo instanceof AdapterView.AdapterContextMenuInfo)
    {
      final AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;

      mSelectedPosition = info.position;
      final Object obj = mAdapter.getItem(mSelectedPosition);
      final int type = mAdapter.getItemViewType(mSelectedPosition);

      if (type == BookmarkListAdapter.TYPE_BOOKMARK)
      {
        final MenuInflater inflater = getActivity().getMenuInflater();
        inflater.inflate(R.menu.menu_bookmarks, menu);

        for (final ShareAction action : ShareAction.ACTIONS.values())
        {
          if (action.isSupported(getActivity()))
            menu.add(Menu.NONE, action.getId(), action.getId(), getResources().getString(action.getNameResId()));
        }

        menu.setHeaderTitle(((Bookmark) obj).getName());
      }
      else if (type == BookmarkListAdapter.TYPE_TRACK)
      {
        menu.add(Menu.NONE, MENU_DELETE_TRACK, MENU_DELETE_TRACK, getString(R.string.delete));
        menu.setHeaderTitle(((Track) obj).getName());
      }

      super.onCreateContextMenu(menu, v, menuInfo);
    }
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    final int itemId = item.getItemId();
    final Object obj = mAdapter.getItem(mSelectedPosition);

    if (itemId == R.id.set_edit)
    {
      editBookmark(mCategory.getId(), ((Bookmark) obj).getBookmarkId());
    }
    else if (itemId == R.id.set_delete)
    {
      BookmarkManager.INSTANCE.deleteBookmark((Bookmark) obj);
      mAdapter.notifyDataSetChanged();
    }
    else if (ShareAction.ACTIONS.containsKey(itemId))
    {
      final ShareAction shareAction = ShareAction.ACTIONS.get(itemId);
      shareAction.shareMapObject(getActivity(), (Bookmark) obj);
    }
    else if (itemId == MENU_DELETE_TRACK)
    {
      BookmarkManager.INSTANCE.deleteTrack((Track) obj);
      mAdapter.notifyDataSetChanged();
    }

    return super.onContextItemSelected(item);
  }

  private void editBookmark(int cat, int bmk)
  {
    final Bundle args = new Bundle();
    args.putInt(EditBookmarkFragment.EXTRA_CATEGORY_ID, cat);
    args.putInt(EditBookmarkFragment.EXTRA_BOOKMARK_ID, bmk);
    final EditBookmarkFragment fragment = (EditBookmarkFragment) Fragment.instantiate(getActivity(), EditBookmarkFragment.class.getName(), args);
    fragment.setArguments(args);
    fragment.show(getActivity().getSupportFragmentManager(), null);
  }

  private void sendBookmarkMail()
  {
    String path = MWMApplication.get().getTempPath();
    final String name = BookmarkManager.INSTANCE.saveToKmzFile(mCategory.getId(), path);
    if (name == null)
    {
      // some error occurred
      return;
    }

    final Intent intent = new Intent(Intent.ACTION_SEND);
    intent.putExtra(android.content.Intent.EXTRA_SUBJECT, getString(R.string.share_bookmarks_email_subject));
    intent.putExtra(android.content.Intent.EXTRA_TEXT, String.format(getString(R.string.share_bookmarks_email_body), name));

    path = path + name + ".kmz";
    Log.d(TAG, "KMZ file path = " + path);
    intent.putExtra(android.content.Intent.EXTRA_STREAM, Uri.parse("file://" + path));
    intent.setType("application/vnd.google-earth.kmz");

    try
    {
      startActivity(Intent.createChooser(intent, getString(R.string.share_by_email)));
    } catch (final Exception ex)
    {
      Log.i(TAG, "Can't run E-Mail activity" + ex);
    }
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
      sendBookmarkMail();
      return true;
    }

    return super.onOptionsItemSelected(item);
  }
}
