package app.organicmaps.editor;

import static app.organicmaps.sdk.util.Utils.getLocalizedFeatureType;

import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.editor.data.FeatureCategory;
import app.organicmaps.sdk.util.Language;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.SearchToolbarController;
import app.organicmaps.widget.ToolbarController;
import app.organicmaps.widget.recycler.DividerItemDecorationWithPadding;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.util.Arrays;
import java.util.Comparator;

public class FeatureCategoryFragment
    extends BaseMwmRecyclerFragment<FeatureCategoryAdapter> implements FeatureCategoryAdapter.FooterListener
{
  private static final String NOTE_CONFIRMATION_SHOWN = "NoteConfirmationAlertWasShown";
  private static final String KEY_PENDING_NOTE_TEXT = "pending_note_text";

  private FeatureCategory mSelectedCategory;
  protected ToolbarController mToolbarController;
  private String mPendingNoteText = "";

  public interface FeatureCategoryListener
  {
    void onFeatureCategorySelected(FeatureCategory category);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_categories, container, false);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getRecyclerView().addItemDecoration(new DividerItemDecorationWithPadding(requireContext()));

    final Bundle args = getArguments();
    if (args != null)
    {
      mSelectedCategory =
          Utils.getParcelable(args, FeatureCategoryActivity.EXTRA_FEATURE_CATEGORY, FeatureCategory.class);
    }
    mToolbarController = new SearchToolbarController(view, requireActivity()) {
      @Override
      protected void onTextChanged(String query)
      {
        setFilter(query);
      }
    };

    if (savedInstanceState != null)
      mPendingNoteText = savedInstanceState.getString(KEY_PENDING_NOTE_TEXT, "");
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putString(KEY_PENDING_NOTE_TEXT, mPendingNoteText);
  }

  private void setFilter(String query)
  {
    String locale = Language.getDefaultLocale();
    String[] creatableTypes = query.isEmpty() ? Editor.nativeGetAllCreatableFeatureTypes(locale)
                                              : Editor.nativeSearchCreatableFeatureTypes(query, locale);

    FeatureCategory[] categories = makeFeatureCategoriesFromTypes(creatableTypes, true);

    // Hide recent categories during search
    FeatureCategory[] recentCategories = query.isEmpty()
                                           ? makeFeatureCategoriesFromTypes(Editor.nativeGetRecentCategories(), false)
                                           : new FeatureCategory[0];

    getAdapter().setCategories(categories, recentCategories);
    getRecyclerView().scrollToPosition(0);
  }

  @NonNull
  @Override
  protected FeatureCategoryAdapter createAdapter()
  {
    String locale = Language.getDefaultLocale();
    String[] creatableTypes = Editor.nativeGetAllCreatableFeatureTypes(locale);
    String[] recentTypes = Editor.nativeGetRecentCategories();

    FeatureCategory[] categories = makeFeatureCategoriesFromTypes(creatableTypes, true);
    FeatureCategory[] recentCategories = makeFeatureCategoriesFromTypes(recentTypes, false);

    return new FeatureCategoryAdapter(this, categories, recentCategories, mSelectedCategory);
  }

  @NonNull
  private FeatureCategory[] makeFeatureCategoriesFromTypes(@NonNull String[] types, boolean sort)
  {
    FeatureCategory[] categories = new FeatureCategory[types.length];

    for (int i = 0; i < types.length; ++i)
    {
      String localizedType = getLocalizedFeatureType(requireContext(), types[i]);
      categories[i] = new FeatureCategory(types[i], localizedType);
    }

    if (sort)
      Arrays.sort(categories, Comparator.comparing(FeatureCategory::getLocalizedTypeName));

    return categories;
  }

  public void selectCategory(FeatureCategory category)
  {
    Editor.nativeAddToRecentCategories(category.getType());
    if (requireActivity() instanceof FeatureCategoryListener)
      ((FeatureCategoryListener) requireActivity()).onFeatureCategorySelected(category);
    else if (getParentFragment() instanceof FeatureCategoryListener)
      ((FeatureCategoryListener) getParentFragment()).onFeatureCategorySelected(category);
  }

  public String getPendingNoteText()
  {
    return mPendingNoteText;
  }

  @Override
  public void onNoteTextChanged(String newText)
  {
    mPendingNoteText = newText;
  }

  @Override
  public void onSendNoteClicked()
  {
    if (!OsmOAuth.isAuthorized())
    {
      final Intent intent = new Intent(requireActivity(), OsmLoginActivity.class);
      startActivity(intent);
      return;
    }

    final double[] center = Framework.nativeGetScreenRectCenter();
    final double lat = center[0];
    final double lon = center[1];

    if (!MwmApplication.prefs(requireContext().getApplicationContext()).contains(NOTE_CONFIRMATION_SHOWN))
    {
      showNoteConfirmationDialog(lat, lon, mPendingNoteText);
    }
    else
    {
      Editor.nativeCreateStandaloneNote(lat, lon, mPendingNoteText);
      mPendingNoteText = "";
      Toast.makeText(requireContext(), R.string.osm_note_toast, Toast.LENGTH_SHORT).show();
      requireActivity().finish();
    }
  }

  // Duplicate of showNoobDialog()
  private void showNoteConfirmationDialog(double lat, double lon, String noteText)
  {
    new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.editor_share_to_all_dialog_title)
        .setMessage(getString(R.string.editor_share_to_all_dialog_message_1) + " "
                    + getString(R.string.editor_share_to_all_dialog_message_2))
        .setPositiveButton(android.R.string.ok,
                           (dlg, which) -> {
                             MwmApplication.prefs(requireContext().getApplicationContext())
                                 .edit()
                                 .putBoolean(NOTE_CONFIRMATION_SHOWN, true)
                                 .apply();
                             Editor.nativeCreateStandaloneNote(lat, lon, noteText);
                             mPendingNoteText = "";
                             Toast.makeText(requireContext(), R.string.osm_note_toast, Toast.LENGTH_SHORT).show();
                             requireActivity().finish();
                           })
        .setNegativeButton(R.string.cancel, null)
        .show();
  }
}
