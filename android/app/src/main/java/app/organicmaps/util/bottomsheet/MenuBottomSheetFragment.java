package app.organicmaps.util.bottomsheet;

import android.app.Activity;
import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import java.util.ArrayList;
import java.util.Objects;

import app.organicmaps.R;
import app.organicmaps.sdk.util.UiUtils;

public class MenuBottomSheetFragment extends BottomSheetDialogFragment
{

  @Nullable
  private ArrayList<MenuBottomSheetItem> mMenuBottomSheetItems;
  @Nullable
  private Fragment mHeaderFragment;

  public static MenuBottomSheetFragment newInstance(String id)
  {
    Bundle args = new Bundle();
    args.putString("id", id);
    MenuBottomSheetFragment f = new MenuBottomSheetFragment();
    f.setArguments(args);
    return f;
  }

  public static MenuBottomSheetFragment newInstance(String id, String title)
  {
    Bundle args = new Bundle();
    args.putString("id", id);
    args.putString("title", title);
    MenuBottomSheetFragment f = new MenuBottomSheetFragment();
    f.setArguments(args);
    return f;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    return new BottomSheetDialog(requireContext(), getTheme()) {
      @Override
      public void onAttachedToWindow() {
        super.onAttachedToWindow();
        Window window = Objects.requireNonNull(getWindow());
        WindowInsetsControllerCompat insetsController =
                WindowCompat.getInsetsController(window, window.getDecorView());
        insetsController.setAppearanceLightNavigationBars(false);
      }
    };
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
    attachToNearestContext();
    TextView titleView = view.findViewById(R.id.bottomSheetTitle);
    RecyclerView recyclerView = view.findViewById(R.id.bottomSheetMenuContainer);
    if (getArguments() != null)
    {
      String title = getArguments().getString("title");
      if (title != null && !title.isEmpty())
      {
        titleView.setVisibility(View.VISIBLE);
        titleView.setText(title);
      }
      else
        titleView.setVisibility(View.GONE);
    }
    else
      titleView.setVisibility(View.GONE);

    if (mMenuBottomSheetItems != null)
    {
      MenuAdapter menuAdapter = new MenuAdapter(mMenuBottomSheetItems, this::dismiss);
      recyclerView.setAdapter(menuAdapter);
      recyclerView.setLayoutManager(new LinearLayoutManager(requireActivity()));
    }
    if (mHeaderFragment != null)
      getChildFragmentManager().beginTransaction().add(R.id.bottom_sheet_menu_header, mHeaderFragment).commit();

    // If there is nothing to show, hide the sheet
    if (!UiUtils.isVisible(titleView) && mMenuBottomSheetItems == null && mHeaderFragment == null)
      dismiss();
  }

  private void attachToNearestContext()
  {
    // Try to attach to the parent fragment if any
    // In other cases, attach to the activity
    MenuBottomSheetInterface bottomSheetInterface = null;
    MenuBottomSheetInterfaceWithHeader bottomSheetInterfaceWithHeader = null;

    Fragment parentFragment = getParentFragment();
    if (parentFragment instanceof MenuBottomSheetInterfaceWithHeader)
      bottomSheetInterfaceWithHeader = (MenuBottomSheetInterfaceWithHeader) parentFragment;
    else if (parentFragment instanceof MenuBottomSheetInterface)
      bottomSheetInterface = (MenuBottomSheetInterface) parentFragment;
    else
    {
      Activity parentActivity = requireActivity();
      if (parentActivity instanceof MenuBottomSheetInterfaceWithHeader)
        bottomSheetInterfaceWithHeader  = (MenuBottomSheetInterfaceWithHeader) parentActivity;
      else if (parentActivity instanceof MenuBottomSheetInterface)
        bottomSheetInterface  = (MenuBottomSheetInterface) parentActivity;
    }

    if (bottomSheetInterface != null)
    {
      if (getArguments() != null)
      {
        String id = getArguments().getString("id");
        if (id != null && !id.isEmpty())
          mMenuBottomSheetItems = bottomSheetInterface.getMenuBottomSheetItems(id);
      }
    }
    else if (bottomSheetInterfaceWithHeader != null)
    {
      if (getArguments() != null)
      {
        String id = getArguments().getString("id");
        if (id != null && !id.isEmpty())
        {
          mMenuBottomSheetItems = bottomSheetInterfaceWithHeader.getMenuBottomSheetItems(id);
          mHeaderFragment = bottomSheetInterfaceWithHeader.getMenuBottomSheetFragment(id);
        }
      }
    }
  }

  public interface MenuBottomSheetInterfaceWithHeader
  {
    @Nullable
    Fragment getMenuBottomSheetFragment(String id);
    @Nullable
    ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems(String id);
  }

  public interface MenuBottomSheetInterface
  {
    @Nullable
    ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems(String id);
  }

}
