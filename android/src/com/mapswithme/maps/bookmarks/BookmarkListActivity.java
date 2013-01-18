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
import android.widget.EditText;
import android.widget.CompoundButton.OnCheckedChangeListener;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public class BookmarkListActivity extends AbstractBookmarkListActivity
{

  private EditText mSetName;
  private CheckBox mIsVisible;
  private BookmarkCategory mEditedSet;
  private int mSelectedPosition;
  private BookmarkAdapter mPinAdapter;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.pins);
    if ((mEditedSet = mManager.getCategoryById(getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1))) != null)
    {
      setListAdapter(mPinAdapter = new BookmarkAdapter(this, mEditedSet));
      setUpViews();
      getListView().setOnItemClickListener(new OnItemClickListener()
      {

        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id)
        {
          startPinActivity((Bookmark) (getListView().getAdapter()).getItem(mSelectedPosition));
        }
      });
    }
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

  private void startPinActivity(Bookmark pin)
  {
    startActivity(new Intent(this, BookmarkActivity.class).putExtra(BookmarkActivity.PIN, mManager.getPinId(pin)));
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    Bookmark pin = (Bookmark) (getListView().getAdapter()).getItem(mSelectedPosition);
    int itemId = item.getItemId();
    if (itemId == R.id.set_edit)
    {
      startPinActivity(pin);
    }
    else if (itemId == R.id.set_delete)
    {
     // mManager.deletePin(pin);
      ((BookmarkAdapter) getListView().getAdapter()).notifyDataSetChanged();
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
