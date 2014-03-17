package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.content.Intent;
import android.graphics.Point;
import android.os.Bundle;
import android.text.Editable;
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
  private FooterHandler m_handler;

  private Bookmark getBookmarkFromIntent()
  {
    // Note that Point result from the intent is actually a pair
    // of (category index, bookmark index in category).
    final Point cab = ((ParcelablePoint) getIntent().getParcelableExtra(BookmarkActivity.PIN)).getPoint();
    return mManager.getBookmark(cab.x, cab.y);
  }

  @Override
  protected ChooseBookmarkCategoryAdapter getAdapter()
  {
    return (ChooseBookmarkCategoryAdapter) ((HeaderViewListAdapter) getListView().getAdapter()).getWrappedAdapter();
  }

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
        getAdapter().chooseItem(position);

        final Bookmark bmk = getBookmarkFromIntent();
        bmk.setCategoryId(position);
        getIntent().putExtra(BookmarkActivity.PIN,
            new ParcelablePoint(bmk.getCategoryId(), bmk.getBookmarkId()));

        onBackPressed();
      }
    });

    // Set adapter only after FooterHandler is initialized and added into layout.
    setListAdapter(new ChooseBookmarkCategoryAdapter(this, getIntent().getIntExtra(BookmarkActivity.PIN_SET, 0)));

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
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (resultCode == RESULT_OK)
    {
      getAdapter().chooseItem(data.getIntExtra(BookmarkActivity.PIN_SET, 0));

      getIntent().putExtra(BookmarkActivity.PIN, data.getParcelableExtra(BookmarkActivity.PIN));
    }

    super.onActivityResult(requestCode, resultCode, data);
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
      final Editable e = mNewName.getText();
      if (e.length() > 0)
      {
        createNewCategory(e.toString());

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
          onBackPressed();
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
              (event.getAction() == KeyEvent.ACTION_DOWN &&
               event.getKeyCode() == KeyEvent.KEYCODE_ENTER))
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
      final int index = mManager.createCategory(getBookmarkFromIntent(), name);

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
