package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.ListView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.util.ShareAction;

public class BookmarksListFragment extends BaseMwmListFragment
{
  public static final String TAG = "BookmarkListActivity";

  private EditText mSetName;
  private BookmarkCategory mEditedSet;
  private int mSelectedPosition;
  private BookmarkListAdapter mPinAdapter;
  private int mIndex;
  private ActionBar mActionBar;

  // Menu routines
  static final int ID_SEND_BY_EMAIL = 0x01;
  static final int MENU_DELETE_TRACK = 0x42;

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    final ViewGroup root = (ViewGroup) inflater.inflate(R.layout.fragment_bookmarks_list, container, false);
    setUpViews(root);
    return root;
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    createListAdapter();
    registerForContextMenu(getListView());
    setHasOptionsMenu(true);
    if (getActivity() instanceof AppCompatActivity)
      mActionBar = ((AppCompatActivity) getActivity()).getSupportActionBar();
  }

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    // Initialize with passed edited set.
    mIndex = getArguments().getInt(ChooseBookmarkCategoryActivity.BOOKMARK_SET, -1);
    mEditedSet = BookmarkManager.INSTANCE.getCategoryById(mIndex);
  }

  @Override
  public void onListItemClick(ListView l, View v, int position, long id)
  {
    switch (mPinAdapter.getItemViewType(position))
    {
    case BookmarkListAdapter.TYPE_SECTION:
      return;
    case BookmarkListAdapter.TYPE_BMK:
      final Bookmark bmk = (Bookmark) mPinAdapter.getItem(position);
      BookmarkManager.INSTANCE.showBookmarkOnMap(mIndex, bmk.getBookmarkId());
      break;
    case BookmarkListAdapter.TYPE_TRACK:
      final Track track = (Track) mPinAdapter.getItem(position);
      Framework.nativeShowTrackRect(track.getCategoryId(), track.getTrackId());
      break;
    }

    final Intent i = new Intent(getActivity(), MWMActivity.class);
    i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    startActivity(i);
  }

  private void createListAdapter()
  {
    mPinAdapter = new BookmarkListAdapter(getActivity(), mEditedSet);

    setListAdapter(mPinAdapter);

    mPinAdapter.startLocationUpdate();
  }

  private void assignCategoryParams()
  {
    final String name = mSetName.getText().toString();
    if (!name.equals(mEditedSet.getName()))
      BookmarkManager.INSTANCE.setCategoryName(mEditedSet, name);
  }

  private void setUpViews(ViewGroup root)
  {
    mSetName = (EditText) root.findViewById(R.id.pin_set_name);
    mSetName.setText(mEditedSet.getName());
    mSetName.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        // Note! Do not set actual name here - saving process may be too long
        // see assignCategoryParams() instead.
        if (mActionBar != null)
          mActionBar.setTitle(s);
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

      @Override
      public void afterTextChanged(Editable s) {}
    });
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo)
  {
    assignCategoryParams();

    // Some list views can be section delimiters.
    if (menuInfo instanceof AdapterView.AdapterContextMenuInfo)
    {
      final AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;

      mSelectedPosition = info.position;
      final Object obj = mPinAdapter.getItem(mSelectedPosition);
      final int type = mPinAdapter.getItemViewType(mSelectedPosition);

      if (type == BookmarkListAdapter.TYPE_BMK)
      {
        final MenuInflater inflater = getActivity().getMenuInflater();
        inflater.inflate(R.menu.pin_sets_context_menu, menu);

        for (final ShareAction sa : ShareAction.ACTIONS.values())
        {
          if (sa.isSupported(getActivity()))
            menu.add(Menu.NONE, sa.getId(), sa.getId(), getResources().getString(sa.getNameResId()));
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
    final Object obj = mPinAdapter.getItem(mSelectedPosition);

    if (itemId == R.id.set_edit)
    {
      startPinActivity(mEditedSet.getId(), ((Bookmark) obj).getBookmarkId());
    }
    else if (itemId == R.id.set_delete)
    {
      BookmarkManager.INSTANCE.deleteBookmark((Bookmark) obj);
      mPinAdapter.notifyDataSetChanged();
    }
    else if (ShareAction.ACTIONS.containsKey(itemId))
    {
      final ShareAction shareAction = ShareAction.ACTIONS.get(itemId);
      shareAction.shareMapObject(getActivity(), (Bookmark) obj);
    }
    else if (itemId == MENU_DELETE_TRACK)
    {
      BookmarkManager.INSTANCE.deleteTrack((Track) obj);
      mPinAdapter.notifyDataSetChanged();
    }

    return super.onContextItemSelected(item);
  }

  private void startPinActivity(int cat, int bmk)
  {
    startActivity(new Intent(getActivity(), ChooseBookmarkCategoryActivity.class)
        .putExtra(ChooseBookmarkCategoryActivity.BOOKMARK, new ParcelablePoint(cat, bmk)));
  }

  @Override
  public void onResume()
  {
    super.onResume();

    mPinAdapter.startLocationUpdate();
  }

  @Override
  public void onStart()
  {
    super.onStart();

    mPinAdapter.notifyDataSetChanged();
  }

  @Override
  public void onPause()
  {
    assignCategoryParams();

    mPinAdapter.stopLocationUpdate();

    super.onPause();
  }

  private void sendBookmarkMail()
  {
    assignCategoryParams();

    String path = MWMApplication.get().getTempPath();
    final String name = BookmarkManager.INSTANCE.saveToKmzFile(mEditedSet.getId(), path);
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
//
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == ID_SEND_BY_EMAIL)
    {
      sendBookmarkMail();
      return true;
    }

    return super.onOptionsItemSelected(item);
  }
}
