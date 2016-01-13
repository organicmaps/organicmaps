package com.mapswithme.maps.widget.placepage;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.bookmarks.ChooseBookmarkCategoryFragment;
import com.mapswithme.maps.bookmarks.ChooseBookmarkCategoryFragment.Listener;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

public class EditBookmarkFragment extends BaseMwmDialogFragment implements View.OnClickListener, Listener
{
  public static final String EXTRA_CATEGORY_ID = "CategoryId";
  public static final String EXTRA_BOOKMARK_ID = "BookmarkId";

  private EditText mEtDescription;
  private EditText mEtName;
  private TextView mTvBookmarkGroup;
  private ImageView mIvColor;
  private Bookmark mBookmark;

  public EditBookmarkFragment() {}

  @Override
  protected int getCustomTheme()
  {
    return getFullscreenTheme();
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_edit_bookmark, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    final Bundle args = getArguments();
    int categoryId = args.getInt(EXTRA_CATEGORY_ID);
    int bookmarkId = args.getInt(EXTRA_BOOKMARK_ID);
    mBookmark = BookmarkManager.INSTANCE.getBookmark(categoryId, bookmarkId);

    mEtName = (EditText) view.findViewById(R.id.et__bookmark_name);
    mEtDescription = (EditText) view.findViewById(R.id.et__description);
    mTvBookmarkGroup = (TextView) view.findViewById(R.id.tv__bookmark_set);
    mTvBookmarkGroup.setOnClickListener(this);
    mIvColor = (ImageView) view.findViewById(R.id.iv__bookmark_color);
    mIvColor.setOnClickListener(this);
    refreshBookmark();
    initToolbar(view);
  }

  private void initToolbar(View view)
  {
    Toolbar toolbar = (Toolbar) view.findViewById(R.id.toolbar);
    final TextView textView = (TextView) toolbar.findViewById(R.id.tv__save);
    textView.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        saveBookmark();
      }
    });
    UiUtils.showHomeUpButton(toolbar);
    toolbar.setTitle(R.string.description);
    toolbar.setNavigationOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dismiss();
      }
    });
  }

  private void saveBookmark()
  {
    mBookmark.setParams(mEtName.getText().toString(), null, mEtDescription.getText().toString());
    dismiss();
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.iv__bookmark_color:
      selectBookmarkColor();
      break;
    case R.id.tv__bookmark_set:
      selectBookmarkSet();
      break;
    }
  }

  private void selectBookmarkSet()
  {
    final Bundle args = new Bundle();
    args.putInt(ChooseBookmarkCategoryFragment.CATEGORY_ID, mBookmark.getCategoryId());
    args.putInt(ChooseBookmarkCategoryFragment.BOOKMARK_ID, mBookmark.getBookmarkId());
    final ChooseBookmarkCategoryFragment fragment = (ChooseBookmarkCategoryFragment) Fragment.instantiate(getActivity(), ChooseBookmarkCategoryFragment.class.getName(), args);
    fragment.show(getChildFragmentManager(), null);
  }

  private void selectBookmarkColor()
  {
    final Bundle args = new Bundle();
    args.putString(BookmarkColorDialogFragment.ICON_TYPE, mBookmark.getIcon().getType());
    final BookmarkColorDialogFragment dialogFragment = (BookmarkColorDialogFragment) BookmarkColorDialogFragment.
        instantiate(getActivity(), BookmarkColorDialogFragment.class.getName(), args);

    dialogFragment.setOnColorSetListener(new BookmarkColorDialogFragment.OnBookmarkColorChangeListener()
    {
      @Override
      public void onBookmarkColorSet(int colorPos)
      {
        final Icon newIcon = BookmarkManager.getIcons().get(colorPos);
        final String from = mBookmark.getIcon().getName();
        final String to = newIcon.getName();
        if (!TextUtils.equals(from, to))
          Statistics.INSTANCE.trackColorChanged(from, to);

        mBookmark.setParams(mBookmark.getName(), newIcon, mBookmark.getBookmarkDescription());
        mBookmark = BookmarkManager.INSTANCE.getBookmark(mBookmark.getCategoryId(), mBookmark.getBookmarkId());
        refreshColorMarker();
      }
    });

    dialogFragment.show(getActivity().getSupportFragmentManager(), null);
  }

  private void refreshColorMarker()
  {
    mIvColor.setImageResource(mBookmark.getIcon().getSelectedResId());
  }

  private void refreshBookmark()
  {
    mEtName.setText(mBookmark.getName());
    mEtName.selectAll();
    InputUtils.showKeyboard(mEtName);

    mEtDescription.setText(mBookmark.getBookmarkDescription());
    mTvBookmarkGroup.setText(mBookmark.getCategoryName(getActivity()));
    refreshColorMarker();
  }

  @Override
  public void onCategoryChanged(int bookmarkId, int newCategoryId)
  {
    mBookmark = BookmarkManager.INSTANCE.getBookmark(newCategoryId, bookmarkId);
    refreshBookmark();
  }
}
