package app.organicmaps.bookmarks;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioGroup;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmDialogFragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.util.UiUtils;

public class ChooseBookmarksSortingTypeFragment
    extends BaseMwmDialogFragment implements RadioGroup.OnCheckedChangeListener
{
  private static final String EXTRA_SORTING_TYPES = "sorting_types";
  private static final String EXTRA_CURRENT_SORT_TYPE = "current_sort_type";

  @Nullable
  private ChooseSortingTypeListener mListener;

  public interface ChooseSortingTypeListener
  {
    void onResetSorting();
    void onSort(@BookmarkCategory.SortingType int sortingType);
  }

  public static void chooseSortingType(@NonNull @BookmarkCategory.SortingType int[] availableTypes, int currentType,
                                       @NonNull Context context, @NonNull FragmentManager manager)
  {
    Bundle args = new Bundle();
    args.putIntArray(EXTRA_SORTING_TYPES, availableTypes);
    args.putInt(EXTRA_CURRENT_SORT_TYPE, currentType);
    String name = ChooseBookmarksSortingTypeFragment.class.getName();
    final ChooseBookmarksSortingTypeFragment fragment =
        (ChooseBookmarksSortingTypeFragment) manager.getFragmentFactory().instantiate(context.getClassLoader(), name);
    fragment.setArguments(args);
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

  @IdRes
  private int getViewId(int sortingType)
  {
    if (sortingType >= 0)
    {
      switch (sortingType)
      {
      case BookmarkCategory.SortingType.BY_TYPE: return R.id.sort_by_type;
      case BookmarkCategory.SortingType.BY_DISTANCE: return R.id.sort_by_distance;
      case BookmarkCategory.SortingType.BY_TIME: return R.id.sort_by_time;
      case BookmarkCategory.SortingType.BY_NAME: return R.id.sort_by_name;
      }
    }
    return R.id.sort_by_default;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    final Bundle args = getArguments();
    if (args == null)
      throw new AssertionError("Arguments of choose sorting type view can't be null.");

    UiUtils.hide(view, R.id.sort_by_type, R.id.sort_by_distance, R.id.sort_by_time, R.id.sort_by_name);

    @BookmarkCategory.SortingType
    int[] availableSortingTypes = args.getIntArray(EXTRA_SORTING_TYPES);
    if (availableSortingTypes == null)
      throw new AssertionError("Available sorting types can't be null.");

    for (@BookmarkCategory.SortingType int type : availableSortingTypes)
      UiUtils.show(view.findViewById(getViewId(type)));

    int currentType = args.getInt(EXTRA_CURRENT_SORT_TYPE);

    RadioGroup radioGroup = view.findViewById(R.id.sorting_types);
    radioGroup.clearCheck();
    radioGroup.check(getViewId(currentType));
    radioGroup.setOnCheckedChangeListener(this);
  }

  @Override
  public void onAttach(@NonNull Context context)
  {
    super.onAttach(context);
    onAttachInternal();
  }

  private void onAttachInternal()
  {
    mListener = (ChooseSortingTypeListener) (getParentFragment() == null ? getTargetFragment() : getParentFragment());
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    mListener = null;
  }

  private void resetSorting()
  {
    if (mListener != null)
      mListener.onResetSorting();
    dismiss();
  }

  private void setSortingType(@BookmarkCategory.SortingType int sortingType)
  {
    if (mListener != null)
      mListener.onSort(sortingType);
    dismiss();
  }

  @Override
  public void onCheckedChanged(@NonNull RadioGroup group, @IdRes int id)
  {
    if (id == R.id.sort_by_default)
      resetSorting();
    else if (id == R.id.sort_by_type)
      setSortingType(BookmarkCategory.SortingType.BY_TYPE);
    else if (id == R.id.sort_by_distance)
      setSortingType(BookmarkCategory.SortingType.BY_DISTANCE);
    else if (id == R.id.sort_by_time)
      setSortingType(BookmarkCategory.SortingType.BY_TIME);
    else if (id == R.id.sort_by_name)
      setSortingType(BookmarkCategory.SortingType.BY_NAME);
  }
}
