package com.mapswithme.maps.ugc.routes;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.base.FinishActivityToolbarController;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.widget.ToolbarController;

import java.util.Objects;

public class UgcRouteEditSettingsFragment extends BaseMwmToolbarFragment
{
  public static final String EXTRA_BOOKMARK_CATEGORY = "bookmark_category";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkCategory mCategory;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mAccessRulesView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private EditText mEditDescView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private EditText mEditCategoryNameView;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = getArguments();
    if (args == null)
      throw new IllegalArgumentException("Args must be not null");
    mCategory = Objects.requireNonNull(args.getParcelable(EXTRA_BOOKMARK_CATEGORY));
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_route_edit, container, false);
    initViews(root);
    return root;
  }

  private void initViews(@NonNull View root)
  {
    mEditCategoryNameView = root.findViewById(R.id.edit_category_name_view);
    mEditCategoryNameView.setText(mCategory.getName());
    mEditCategoryNameView.requestFocus();
    mAccessRulesView = root.findViewById(R.id.sharing_options_desc);
    mAccessRulesView.setText(mCategory.getAccessRules().getNameResId());
    mEditDescView = root.findViewById(R.id.edit_description);
    mEditDescView.setText(mCategory.getDescription());
    View clearNameBtn = root.findViewById(R.id.edit_text_clear_btn);
    clearNameBtn.setOnClickListener(v -> mEditCategoryNameView.getEditableText().clear());
    View sharingOptionsBtn = root.findViewById(R.id.open_sharing_options_screen_btn_container);
    sharingOptionsBtn.setOnClickListener(v -> onSharingOptionsClicked());
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    mCategory = BookmarkManager.INSTANCE.getAllCategoriesSnapshot().refresh(mCategory);
    mAccessRulesView.setText(mCategory.getAccessRules().getNameResId());
  }

  private void onSharingOptionsClicked()
  {
    openSharingOptionsScreen();
  }

  private void openSharingOptionsScreen()
  {
    Intent intent = new Intent(getContext(), UgcRouteSharingOptionsActivity.class)
        .putExtra(UgcSharingOptionsFragment.EXTRA_BOOKMARK_CATEGORY, mCategory);
    startActivityForResult(intent, UgcRouteSharingOptionsActivity.REQUEST_CODE);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.edit);
  }

  @NonNull
  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new FinishActivityToolbarController(root, getActivity());
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.done)
    {
      onEditDoneClicked();
      return true;
    }
    return super.onOptionsItemSelected(item);
  }

  private void onEditDoneClicked()
  {
    String categoryName = mEditCategoryNameView.getEditableText().toString().trim();

    if (!TextUtils.equals(categoryName, mCategory.getName()))
      BookmarkManager.INSTANCE.setCategoryName(mCategory.getId(), categoryName);

    String categoryDesc = mEditDescView.getEditableText().toString().trim();
    if (!TextUtils.equals(mCategory.getDescription(), categoryDesc))
      BookmarkManager.INSTANCE.setCategoryDescription(mCategory.getId(), categoryDesc);
  }
}
