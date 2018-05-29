package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.BookmarkSharingResult;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.widget.placepage.EditBookmarkFragment;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;

public class BookmarksListFragment extends BaseMwmRecyclerFragment
    implements RecyclerLongClickListener, RecyclerClickListener,
               MenuItem.OnMenuItemClickListener,
               BookmarkManager.BookmarksSharingListener
{
  public static final String TAG = BookmarksListFragment.class.getSimpleName();
  public static final String EXTRA_CATEGORY = "bookmark_category";

  private BookmarkCategory mCategory;
  private int mSelectedPosition;

  @CallSuper
  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mCategory = getArguments().getParcelable(EXTRA_CATEGORY);
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new BookmarkListAdapter(getActivity(), mCategory.getId());
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_bookmark_list, container, false);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    configureAdapter();
    setHasOptionsMenu(true);
    ActionBar bar = ((AppCompatActivity) getActivity()).getSupportActionBar();
    if (bar != null)
      bar.setTitle(mCategory.getName());
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addSharingListener(this);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    BookmarkListAdapter adapter = (BookmarkListAdapter) getAdapter();
    if (adapter == null)
      return;

    adapter.startLocationUpdate();
    adapter.notifyDataSetChanged();
  }

  @Override
  public void onPause()
  {
    super.onPause();

    BookmarkListAdapter adapter = (BookmarkListAdapter) getAdapter();
    if (adapter != null)
      adapter.stopLocationUpdate();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeSharingListener(this);
  }

  private void configureAdapter()
  {
    BookmarkListAdapter adapter = (BookmarkListAdapter) getAdapter();
    if (adapter == null)
      return;

    adapter.startLocationUpdate();
    adapter.setOnClickListener(this);
    adapter.setOnLongClickListener(this);
  }

  @Override
  public void onItemClick(View v, int position)
  {
    final Intent i = new Intent(getActivity(), MwmActivity.class);

    BookmarkListAdapter adapter = (BookmarkListAdapter) getAdapter();

    if (adapter != null)
    {
      switch (adapter.getItemViewType(position))
      {
        case BookmarkListAdapter.TYPE_SECTION:
          return;
        case BookmarkListAdapter.TYPE_BOOKMARK:
          final Bookmark bookmark = (Bookmark) adapter.getItem(position);
          i.putExtra(MwmActivity.EXTRA_TASK,
                     new MwmActivity.ShowBookmarkTask(bookmark.getCategoryId(), bookmark.getBookmarkId()));
          break;
        case BookmarkListAdapter.TYPE_TRACK:
          final Track track = (Track) adapter.getItem(position);
          i.putExtra(MwmActivity.EXTRA_TASK,
                     new MwmActivity.ShowTrackTask(track.getCategoryId(), track.getTrackId()));
          break;
      }
    }

    i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    startActivity(i);
  }

  @Override
  public void onLongItemClick(View v, int position)
  {
    BookmarkListAdapter adapter = (BookmarkListAdapter) getAdapter();
    if (adapter == null)
      return;

    mSelectedPosition = position;
    final Object item = adapter.getItem(mSelectedPosition);
    int type = adapter.getItemViewType(mSelectedPosition);

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
                             BookmarkManager.INSTANCE.deleteTrack(((Track) item).getTrackId());
                             adapter.notifyDataSetChanged();
                             return false;
                           }
                         }).tint().show();
        break;
    }
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    SharingHelper.INSTANCE.onPreparedFileForSharing(getActivity(), result);
  }

  @Override
  public boolean onMenuItemClick(MenuItem menuItem)
  {
    BookmarkListAdapter adapter = (BookmarkListAdapter) getAdapter();
    if (adapter == null)
      return false;

    Bookmark item = (Bookmark) adapter.getItem(mSelectedPosition);

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
      EditBookmarkFragment.editBookmark(item.getCategoryId(), item.getBookmarkId(), getActivity(),
                                        getChildFragmentManager(), new EditBookmarkFragment.EditBookmarkListener()
          {
            @Override
            public void onBookmarkSaved(long bookmarkId)
            {
              adapter.notifyDataSetChanged();
            }
          });
      break;

    case R.id.delete:
      BookmarkManager.INSTANCE.deleteBookmark(item.getBookmarkId());
      adapter.notifyDataSetChanged();
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
      SharingHelper.INSTANCE.prepareBookmarkCategoryForSharing(getActivity(), mCategory.getId());
      return true;
    }

    return super.onOptionsItemSelected(item);
  }
}
