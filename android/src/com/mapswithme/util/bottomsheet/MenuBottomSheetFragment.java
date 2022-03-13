package com.mapswithme.util.bottomsheet;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;

import java.util.ArrayList;

public class MenuBottomSheetFragment extends BottomSheetDialogFragment
{
  @Nullable
  private final String title;
  private final ArrayList<MenuBottomSheetItem> menuBottomSheetItems;

  public MenuBottomSheetFragment(@NonNull String title, ArrayList<MenuBottomSheetItem> menuBottomSheetItems)
  {
    this.title = title;
    this.menuBottomSheetItems = menuBottomSheetItems;
  }

  public MenuBottomSheetFragment(ArrayList<MenuBottomSheetItem> menuBottomSheetItems)
  {
    this.title = null;
    this.menuBottomSheetItems = menuBottomSheetItems;
  }

  @Override
  public int getTheme()
  {
    return ThemeUtils.isNightTheme(requireContext())
        ? R.style.MwmTheme_Night_BottomSheetDialog
        : R.style.MwmTheme_BottomSheetDialog;
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.bottom_sheet, container);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BottomSheetBehavior<View> behavior = BottomSheetBehavior.from((View) requireView().getParent());
    // By default sheets in landscape start at their peek height.
    // We fix this by forcing the expanded state and disabling the collapsed one
    behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
    behavior.setSkipCollapsed(true);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    TextView titleView = view.findViewById(R.id.bottomSheetTitle);
    RecyclerView recyclerView = view.findViewById(R.id.bottomSheetMenuContainer);
    if (title != null)
    {
      titleView.setVisibility(View.VISIBLE);
      titleView.setText(title);
    } else
      titleView.setVisibility(View.GONE);

    MenuAdapter menuAdapter = new MenuAdapter(menuBottomSheetItems, this::dismiss);
    recyclerView.setAdapter(menuAdapter);
    recyclerView.setLayoutManager(new LinearLayoutManager(requireActivity()));
  }
}
