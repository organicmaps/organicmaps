package app.organicmaps.bookmarks;

import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.MenuProvider;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.DataChangedListener;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.BookmarkColorDialogFragment;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;
import java.util.Objects;

public class BookmarkCategorySettingsFragment
    extends BaseMwmToolbarFragment implements BookmarkColorDialogFragment.OnBookmarkColorChangeListener
{
  private static final int TEXT_LENGTH_LIMIT = 60;
  private static final String EXTRA_COLOR_PICKER_TYPE = "color_picker_type";

  private enum ColorPickerType
  {
    BOOKMARK,
    TRACK
  }

  private ColorPickerType mColorPickerType = ColorPickerType.BOOKMARK;

  @NonNull
  private final DataChangedListener mCategoriesListener = this::onCategoriesChanged;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private BookmarkCategory mCategory;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private TextInputEditText mEditDescView;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private TextInputEditText mEditCategoryNameView;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private View mColorBookmarksBtn;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private View mColorTracksBtn;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private View mColorSectionDivider;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private View mColorSectionSpacer;

  @NonNull
  private final MenuProvider mMenuProvider = new MenuProvider() {
    @Override
    public void onCreateMenu(@NonNull Menu menu, @NonNull MenuInflater menuInflater)
    {
      menuInflater.inflate(R.menu.menu_done, menu);
    }

    @Override
    public boolean onMenuItemSelected(@NonNull MenuItem menuItem)
    {
      if (menuItem.getItemId() == R.id.done)
      {
        onEditDoneClicked();
        return true;
      }
      return false;
    }
  };

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    final Bundle args = requireArguments();
    mCategory = Objects.requireNonNull(
        Utils.getParcelable(args, BookmarkCategorySettingsActivity.EXTRA_BOOKMARK_CATEGORY, BookmarkCategory.class));
    if (savedInstanceState != null)
      mColorPickerType =
          ColorPickerType
              .values()[savedInstanceState.getInt(EXTRA_COLOR_PICKER_TYPE, ColorPickerType.BOOKMARK.ordinal())];
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putInt(EXTRA_COLOR_PICKER_TYPE, mColorPickerType.ordinal());
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addCategoriesUpdatesListener(mCategoriesListener);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(mCategoriesListener);
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    final View root = inflater.inflate(R.layout.fragment_bookmark_category_settings, container, false);
    requireActivity().addMenuProvider(mMenuProvider, getViewLifecycleOwner());
    initViews(root);
    return root;
  }

  private void initViews(@NonNull View root)
  {
    mEditCategoryNameView = root.findViewById(R.id.edit_list_name_view);
    TextInputLayout clearNameBtn = root.findViewById(R.id.edit_list_name_input);
    clearNameBtn.setEndIconOnClickListener(v -> clearAndFocus(mEditCategoryNameView));
    mEditCategoryNameView.setText(mCategory.getName());
    InputFilter[] f = {new InputFilter.LengthFilter(TEXT_LENGTH_LIMIT)};
    mEditCategoryNameView.setFilters(f);
    mEditCategoryNameView.requestFocus();
    mEditCategoryNameView.addTextChangedListener(new TextWatcher() {
      @Override
      public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2)
      {}

      @Override
      public void onTextChanged(CharSequence charSequence, int i, int i1, int i2)
      {
        clearNameBtn.setEndIconVisible(charSequence.length() > 0);
      }

      @Override
      public void afterTextChanged(Editable editable)
      {}
    });
    mEditDescView = root.findViewById(R.id.edit_description);
    mEditDescView.setText(mCategory.getDescription());

    mColorBookmarksBtn = root.findViewById(R.id.color_bookmarks_btn);
    mColorBookmarksBtn.setOnClickListener(v -> {
      mColorPickerType = ColorPickerType.BOOKMARK;
      showColorPicker();
    });
    mColorTracksBtn = root.findViewById(R.id.color_tracks_btn);
    mColorTracksBtn.setOnClickListener(v -> {
      mColorPickerType = ColorPickerType.TRACK;
      showColorPicker();
    });
    mColorSectionDivider = root.findViewById(R.id.color_section_divider);
    mColorSectionSpacer = root.findViewById(R.id.color_section_spacer);

    updateColorButtonsVisibility();
  }

  private void updateColorButtonsVisibility()
  {
    final BookmarkCategory category = BookmarkManager.INSTANCE.getCategoryById(mCategory.getId());
    final int bookmarksCount = category.getBookmarksCount();
    final int tracksCount = category.getTracksCount();

    mColorBookmarksBtn.setVisibility(bookmarksCount > 0 ? View.VISIBLE : View.GONE);
    mColorTracksBtn.setVisibility(tracksCount > 0 ? View.VISIBLE : View.GONE);

    final boolean hasContent = bookmarksCount > 0 || tracksCount > 0;
    mColorSectionDivider.setVisibility(hasContent ? View.VISIBLE : View.GONE);
    mColorSectionSpacer.setVisibility(hasContent ? View.VISIBLE : View.GONE);
  }

  private void onEditDoneClicked()
  {
    final String newCategoryName = getEditableCategoryName();
    if (!validateCategoryName(newCategoryName))
      return;

    if (isCategoryNameChanged())
      mCategory.setName(newCategoryName);

    if (isCategoryDescChanged())
      mCategory.setDescription(getEditableCategoryDesc());

    requireActivity().finish();
  }

  private boolean isCategoryNameChanged()
  {
    String categoryName = getEditableCategoryName();
    return !TextUtils.equals(categoryName, mCategory.getName());
  }

  private boolean validateCategoryName(@Nullable String name)
  {
    if (TextUtils.isEmpty(name))
    {
      new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
          .setTitle(R.string.bookmarks_error_title_empty_list_name)
          .setMessage(R.string.bookmarks_error_message_empty_list_name)
          .setPositiveButton(R.string.ok, null)
          .show();
      return false;
    }

    if (BookmarkManager.INSTANCE.isUsedCategoryName(name) && !TextUtils.equals(name, mCategory.getName()))
    {
      new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
          .setTitle(R.string.bookmarks_error_title_list_name_already_taken)
          .setMessage(R.string.bookmarks_error_message_list_name_already_taken)
          .setPositiveButton(R.string.ok, null)
          .show();
      return false;
    }
    return true;
  }

  @NonNull
  private String getEditableCategoryName()
  {
    return mEditCategoryNameView.getEditableText().toString().trim();
  }

  @NonNull
  private String getEditableCategoryDesc()
  {
    return mEditDescView.getEditableText().toString().trim();
  }

  private boolean isCategoryDescChanged()
  {
    String categoryDesc = getEditableCategoryDesc();
    return !TextUtils.equals(mCategory.getDescription(), categoryDesc);
  }

  private void clearAndFocus(@NonNull TextView textView)
  {
    textView.getEditableText().clear();
    textView.requestFocus();
    InputUtils.showKeyboard(textView);
  }

  private void showColorPicker()
  {
    final Bundle args = new Bundle();
    if (mColorPickerType == ColorPickerType.BOOKMARK)
      args.putInt(BookmarkColorDialogFragment.ICON_COLOR, BookmarkManager.INSTANCE.getLastEditedColor());
    final BookmarkColorDialogFragment dialogFragment = new BookmarkColorDialogFragment();
    dialogFragment.setArguments(args);
    dialogFragment.show(getChildFragmentManager(), BookmarkColorDialogFragment.class.getName());
  }

  @Override
  public void onBookmarkColorSet(int color)
  {
    switch (mColorPickerType)
    {
    case TRACK ->
    {
      mCategory.setCategoryTracksColor(color);
      Toast.makeText(requireContext(), R.string.toast_tracks_color_changed, Toast.LENGTH_SHORT).show();
    }
    case BOOKMARK ->
    {
      mCategory.setCategoryBookmarksColor(color);
      Toast.makeText(requireContext(), R.string.toast_bookmarks_color_changed, Toast.LENGTH_SHORT).show();
    }
    }
  }

  private void onCategoriesChanged()
  {
    if (getView() != null)
      updateColorButtonsVisibility();
  }
}
