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
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.HeaderViewListAdapter;
import android.widget.ImageButton;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.util.statistics.Statistics;

public class ChooseBookmarkCategoryActivity extends AbstractBookmarkCategoryActivity
{
  private static final int REQUEST_CREATE_CATEGORY = 1000;
  private ChooseBookmarkCategoryAdapter mAdapter;
  private FooterHandler m_handler;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    m_handler = new FooterHandler();
    getListView().setOnItemClickListener(new OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        m_handler.switchToAddButton();
        mAdapter.chooseItem(position);

        // Note that Point result from the intent is actually a pair of (category index, bookmark index in category).
        final Point cab = ((ParcelablePoint)getIntent().getParcelableExtra(BookmarkActivity.PIN)).getPoint();
        Bookmark bmk = mManager.getBookmark(cab.x, cab.y);
        bmk.setCategoryId(position);
        getIntent().putExtra(BookmarkActivity.PIN, new ParcelablePoint(position, bmk.getBookmarkId()));
      }
    });
    setListAdapter(mAdapter = new ChooseBookmarkCategoryAdapter(this, getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1)));
    registerForContextMenu(getListView());
  }

  @Override
  public void onBackPressed()
  {
    m_handler.createCategoryIfNeeded();
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

    private void createCategory()
    {
      if (mNewName.getText().length() > 0)
      {
        createNewCategory(mNewName.getText().toString());
        mImm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
      }
    }

    public void createCategoryIfNeeded()
    {
      if (mAddButton.getVisibility() == View.INVISIBLE &&
          mNewLayout.getVisibility() == View.VISIBLE)
      {
        createCategory();
      }
    }

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
          createCategory();
        }
      });

      mCancel = (ImageButton)mRootView.findViewById(R.id.chs_footer_cancel_button);
      mCancel.setOnClickListener(this);

      mNewLayout = mRootView.findViewById(R.id.chs_footer_new_layout);
      mNewName.setOnEditorActionListener(new EditText.OnEditorActionListener()
      {
        @Override
        public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
        {
          if (actionId == EditorInfo.IME_ACTION_DONE ||
              (event.getAction() == KeyEvent.ACTION_DOWN && event.getKeyCode() == KeyEvent.KEYCODE_ENTER))
          {
            createCategory();
            return true;
          }
          return false;
        }
      });
    }

    private void createNewCategory(String name)
    {
      // Note that Point result from the intent is actually a pair of (category index, bookmark index in category).
      final Point cab = ((ParcelablePoint)getIntent().getParcelableExtra(BookmarkActivity.PIN)).getPoint();
      final int index = mManager.createCategory(mManager.getBookmark(cab.x, cab.y), name);

      getIntent().putExtra(BookmarkActivity.PIN_SET, index)
      .putExtra(BookmarkActivity.PIN, new ParcelablePoint(index, 0));

      switchToAddButton();

      getAdapter().chooseItem(index);

      Statistics.INSTANCE.trackGroupCreated(ChooseBookmarkCategoryActivity.this);
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
