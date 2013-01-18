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

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;

public class BookmarkListActivity extends AbstractBookmarkListActivity
{

  private EditText mSetName;
  private CheckBox mIsVisible;
  private BookmarkCategory mEditedSet;
  private int mSelectedPosition;
  private BookmarkListAdapter mPinAdapter;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.pins);
    final int setIndex = getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1);
    if ((mEditedSet = mManager.getCategoryById(setIndex)) == null)
    {
      Point bmk = ((ParcelablePoint)getIntent().getParcelableExtra(BookmarkActivity.PIN)).getPoint();
      mEditedSet = mManager.createCategory(mManager.getBookmark(bmk.x, bmk.y));
      setResult(RESULT_OK,
                new Intent().putExtra(BookmarkActivity.PIN_SET, mManager.getCategoriesCount()-1).
                putExtra(BookmarkActivity.PIN, new ParcelablePoint(mManager.getCategoriesCount()-1, mEditedSet.getSize()-1))
                );
      setTitle(R.string.add_new_set);
    }
    else
    {
      setTitle(mEditedSet.getName());
    }
    setListAdapter(mPinAdapter = new BookmarkListAdapter(this, mEditedSet));
    setUpViews();
    getListView().setOnItemClickListener(new OnItemClickListener()
    {

      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        startPinActivity(setIndex, position);
      }
    });
    registerForContextMenu(getListView());
  }

  private void setUpViews()
  {
    mSetName = (EditText) findViewById(R.id.pin_set_name);
    mSetName.setText(mEditedSet.getName());
    mSetName.addTextChangedListener(new TextWatcher()
    {

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        mEditedSet.setName(s.toString());
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
    mIsVisible.setChecked(mEditedSet.isVisible());
    mIsVisible.setOnCheckedChangeListener(new OnCheckedChangeListener()
    {

      @Override
      public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
      {
        mEditedSet.setVisibility(isChecked);
      }
    });
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    if (menuInfo instanceof AdapterView.AdapterContextMenuInfo)
    {
      AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
      mSelectedPosition = info.position;
      MenuInflater inflater = getMenuInflater();
      inflater.inflate(R.menu.pin_sets_context_menu, menu);
    }
    super.onCreateContextMenu(menu, v, menuInfo);
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
    else
    {
    }
    return super.onContextItemSelected(item);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    mPinAdapter.notifyDataSetChanged();
  }
}
