package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.graphics.Point;
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
import android.widget.EditText;
import android.widget.CompoundButton.OnCheckedChangeListener;

import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkListAdapter.DataChangedListener;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.location.LocationService;

public class BookmarkListActivity extends AbstractBookmarkListActivity
{
  public static final String EDIT_CONTENT = "edit_content";

  private EditText mSetName;
  private CheckBox mIsVisible;
  private BookmarkCategory mEditedSet;
  private Bookmark mBookmark;
  private int mSelectedPosition;
  private BookmarkListAdapter mPinAdapter;
  private LocationService mLocation;
  private boolean mEditContent;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.pins);
    final int setIndex = getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1);
    mEditContent = getIntent().getBooleanExtra(EDIT_CONTENT, true);
    if ((mEditedSet = mManager.getCategoryById(setIndex)) == null)
    {
      Point bmk = ((ParcelablePoint)getIntent().getParcelableExtra(BookmarkActivity.PIN)).getPoint();
      mBookmark = mManager.getBookmark(bmk.x, bmk.y);
      setTitle(R.string.add_new_set);
    }
    else
    {
      setTitle(mEditedSet.getName());
    }

    mLocation = ((MWMApplication) getApplication()).getLocationService();

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
    setListAdapter(mPinAdapter = new BookmarkListAdapter(BookmarkListActivity.this, mEditedSet, new DataChangedListener()
    {
      @Override
      public void onDataChanged(int vis)
      {
        findViewById(R.id.bookmark_usage_hint).setVisibility(vis);
      }
    }));
    mLocation.startUpdate(mPinAdapter);
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
        if (mEditedSet == null)
        {
          mEditedSet = mManager.createCategory(mBookmark, s.toString());
          createListAdapter();
          setResult(RESULT_OK,
                    new Intent().putExtra(BookmarkActivity.PIN_SET, mManager.getCategoriesCount()-1).
                    putExtra(BookmarkActivity.PIN, new ParcelablePoint(mManager.getCategoriesCount()-1, mEditedSet.getSize()-1))
                    );
          mIsVisible.setChecked(true);
          mPinAdapter.notifyDataSetChanged();
        }
        else
        {
          mEditedSet.setName(s.toString());
        }
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
    startActivity(new Intent(this, BookmarkActivity.class).
                  putExtra(BookmarkActivity.PIN, new ParcelablePoint(cat, bmk)));
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
      mLocation.stopUpdate(mPinAdapter);
    super.onPause();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    if (mPinAdapter != null)
      mLocation.startUpdate(mPinAdapter);
  }
}
