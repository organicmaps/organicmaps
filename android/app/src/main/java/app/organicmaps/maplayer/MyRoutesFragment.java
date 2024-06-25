package app.organicmaps.maplayer;

import android.app.AlertDialog;
import android.os.Bundle;
import android.os.SystemClock;
import android.text.InputFilter;
import android.text.InputType;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.button.MaterialButton;

import java.util.ArrayList;
import java.util.List;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;

public class MyRoutesFragment extends Fragment
{
  private static final String MYROUTES_MENU_ID = "MYROUTES_MENU_BOTTOM_SHEET";
  @Nullable
  private RoutesAdapter mAdapter;
  private MapButtonsViewModel mMapButtonsViewModel;
  private String mEditText = "";
  private String mDialogCaller = "";
  private RouteBottomSheetItem mCurrentItem = null;
  private static final String SAVE_ID = "SAVE_ID";
  private static final String RENAME_ID = "RENAME_ID";
  private static final String DELETE_ID = "DELETE_ID";

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View mRoot = inflater.inflate(R.layout.fragment_myroutes, container, false);

    mMapButtonsViewModel = new ViewModelProvider(requireActivity()).get(MapButtonsViewModel.class);
    MaterialButton mCloseButton = mRoot.findViewById(R.id.close_button);
    mCloseButton.setOnClickListener(view -> closeMyRoutesBottomSheet());

    Button mSaveButton = mRoot.findViewById(R.id.save_button);
    if (Framework.nativeGetRoutePoints().length >= 2)
      mSaveButton.setOnClickListener(view -> onSaveButtonClick());
    else
      mSaveButton.setEnabled(false);

    initRecycler(mRoot);
    return mRoot;
  }

  private void initRecycler(@NonNull View root)
  {
    RecyclerView recycler = root.findViewById(R.id.recycler);
    RecyclerView.LayoutManager layoutManager = new LinearLayoutManager(requireContext(),
        LinearLayoutManager.VERTICAL,
        false);
    recycler.setLayoutManager(layoutManager);
    mAdapter = new RoutesAdapter(getRouteItems());
    recycler.setAdapter(mAdapter);
    recycler.setNestedScrollingEnabled(false);
  }

  private List<RouteBottomSheetItem> getRouteItems()
  {
    String[] savedRouteNames = Framework.nativeGetUserRouteNames();
    List<RouteBottomSheetItem> items = new ArrayList<>();
    for (String routeName : savedRouteNames)
      items.add(createItem(routeName));
    return items;
  }

  private RouteBottomSheetItem createItem(String routeName)
  {
    return RouteBottomSheetItem.create(routeName, this::onItemTitleClick, this::onItemRenameClick, this::onItemDeleteClick);
  }

  private void onSaveButtonClick()
  {
    mEditText = "";
    mDialogCaller = SAVE_ID;
    showTextInputDialog("");
  }

  private void checkSave()
  {
    String newRouteName = mEditText;

    if (newRouteName.isEmpty())
      return;
    if (Framework.nativeHasSavedUserRoute(newRouteName))
    {
      showConfirmationDialog(getString(R.string.user_route_overwrite_title, newRouteName),
              getString(R.string.user_route_overwrite_body, newRouteName),
              R.string.overwrite);
      return;
    }
    save(false);
  }

  private void save(boolean isOverwrite)
  {
    String newRouteName = mEditText;

    if (isOverwrite)
      Framework.nativeDeleteUserRoute(newRouteName);

    Framework.nativeSaveUserRoutePoints(newRouteName);

    if (!isOverwrite)
      mAdapter.addRoute(createItem(newRouteName));
  }

  private void onItemTitleClick(@NonNull View v, @NonNull RouteBottomSheetItem item)
  {
    Framework.nativeLoadUserRoutePoints(item.getRouteName());
  }

  private void onItemRenameClick(@NonNull View v, @NonNull RouteBottomSheetItem item)
  {
    mEditText = "";
    mDialogCaller = RENAME_ID;
    mCurrentItem = item;
    showTextInputDialog(item.getRouteName());
  }

  private void checkRename()
  {
    String newRouteName = mEditText;

    if (newRouteName.isEmpty())
      return;

    if (newRouteName.equals(mCurrentItem.getRouteName()))
      return;
    if (Framework.nativeHasSavedUserRoute(newRouteName))
    {
      showConfirmationDialog(getString(R.string.user_route_overwrite_title, newRouteName),
              getString(R.string.user_route_overwrite_body, newRouteName),
              R.string.overwrite);
      return;
    }
    rename(false);
  }

  private void rename(boolean isOverwrite)
  {
    String newRouteName = mEditText;

    if (isOverwrite)
    {
      Framework.nativeDeleteUserRoute(newRouteName);
      // Sometimes delete takes too long and renaming cannot happen; thus, sleep
      SystemClock.sleep(250); // TODO(Fábio Gomes) find a better solution
    }

    Framework.nativeRenameUserRoute(mCurrentItem.getRouteName(), newRouteName);

    mAdapter.removeRoute(mCurrentItem);
    if (!isOverwrite)
      mAdapter.addRoute(createItem(newRouteName));
  }

  private void onItemDeleteClick(@NonNull View v, @NonNull RouteBottomSheetItem item)
  {
    mDialogCaller = DELETE_ID;
    mCurrentItem = item;
    showConfirmationDialog(getString(R.string.user_route_delete_title, item.getRouteName()),
            getString(R.string.user_route_delete_body),
            R.string.delete);
  }

  private void delete()
  {
    Framework.nativeDeleteUserRoute(mCurrentItem.getRouteName());
    mAdapter.removeRoute(mCurrentItem);
  }

  private void closeMyRoutesBottomSheet()
  {
    MenuBottomSheetFragment bottomSheet =
        (MenuBottomSheetFragment) requireActivity().getSupportFragmentManager().findFragmentByTag(MYROUTES_MENU_ID);
    if (bottomSheet != null)
      bottomSheet.dismiss();
  }

  private void showConfirmationDialog(String title, String message, @StringRes int buttonText)
  {
    AlertDialog dialog = new AlertDialog.Builder(this.getContext(), R.style.MwmTheme_AlertDialog)
            .setTitle(title)
            .setMessage(message)
            .setPositiveButton(buttonText, (dialogInterface, i) -> {
                switch (mDialogCaller) {
                    case SAVE_ID -> save(true);
                    case RENAME_ID -> rename(true);
                    case DELETE_ID -> delete();
                }
            })
            .setNegativeButton(R.string.cancel, (dialogInterface, i) -> {
              switch (mDialogCaller) {
                case SAVE_ID, RENAME_ID -> retryInput();
              }
            })
            .create();

    dialog.show();
  }

  private void showTextInputDialog(String defaultText)
  {
    EditText input =  new EditText(this.getContext());
    input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
    input.setFilters(getInputFilters());
    input.setText(defaultText);

    AlertDialog dialog = new AlertDialog.Builder(this.getContext(), R.style.MwmTheme_AlertDialog)
            .setTitle(R.string.user_route_input_title)
            .setView(input)
            .setPositiveButton(R.string.ok, (dialogInterface, i) -> {
              mEditText = input.getText().toString();
              switch (mDialogCaller) {
                case SAVE_ID -> checkSave();
                case RENAME_ID -> checkRename();
              }
            })
            .setNegativeButton(R.string.cancel, null)
            .create();

    dialog.show();
  }

  private void retryInput()
  {
    showTextInputDialog(mEditText);
  }

  private InputFilter[] getInputFilters()
  {
    InputFilter filter = (source, start, end, dest, dstart, dend) -> {
      for (int i = start; i < end; i++)
      {
        char current = source.charAt(i);
        if (!Character.isSpaceChar(current) && !Character.isLetterOrDigit(current))
        {
          return "";
        }
      }
      return null;
    };
    return new InputFilter[] {filter, new InputFilter.LengthFilter(32)};
  }
}
