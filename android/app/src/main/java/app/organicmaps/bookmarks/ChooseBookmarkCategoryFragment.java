package app.organicmaps.bookmarks;

import android.app.Dialog;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmDialogFragment;
import app.organicmaps.dialog.EditTextDialogFragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.util.ArrayList;
import java.util.List;

public class ChooseBookmarkCategoryFragment extends BaseMwmDialogFragment
{
  public static final String CATEGORY_ID = "ExtraCategoryId";

  public interface Listener
  {
    void onCategoryChanged(@NonNull BookmarkCategory newCategory);
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    // Neither field survives recreation, and restoring only one leaves the empty input unguarded.
    final EditTextDialogFragment nameDialog =
        (EditTextDialogFragment) getChildFragmentManager().findFragmentByTag(EditTextDialogFragment.TAG);
    if (nameDialog != null)
    {
      nameDialog.setTextSaveListener(this::createCategory);
      nameDialog.setValidator(new CategoryValidator());
    }
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    final long checkedId = requireArguments().getLong(CATEGORY_ID);
    // getCategories() is a live view that is re-sorted on every change; snapshot it so indices stay valid.
    final List<BookmarkCategory> categories = new ArrayList<>(BookmarkManager.INSTANCE.getCategories());

    final String[] names = new String[categories.size()];
    int checkedItem = -1;
    for (int i = 0; i < categories.size(); i++)
    {
      names[i] = categories.get(i).getName();
      if (categories.get(i).getId() == checkedId)
        checkedItem = i;
    }

    final AlertDialog dialog =
        new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setTitle(R.string.select_list)
            .setSingleChoiceItems(names, checkedItem, (d, which) -> onCategorySet(categories.get(which)))
            .setPositiveButton(R.string.add_new_set, null)
            .setNegativeButton(R.string.cancel, null)
            .create();

    // Must not dismiss: the name dialog lives in this fragment's child FragmentManager.
    dialog.setOnShowListener(
        d -> dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> onCategoryCreate()));

    return dialog;
  }

  @Nullable
  private Listener getListener()
  {
    final Fragment parent = getParentFragment();
    if (parent instanceof Listener)
      return (Listener) parent;
    return getActivity() instanceof Listener ? (Listener) getActivity() : null;
  }

  private void onCategorySet(@NonNull BookmarkCategory category)
  {
    final Listener listener = getListener();
    if (listener != null)
      listener.onCategoryChanged(category);
    dismiss();
  }

  private void createCategory(@NonNull String name)
  {
    final long categoryId = BookmarkManager.INSTANCE.createCategory(name);
    onCategorySet(BookmarkManager.INSTANCE.getCategoryById(categoryId));
  }

  private void onCategoryCreate()
  {
    // The picker stays open behind the name dialog, so guard against a second one. show() and
    // dismiss() commit asynchronously, so flush them before asking.
    final FragmentManager fm = getChildFragmentManager();
    fm.executePendingTransactions();
    if (fm.findFragmentByTag(EditTextDialogFragment.TAG) != null)
      return;

    EditTextDialogFragment dialogFragment =
        EditTextDialogFragment.show(getString(R.string.bookmark_set_name), null, "", getString(R.string.ok),
                                    getString(R.string.cancel), CategoryValidator.MAX_NAME_LENGTH, this);
    dialogFragment.setValidator(new CategoryValidator());
    dialogFragment.setTextSaveListener(this::createCategory);
  }
}
