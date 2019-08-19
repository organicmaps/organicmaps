package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.UiUtils;

public class ChooseBookmarksSortingTypeFragment extends BaseMwmDialogFragment
    implements RadioGroup.OnCheckedChangeListener
{
  private static final String EXTRA_SORTING_TYPES = "sorting_types";
  private static final String EXTRA_CURRENT_SORT_TYPE = "current_sort_type";

  @Nullable
  ChooseSortingTypeListener mListener;

  public interface ChooseSortingTypeListener
  {
    void onResetSorting();
    void onSort(@BookmarkManager.SortingType int sortingType);
  }

  public void setChooseSortingTypeListener(@Nullable ChooseSortingTypeListener listener)
  {
    mListener = listener;
  }

  public static void chooseSortingType(@NonNull @BookmarkManager.SortingType int[] availableTypes,
                                       int currentType, @NonNull ChooseSortingTypeListener listener,
                                       @NonNull Context context,
                                       @NonNull FragmentManager manager)
  {
    Bundle args = new Bundle();
    args.putIntArray(EXTRA_SORTING_TYPES, availableTypes);
    args.putInt(EXTRA_CURRENT_SORT_TYPE, currentType);
    String name = ChooseBookmarksSortingTypeFragment.class.getName();
    final ChooseBookmarksSortingTypeFragment fragment =
        (ChooseBookmarksSortingTypeFragment) Fragment.instantiate(context, name, args);
    fragment.setArguments(args);
    fragment.setChooseSortingTypeListener(listener);
    fragment.show(manager, name);
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.dialog_sorting_types, container, false);
  }

  @Override
  protected int getStyle()
  {
    return STYLE_NO_TITLE;
  }

  private int getViewId(int sortingType)
  {
    if (sortingType >= 0)
    {
      switch (sortingType)
      {
        case BookmarkManager.SORT_BY_TYPE:
          return R.id.sort_by_type;
        case BookmarkManager.SORT_BY_DISTANCE:
          return R.id.sort_by_distance;
        case BookmarkManager.SORT_BY_TIME:
          return R.id.sort_by_time;
      }
    }
    return R.id.sort_by_default;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    final Bundle args = getArguments();

    UiUtils.hide(view.findViewById(R.id.sort_by_type));
    UiUtils.hide(view.findViewById(R.id.sort_by_distance));
    UiUtils.hide(view.findViewById(R.id.sort_by_time));

    @BookmarkManager.SortingType
    int[] availableSortingTypes = args.getIntArray(EXTRA_SORTING_TYPES);
    for (int i = 0; i < availableSortingTypes.length; ++i)
      UiUtils.show(view.findViewById(getViewId(availableSortingTypes[i])));

    int currentType = args.getInt(EXTRA_CURRENT_SORT_TYPE);

    RadioGroup radioGroup = view.findViewById(R.id.sorting_types);
    radioGroup.clearCheck();
    radioGroup.check(getViewId(currentType));
    radioGroup.setOnCheckedChangeListener(this);
  }

  private void resetSorting()
  {
    if (mListener != null)
      mListener.onResetSorting();
    dismiss();
  }

  private void setSortingType(@BookmarkManager.SortingType int sortingType)
  {
    if (mListener != null)
      mListener.onSort(sortingType);
    dismiss();
  }

  @Override
  public void onCheckedChanged(RadioGroup group, int id)
  {
    switch (id)
    {
      case R.id.sort_by_default:
        resetSorting();
        break;
      case R.id.sort_by_type:
        setSortingType(BookmarkManager.SORT_BY_TYPE);
        break;
      case R.id.sort_by_distance:
        setSortingType(BookmarkManager.SORT_BY_DISTANCE);
        break;
      case R.id.sort_by_time:
        setSortingType(BookmarkManager.SORT_BY_TIME);
        break;
    }
  }
}
