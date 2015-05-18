package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.graphics.Point;
import android.os.Bundle;
import android.text.Editable;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.util.statistics.Statistics;

public class ChooseBookmarkCategoryFragment extends BaseMwmListFragment
{
  private FooterHelper mFooterHelper;
  private ChooseBookmarkCategoryAdapter mAdapter;

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    final ListView listView = getListView();
    mFooterHelper = new FooterHelper(listView);
    // Set adapter only after FooterHandler is initialized and added into layout.
    mAdapter = new ChooseBookmarkCategoryAdapter(getActivity(), getArguments().getInt(ChooseBookmarkCategoryActivity.PIN_SET, 0));
    setListAdapter(mAdapter);
    registerForContextMenu(listView);
  }

  @Override
  public void onListItemClick(ListView l, View v, int position, long id)
  {
    mFooterHelper.switchToAddButton();
    mAdapter.chooseItem(position);

    final Bookmark bmk = getBookmarkFromIntent();
    bmk.setCategoryId(position);
    getActivity().getIntent().putExtra(ChooseBookmarkCategoryActivity.PIN,
        new ParcelablePoint(bmk.getCategoryId(), bmk.getBookmarkId()));

    getActivity().onBackPressed();
  }

  private Bookmark getBookmarkFromIntent()
  {
    // Note that Point result from the intent is actually a pair
    // of (category index, bookmark index in category).
    final Point cab = ((ParcelablePoint) getArguments().getParcelable(ChooseBookmarkCategoryActivity.PIN)).getPoint();
    return BookmarkManager.INSTANCE.getBookmark(cab.x, cab.y);
  }

  private class FooterHelper implements View.OnClickListener
  {
    View mRootView;
    EditText mNewName;
    Button mAddButton;
    View mNewLayout;

    InputMethodManager mImm = (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);

    public FooterHelper(ViewGroup root)
    {
      mRootView = getActivity().getLayoutInflater().inflate(R.layout.choose_category_footer, root, false);
      getListView().addFooterView(mRootView);

      mAddButton = (Button) mRootView.findViewById(R.id.chs_footer_button);
      mAddButton.setOnClickListener(this);
      mNewLayout = mRootView.findViewById(R.id.chs_footer_new_layout);
      mNewLayout.findViewById(R.id.chs_footer_create_button).setOnClickListener(this);
      mNewLayout.findViewById(R.id.chs_footer_cancel_button).setOnClickListener(this);
      mNewName = (EditText) mNewLayout.findViewById(R.id.chs_footer_field);
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

    private void createCategory()
    {
      final Editable e = mNewName.getText();
      if (e.length() > 0)
      {
        createCategory(e.toString());
        mImm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
      }
    }

    private void createCategory(String name)
    {
      final int category = BookmarkManager.INSTANCE.createCategory(name);
      getBookmarkFromIntent().setCategoryId(category);

      getActivity().getIntent().putExtra(ChooseBookmarkCategoryActivity.PIN_SET, category)
          .putExtra(ChooseBookmarkCategoryActivity.PIN, new ParcelablePoint(category, 0));

      getArguments().putInt(ChooseBookmarkCategoryActivity.PIN_SET, category);
      getArguments().putParcelable(ChooseBookmarkCategoryActivity.PIN, new ParcelablePoint(category, 0));

      switchToAddButton();

      mAdapter.chooseItem(category);

      Statistics.INSTANCE.trackGroupCreated();
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
      switch (v.getId())
      {
      case R.id.chs_footer_create_button:
        createCategory();
        getActivity().onBackPressed();
        break;
      case R.id.chs_footer_button:
        mAddButton.setVisibility(View.INVISIBLE);
        mNewLayout.setVisibility(View.VISIBLE);
        mNewName.requestFocus();
        mImm.showSoftInput(mNewName, InputMethodManager.SHOW_IMPLICIT);
        break;
      case R.id.chs_footer_cancel_button:
        switchToAddButton();
        break;
      }
    }
  }

}
