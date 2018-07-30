package com.mapswithme.maps.bookmarks;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AppCompatDialog;
import android.view.LayoutInflater;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public class ShowOnMapCatalogCategoryFragment extends DialogFragment
{
  public static final String TAG = ShowOnMapCatalogCategoryFragment.class.getCanonicalName();

  static final String ARGS_CATEGORY = "downloaded_category";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkCategory mCategory;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = getArguments();
    mCategory = getArgsOrThrow(args);
  }

  private BookmarkCategory getArgsOrThrow(@Nullable Bundle args)
  {
    BookmarkCategory category;
    if (args == null || ((category = args.getParcelable(ARGS_CATEGORY)) == null))
      throw new IllegalArgumentException("Category not found");

    return category;
  }

  @NonNull
  @Override
  @SuppressLint("InflateParams")
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    LayoutInflater inflater = getActivity().getLayoutInflater();
    View root = inflater.inflate(R.layout.fragment_show_on_map_catalog_category, null, false);
    View acceptBtn = root.findViewById(R.id.show_on_map_accept_btn);
    acceptBtn.setOnClickListener(v -> onAccepted());
    root.setOnClickListener(view -> onDeclined());
    Dialog dialog = new AppCompatDialog(getActivity());
    dialog.setContentView(root);
    return dialog;
  }

  private void onDeclined()
  {
    dismissAllowingStateLoss();
  }

  private void onAccepted()
  {
    Intent result = new Intent().putExtra(BookmarksCatalogActivity.EXTRA_CATEGORY, mCategory);
    getActivity().setResult(Activity.RESULT_OK, result);
    dismissAllowingStateLoss();
    getActivity().finish();
  }

  void setCategory(@NonNull BookmarkCategory category)
  {
    mCategory = category;
  }

  @NonNull
  public static ShowOnMapCatalogCategoryFragment newInstance(@NonNull BookmarkCategory category)
  {
    Bundle args = new Bundle();
    args.putParcelable(ARGS_CATEGORY, category);
    ShowOnMapCatalogCategoryFragment fragment = new ShowOnMapCatalogCategoryFragment();
    fragment.setArguments(args);
    return fragment;
  }
}
