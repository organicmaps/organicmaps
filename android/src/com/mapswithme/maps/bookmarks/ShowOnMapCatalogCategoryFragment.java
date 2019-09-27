package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.util.statistics.Statistics;

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
    mCategory = getCategoryOrThrow(args);
  }

  @NonNull
  private BookmarkCategory getCategoryOrThrow(@Nullable Bundle args)
  {
    BookmarkCategory category;
    if (args == null || ((category = args.getParcelable(ARGS_CATEGORY)) == null))
      throw new IllegalArgumentException("Category not found");

    return category;
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable
      Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_show_on_map_catalog_category, container, false);
    View acceptBtn = root.findViewById(R.id.show_on_map_accept_btn);
    acceptBtn.setOnClickListener(v -> onAccepted());
    root.setOnClickListener(view -> onDeclined());
    return root;
  }

  private void onDeclined()
  {
    Statistics.INSTANCE.trackDownloadBookmarkDialog(Statistics.ParamValue.NOT_NOW);
    dismissAllowingStateLoss();
  }

  private void onAccepted()
  {
    Statistics.INSTANCE.trackDownloadBookmarkDialog(Statistics.ParamValue.VIEW_ON_MAP);
    Intent result = new Intent().putExtra(BookmarksCatalogActivity.EXTRA_DOWNLOADED_CATEGORY,
                                          mCategory);
    getActivity().setResult(Activity.RESULT_OK, result);
    dismissAllowingStateLoss();
    getActivity().finish();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    Statistics.INSTANCE.trackDownloadBookmarkDialog(Statistics.ParamValue.CLICK_OUTSIDE);
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
