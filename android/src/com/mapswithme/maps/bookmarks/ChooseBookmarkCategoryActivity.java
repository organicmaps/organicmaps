package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.content.Intent;
import android.graphics.Point;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnKeyListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.HeaderViewListAdapter;
import android.widget.ImageButton;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;

public class ChooseBookmarkCategoryActivity extends AbstractBookmarkCategoryActivity
{
  private static final int REQUEST_CREATE_CATEGORY = 1000;
  private ChooseBookmarkCategoryAdapter mAdapter;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    final FooterHandler handler = new FooterHandler();
    getListView().setOnItemClickListener(new OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        handler.switchToAddButton();
        mAdapter.chooseItem(position);
        Point cab = ((ParcelablePoint)getIntent().getParcelableExtra(BookmarkActivity.PIN)).getPoint();
        BookmarkCategory cat = mManager.getCategoryById(position);
        mManager.getBookmark(cab.x, cab.y).setCategory(cat.getName(), position);
        getIntent().putExtra(BookmarkActivity.PIN, new ParcelablePoint(position, cat.getSize()-1));
      }
    });
    setListAdapter(mAdapter = new ChooseBookmarkCategoryAdapter(this, getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1)));
    registerForContextMenu(getListView());
  }

  @Override
  public void onBackPressed()
  {
    setResult(RESULT_OK, new Intent().putExtra(BookmarkActivity.PIN, getIntent().getParcelableExtra(BookmarkActivity.PIN)));
    super.onBackPressed();
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v,
                                  ContextMenuInfo menuInfo)
  {
    getMenuInflater().inflate(R.menu.choose_pin_sets_context_menu, menu);
    super.onCreateContextMenu(menu, v, menuInfo);
  }

  @Override
  protected ChooseBookmarkCategoryAdapter getAdapter()
  {
    return (ChooseBookmarkCategoryAdapter)((HeaderViewListAdapter) getListView().getAdapter()).getWrappedAdapter();
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (requestCode == REQUEST_CREATE_CATEGORY && resultCode == RESULT_OK)
    {
      mAdapter.chooseItem(data.getIntExtra(BookmarkActivity.PIN_SET, 0));
      getIntent().putExtra(BookmarkActivity.PIN, data.getParcelableExtra(BookmarkActivity.PIN));
    }
    super.onActivityResult(requestCode, resultCode, data);
  }

  @Override
  protected boolean enableEditing()
  {
    return false;
  }

  private class FooterHandler implements View.OnClickListener
  {
    View mRootView;
    EditText mNewName;
    Button mAddButton;
    ImageButton mCancel;
    View mNewLayout;
    InputMethodManager mImm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);

    public FooterHandler()
    {
      mRootView = getLayoutInflater().inflate(R.layout.choose_category_footer, null);
      getListView().addFooterView(mRootView);
      mNewName = (EditText)mRootView.findViewById(R.id.chs_footer_field);
      mAddButton = (Button)mRootView.findViewById(R.id.chs_footer_button);
      mAddButton.setOnClickListener(this);
      mRootView.findViewById(R.id.chs_footer_create_button).setOnClickListener(new OnClickListener()
      {

        @Override
        public void onClick(View v)
        {
          if ( mNewName.getText().length() > 0) {
            createNewCategory(mNewName.getText().toString());
            mImm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
          }
        }
      });
      mCancel = (ImageButton)mRootView.findViewById(R.id.chs_footer_cancel_button);
      mCancel.setOnClickListener(this);
      mNewLayout = mRootView.findViewById(R.id.chs_footer_new_layout);
      mNewName.setOnKeyListener(new OnKeyListener() {
        public boolean onKey(View v, int keyCode, KeyEvent event) {
            // If the event is a key-down event on the "enter" button
            if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                (keyCode == KeyEvent.KEYCODE_ENTER && mNewName.getText().length() > 0)) {
              // Perform action on key press
              createNewCategory(mNewName.getText().toString());
              return true;
            }
            return false;
        }
    });
    }

    private void createNewCategory(String name)
    {
      Point cab = ((ParcelablePoint)getIntent().getParcelableExtra(BookmarkActivity.PIN)).getPoint();
      mManager.createCategory(mManager.getBookmark(cab.x, cab.y), name);
      getIntent().putExtra(BookmarkActivity.PIN_SET, mManager.getCategoriesCount()-1).
      putExtra(BookmarkActivity.PIN, new ParcelablePoint(mManager.getCategoriesCount()-1, 0));
      switchToAddButton();
      getAdapter().chooseItem(mManager.getCategoriesCount()-1);
    }

    private void switchToAddButton()
    {
      if (mAddButton.getVisibility() != View.VISIBLE)
      {
        mNewName.setText("");
        mAddButton.setVisibility(View.VISIBLE);
        mNewLayout.setVisibility(View.INVISIBLE);
      }
    }

    @Override
    public void onClick(View v)
    {
      if (v.getId() == R.id.chs_footer_button)
      {
        mAddButton.setVisibility(View.INVISIBLE);
        mNewLayout.setVisibility(View.VISIBLE);
        mNewName.requestFocus();
        mImm.showSoftInput(mNewName, InputMethodManager.SHOW_IMPLICIT);
      }
      else if (v.getId() == R.id.chs_footer_cancel_button)
      {
        switchToAddButton();
      }
    }
  }
}
