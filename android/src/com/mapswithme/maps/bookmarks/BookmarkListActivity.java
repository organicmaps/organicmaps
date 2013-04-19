package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.CheckBox;
import android.widget.EditText;

import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkListAdapter.DataChangedListener;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.util.ShareAction;


public class BookmarkListActivity extends AbstractBookmarkListActivity
{
  public static final String TAG = "BookmarkListActivity";
  public static final String EDIT_CONTENT = "edit_content";

  private EditText mSetName;
  private CheckBox mIsVisible;
  private BookmarkCategory mEditedSet;
  private int mSelectedPosition;
  private BookmarkListAdapter mPinAdapter;
  private boolean mEditContent;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.bookmarks_list);
    final int setIndex = getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1);
    mEditContent = getIntent().getBooleanExtra(EDIT_CONTENT, true);
    mEditedSet = mManager.getCategoryById(setIndex);
    setTitle(mEditedSet.getName());

    if (mEditedSet != null)
      createListAdapter();

    setUpViews();
    getListView().setOnItemClickListener(new OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        Intent i = new Intent(BookmarkListActivity.this, MWMActivity.class);
        i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        mManager.showBookmarkOnMap(setIndex, position);
        startActivity(i);
      }
    });
    registerForContextMenu(getListView());
  }

  private void createListAdapter()
  {
    setListAdapter(mPinAdapter = new BookmarkListAdapter(this,
                                                         ((MWMApplication) getApplication()).getLocationService(),
                                                         mEditedSet, new DataChangedListener()
    {
      @Override
      public void onDataChanged(int vis)
      {
        findViewById(R.id.bookmark_usage_hint).setVisibility(vis);
      }
    }));

    mPinAdapter.startLocationUpdate();
  }

  private void assignCategoryParams()
  {
    if (mEditedSet != null)
    {
      final String name = mSetName.getText().toString();
      if (!name.equals(mEditedSet.getName()))
        mManager.setCategoryName(mEditedSet, name);

      final boolean visible = mIsVisible.isChecked();
      if (visible != mEditedSet.isVisible())
        mEditedSet.setVisibility(mIsVisible.isChecked());
    }
  }

  private void setUpViews()
  {
    mSetName = (EditText) findViewById(R.id.pin_set_name);
    if (mEditedSet != null)
      mSetName.setText(mEditedSet.getName());

    mSetName.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        setTitle(s.toString());

        // Note! Do not set actual name here - saving process may be too long
        // see assignCategoryParams() instead.
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after)
      {
      }

      @Override
      public void afterTextChanged(Editable s)
      {
      }
    });

    mIsVisible = (CheckBox) findViewById(R.id.pin_set_visible);
    if (mEditedSet != null)
      mIsVisible.setChecked(mEditedSet.isVisible());
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    if (mEditContent)
    {
      assignCategoryParams();

      if (menuInfo instanceof AdapterView.AdapterContextMenuInfo)
      {
        AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
        mSelectedPosition = info.position;
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.pin_sets_context_menu, menu);


        for (ShareAction sa : ShareAction.ACTIONS.values())
        {
          if (sa.isSupported(this))
            menu.add(Menu.NONE, sa.getId(), sa.getId(), getResources().getString(sa.getNameResId()));
        }

        menu.setHeaderTitle(mManager.getBookmark(mEditedSet.getId(), mSelectedPosition).getName());
      }

      super.onCreateContextMenu(menu, v, menuInfo);
    }
  }

  private void startPinActivity(int cat, int bmk)
  {
    startActivity(new Intent(this, BookmarkActivity.class)
    .putExtra(BookmarkActivity.PIN, new ParcelablePoint(cat, bmk)));
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    int itemId = item.getItemId();
    if (itemId == R.id.set_edit)
    {
      startPinActivity(mEditedSet.getId(), mSelectedPosition);
    }
    else if (itemId == R.id.set_delete)
    {
      mManager.deleteBookmark(mEditedSet.getId(), mSelectedPosition);
      ((BookmarkListAdapter) getListView().getAdapter()).notifyDataSetChanged();
    }
    else if (ShareAction.ACTIONS.containsKey(itemId))
    {
      final ShareAction shareAction = ShareAction.ACTIONS.get(itemId);
      final Bookmark bmk = mManager.getBookmark(mEditedSet.getId(), mSelectedPosition);

      String body;
      if (shareAction instanceof ShareAction.EmailShareAction)
      {
        body = getString(R.string.bookmark_share_email, bmk.getName(), bmk.getGe0Url());
      }
      else
      {
        // SMS specific text for all other cases
        // Because we don't know how much text we have,
        // So take shorter option.
        body = getString(R.string.bookmark_share_sms, bmk.getGe0Url());
      }

      final String subject = bmk.getName();
      shareAction.shareWithText(this, body, subject);
    }

    return super.onContextItemSelected(item);
  }

  @Override
  protected void onStart()
  {
    super.onStart();

    if (mPinAdapter != null)
      mPinAdapter.notifyDataSetChanged();
  }

  @Override
  protected void onPause()
  {
    assignCategoryParams();

    if (mPinAdapter != null)
      mPinAdapter.stopLocationUpdate();

    super.onPause();
  }

  @Override
  protected void onResume()
  {
    super.onResume();

    if (mPinAdapter != null)
      mPinAdapter.startLocationUpdate();
  }

  public void onSendEMail(View v)
  {
    assignCategoryParams();

    String path = ((MWMApplication) getApplication()).getTempPath();
    final String name = mManager.saveToKMZFile(mEditedSet.getId(), path);
    if (name == null)
    {
      // some error occured
      return;
    }

    final Intent intent = new Intent(Intent.ACTION_SEND);
    intent.setType("message/rfc822");
    intent.putExtra(android.content.Intent.EXTRA_SUBJECT, getString(R.string.share_bookmarks_email_subject));
    intent.putExtra(android.content.Intent.EXTRA_TEXT, String.format(getString(R.string.share_bookmarks_email_body), name));

    path = path + name + ".kmz";
    Log.d(TAG, "KMZ file path = " + path);
    intent.putExtra(Intent.EXTRA_STREAM, Uri.parse("file://" + path));

    try
    {
      startActivity(Intent.createChooser(intent, getString(R.string.share_by_email)));
    }
    catch (Exception ex)
    {
      Log.i(TAG, "Can't run E-Mail activity" + ex);
    }
  }
}
