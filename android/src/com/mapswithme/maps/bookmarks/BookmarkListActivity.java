package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;

import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkListAdapter.DataChangedListener;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;


public class BookmarkListActivity extends AbstractBookmarkListActivity
{
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
        final String name = s.toString();
        mManager.setCategoryName(mEditedSet, name);
        setTitle(name);
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
    mIsVisible.setOnCheckedChangeListener(new OnCheckedChangeListener()
    {
      @Override
      public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
      {
        if (mEditedSet != null)
          mEditedSet.setVisibility(isChecked);
      }
    });
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    if (mEditContent)
    {
      if (menuInfo instanceof AdapterView.AdapterContextMenuInfo)
      {
        AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
        mSelectedPosition = info.position;
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.pin_sets_context_menu, menu);
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
}
