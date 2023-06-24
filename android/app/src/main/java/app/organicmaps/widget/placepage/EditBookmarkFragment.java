package app.organicmaps.widget.placepage;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.ViewCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentFactory;
import androidx.fragment.app.FragmentManager;

import app.organicmaps.R;
import app.organicmaps.base.BaseMwmDialogFragment;
import app.organicmaps.bookmarks.ChooseBookmarkCategoryFragment;
import app.organicmaps.bookmarks.ChooseBookmarkCategoryFragment.Listener;
import app.organicmaps.bookmarks.data.BookmarkCategory;
import app.organicmaps.bookmarks.data.BookmarkInfo;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.Icon;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.UiUtils;

import java.util.List;

public class EditBookmarkFragment extends BaseMwmDialogFragment implements View.OnClickListener, Listener
{
  public static final String EXTRA_CATEGORY_ID = "CategoryId";
  public static final String EXTRA_BOOKMARK_ID = "BookmarkId";

  private EditText mEtDescription;
  private EditText mEtName;
  private TextView mTvBookmarkGroup;
  private ImageView mIvColor;
  private BookmarkCategory mBookmarkCategory;
  @Nullable
  private Icon mIcon;
  @Nullable
  private BookmarkInfo mBookmark;
  @Nullable
  private EditBookmarkListener mListener;

  public interface EditBookmarkListener
  {
    void onBookmarkSaved(long bookmarkId, boolean movedFromCategory);
  }

  public static void editBookmark(long categoryId, long bookmarkId, @NonNull Context context,
                                  @NonNull FragmentManager manager,
                                  @Nullable EditBookmarkListener listener)
  {
    final Bundle args = new Bundle();
    args.putLong(EXTRA_CATEGORY_ID, categoryId);
    args.putLong(EXTRA_BOOKMARK_ID, bookmarkId);
    String name = EditBookmarkFragment.class.getName();
    final FragmentFactory factory = manager.getFragmentFactory();
    final EditBookmarkFragment fragment = (EditBookmarkFragment) factory.instantiate(context.getClassLoader(), name);
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
    long categoryId = args.getLong(EXTRA_CATEGORY_ID);
    mBookmarkCategory = BookmarkManager.INSTANCE.getCategoryById(categoryId);
    long bookmarkId = args.getLong(EXTRA_BOOKMARK_ID);
    mBookmark = BookmarkManager.INSTANCE.getBookmarkInfo(bookmarkId);
    if (mBookmark != null)
      mIcon = mBookmark.getIcon();
    mEtName = view.findViewById(R.id.et__bookmark_name);
    mEtDescription = view.findViewById(R.id.et__description);
    mTvBookmarkGroup = view.findViewById(R.id.tv__bookmark_set);
    mTvBookmarkGroup.setOnClickListener(this);
    mIvColor = view.findViewById(R.id.iv__bookmark_color);
    mIvColor.setOnClickListener(this);
    refreshBookmark();
    initToolbar(view);
  }

  private void initToolbar(View view)
  {
    Toolbar toolbar = view.findViewById(R.id.toolbar);
    ViewCompat.setOnApplyWindowInsetsListener(toolbar, (v, windowInsets) -> {
      UiUtils.setViewInsetsPaddingNoBottom(v, windowInsets);
      return windowInsets;
    });
    final TextView textView = toolbar.findViewById(R.id.tv__save);
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
    boolean movedFromCategory = mBookmark.getCategoryId() != mBookmarkCategory.getId();
    if (movedFromCategory)
      BookmarkManager.INSTANCE.notifyCategoryChanging(mBookmark, mBookmarkCategory.getId());
    BookmarkManager.INSTANCE.notifyParametersUpdating(mBookmark, mEtName.getText().toString(),
                                                      mIcon, mEtDescription.getText().toString());

    if (mListener != null)
      mListener.onBookmarkSaved(mBookmark.getBookmarkId(), movedFromCategory);
    dismiss();
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.iv__bookmark_color)
      selectBookmarkColor();
    else if (id == R.id.tv__bookmark_set)
      selectBookmarkSet();
  }

  private void selectBookmarkSet()
  {
    if (mBookmark == null)
      return;

    final Bundle args = new Bundle();
    final List<BookmarkCategory> categories = BookmarkManager.INSTANCE.getCategories();
    final int index = categories.indexOf(mBookmarkCategory);
    args.putInt(ChooseBookmarkCategoryFragment.CATEGORY_POSITION, index);

    final FragmentManager manager = getChildFragmentManager();
    String className = ChooseBookmarkCategoryFragment.class.getName();
    final FragmentFactory factory = manager.getFragmentFactory();
    final ChooseBookmarkCategoryFragment frag =
        (ChooseBookmarkCategoryFragment) factory.instantiate(getContext().getClassLoader(), className);
    frag.setArguments(args);
    frag.show(manager, null);
  }

  private void selectBookmarkColor()
  {
    if (mIcon == null)
      return;

    final Bundle args = new Bundle();
    args.putInt(BookmarkColorDialogFragment.ICON_TYPE, mIcon.getColor());

    final FragmentManager manager = getChildFragmentManager();
    String className = BookmarkColorDialogFragment.class.getName();
    final FragmentFactory factory = manager.getFragmentFactory();
    final BookmarkColorDialogFragment dialogFragment =
        (BookmarkColorDialogFragment) factory.instantiate(getContext().getClassLoader(), className);
    dialogFragment.setArguments(args);
    dialogFragment.setOnColorSetListener(colorPos -> {
      final Icon newIcon = BookmarkManager.ICONS.get(colorPos);
      final String from = mIcon.getName();
      final String to = newIcon.getName();
      if (TextUtils.equals(from, to))
        return;

      mIcon = newIcon;
      refreshColorMarker();
    });

    dialogFragment.show(requireActivity().getSupportFragmentManager(), null);
  }

  private void refreshColorMarker()
  {
    if (mIcon != null)
    {
      Drawable circle = Graphics.drawCircleAndImage(mIcon.argb(),
                                                    R.dimen.track_circle_size,
                                                    R.drawable.ic_bookmark_none,
                                                    R.dimen.bookmark_icon_size,
                                                    requireContext());
      mIvColor.setImageDrawable(circle);
    }
  }

  private void refreshCategory()
  {
    mTvBookmarkGroup.setText(mBookmarkCategory.getName());
  }

  private void refreshBookmark()
  {
    if (mBookmark == null)
      return;

    if (TextUtils.isEmpty(mEtName.getText()))
      mEtName.setText(mBookmark.getName());

    if (TextUtils.isEmpty(mEtDescription.getText()))
    {
      mEtDescription.setText(
          BookmarkManager.INSTANCE.getBookmarkDescription(mBookmark.getBookmarkId()));
    }
    refreshCategory();
    refreshColorMarker();
  }

  @Override
  public void onCategoryChanged(BookmarkCategory newCategory)
  {
    mBookmarkCategory = newCategory;
    refreshCategory();
  }

  public void setEditBookmarkListener(@Nullable EditBookmarkListener listener)
  {
    mListener = listener;
  }
}
