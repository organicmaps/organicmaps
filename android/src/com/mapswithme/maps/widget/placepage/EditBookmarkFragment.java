package com.mapswithme.maps.widget.placepage;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.bookmarks.ChooseBookmarkCategoryFragment;
import com.mapswithme.maps.bookmarks.ChooseBookmarkCategoryFragment.Listener;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Icon;
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
  private int mCategoryId;
  @Nullable
  private Icon mIcon;
  @Nullable
  private Bookmark mBookmark;
  @Nullable
  private EditBookmarkListener mListener;

  public interface EditBookmarkListener
  {
    void onBookmarkSaved(int categoryId, int bookmarkId);
  }

  public static void editBookmark(int categoryId, int bookmarkId, @NonNull Context context,
                                  @NonNull FragmentManager manager,
                                  @Nullable EditBookmarkListener listener)
  {
    final Bundle args = new Bundle();
    args.putInt(EXTRA_CATEGORY_ID, categoryId);
    args.putInt(EXTRA_BOOKMARK_ID, bookmarkId);
    String name = EditBookmarkFragment.class.getName();
    final EditBookmarkFragment fragment = (EditBookmarkFragment) Fragment.instantiate(context, name, args);
    fragment.setArguments(args);
    fragment.setEditBookmarkListener(listener);
    fragment.show(manager, name);
  }

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
  public void onViewCreated(@NonNull View view, Bundle savedInstanceState)
  {
    final Bundle args = getArguments();
    mCategoryId = args.getInt(EXTRA_CATEGORY_ID);
    int bookmarkId = args.getInt(EXTRA_BOOKMARK_ID);
    mBookmark = BookmarkManager.INSTANCE.getBookmark(mCategoryId, bookmarkId);
    mIcon = mBookmark.getIcon();
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
    UiUtils.extendViewWithStatusBar(toolbar);
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
    if (mBookmark == null)
    {
      dismiss();
      return;
    }
    if (mBookmark.getCategoryId() != mCategoryId)
    {
      mBookmark.setCategoryId(mCategoryId);
      Framework.nativeOnBookmarkCategoryChanged(mBookmark.getCategoryId(), mBookmark.getBookmarkId());
    }
    mBookmark.setParams(mEtName.getText().toString(), mIcon, mEtDescription.getText().toString());

    if (mListener != null)
      mListener.onBookmarkSaved(mBookmark.getCategoryId(), mBookmark.getBookmarkId());
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
    if (mBookmark == null)
      return;

    final Bundle args = new Bundle();
    args.putInt(ChooseBookmarkCategoryFragment.CATEGORY_ID, mCategoryId);
    final ChooseBookmarkCategoryFragment fragment = (ChooseBookmarkCategoryFragment) Fragment.instantiate(getActivity(), ChooseBookmarkCategoryFragment.class.getName(), args);
    fragment.show(getChildFragmentManager(), null);
  }

  private void selectBookmarkColor()
  {
    if (mIcon == null)
      return;

    final Bundle args = new Bundle();
    args.putString(BookmarkColorDialogFragment.ICON_TYPE, mIcon.getType());
    final BookmarkColorDialogFragment dialogFragment = (BookmarkColorDialogFragment) BookmarkColorDialogFragment.
        instantiate(getActivity(), BookmarkColorDialogFragment.class.getName(), args);

    dialogFragment.setOnColorSetListener(new BookmarkColorDialogFragment.OnBookmarkColorChangeListener()
    {
      @Override
      public void onBookmarkColorSet(int colorPos)
      {
        final Icon newIcon = BookmarkManager.ICONS.get(colorPos);
        final String from = mIcon.getName();
        final String to = newIcon.getName();
        if (TextUtils.equals(from, to))
          return;

        Statistics.INSTANCE.trackColorChanged(from, to);
        mIcon = newIcon;
        refreshColorMarker();
      }
    });

    dialogFragment.show(getActivity().getSupportFragmentManager(), null);
  }

  private void refreshColorMarker()
  {
    if (mIcon != null)
      mIvColor.setImageResource(mIcon.getSelectedResId());
  }

  private void refreshCategory()
  {
    mTvBookmarkGroup.setText(BookmarkManager.INSTANCE.getCategory(mCategoryId).getName());
  }

  private void refreshBookmark()
  {
    if (mBookmark == null)
      return;

    if (TextUtils.isEmpty(mEtName.getText()))
      mEtName.setText(mBookmark.getTitle());

    if (TextUtils.isEmpty(mEtDescription.getText()))
      mEtDescription.setText(mBookmark.getBookmarkDescription());
    refreshCategory();
    refreshColorMarker();
  }

  @Override
  public void onCategoryChanged(int newCategoryId)
  {
    mCategoryId = newCategoryId;
    refreshCategory();
  }

  public void setEditBookmarkListener(@Nullable EditBookmarkListener listener)
  {
    mListener = listener;
  }
}
