package app.organicmaps.widget.placepage;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.PopupMenu;
import android.widget.TextView;

import androidx.annotation.AttrRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;

import java.util.ArrayList;
import java.util.List;

public final class PlacePageButtons extends Fragment implements Observer<List<PlacePageButtons.ButtonType>>
{
  public static final String PLACEPAGE_MORE_MENU_ID = "PLACEPAGE_MORE_MENU_BOTTOM_SHEET";
  private static final String BUTTON_ROUTE_FINISH_OPTION = "button_route_finish_option";

  private int mMaxButtons;

  private PlacePageButtonClickListener mItemListener;
  private ViewGroup mButtonsContainer;
  private PlacePageViewModel mViewModel;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.pp_buttons_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mButtonsContainer = view.findViewById(R.id.container);
    ViewCompat.setOnApplyWindowInsetsListener(view, (v, windowInsets) -> {
      UiUtils.setViewInsetsPaddingNoTop(v, windowInsets);
      return windowInsets;
    });
    mMaxButtons = getResources().getInteger(R.integer.pp_buttons_max);
    mViewModel.getCurrentButtons().observe(requireActivity(), this);
    createButtons(mViewModel.getCurrentButtons().getValue());
  }

  @Override
  public void onAttach(@NonNull Context context)
  {
    super.onAttach(context);
    try
    {
      mItemListener = (PlacePageButtonClickListener) context;
    }
    catch (ClassCastException e)
    {
      throw new ClassCastException(context + " must implement PlacePageButtonClickListener");
    }
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mViewModel.getCurrentButtons().removeObserver(this);
  }

  private @NonNull
  List<PlacePageButton> collectButtons(List<PlacePageButtons.ButtonType> items)
  {
    List<PlacePageButton> res = new ArrayList<>();
    int count = items.size();
    if (items.size() > mMaxButtons)
      count = mMaxButtons - 1;

    for (int i = 0; i < count; i++)
      res.add(PlacePageButtonFactory.createButton(items.get(i), requireContext()));

    if (items.size() > mMaxButtons)
      res.add(PlacePageButtonFactory.createButton(ButtonType.MORE, requireContext()));
    return res;
  }

  private void showMoreBottomSheet()
  {
    MenuBottomSheetFragment.newInstance(PLACEPAGE_MORE_MENU_ID)
                           .show(requireActivity().getSupportFragmentManager(), PLACEPAGE_MORE_MENU_ID);
  }

  private void createButtons(@Nullable List<ButtonType> buttons)
  {
    if (buttons == null)
      return;
    List<PlacePageButton> shownButtons = collectButtons(buttons);
    mButtonsContainer.removeAllViews();
    for (PlacePageButton button : shownButtons)
      mButtonsContainer.addView(createButton(button));
  }

  private View createButton(@NonNull final PlacePageButton current)
  {
    LayoutInflater inflater = LayoutInflater.from(requireContext());
    final View parent = inflater.inflate(R.layout.place_page_button, mButtonsContainer, false);

    boolean isMultiButton = current.getType() == ButtonType.ROUTE_TO_OR_CONTINUE;

    int actualTitle = current.getTitle();
    int actualIcon = current.getIcon();
    ButtonType actualType = current.getType();

    if (isMultiButton)
    {
      // Read selected option
      List<ButtonType> types = List.of(ButtonType.ROUTE_TO, ButtonType.ROUTE_CONTINUE);
      int pickedButton = MwmApplication.prefs(requireContext()).getInt(BUTTON_ROUTE_FINISH_OPTION, 0);
      if (pickedButton < 0 || pickedButton>=types.size())
        pickedButton = 0;
      actualType = types.get(pickedButton);

      PlacePageButton actualButton = PlacePageButtonFactory.createButton(actualType, requireContext());
      actualTitle = actualButton.getTitle();
      actualIcon = actualButton.getIcon();

      // Show up arrow icon
      ImageView iconUp = parent.findViewById(R.id.icon_up);
      iconUp.setImageDrawable(Graphics.tint(getContext(), R.drawable.ic_triangle_up, R.attr.iconTint));
      UiUtils.show(iconUp);

      parent.setOnLongClickListener((v) -> {
        showButtonsSelectMenu(parent, types, BUTTON_ROUTE_FINISH_OPTION);

        return true;
      });
    }

    setButtonType(parent, actualType, actualTitle, actualIcon);
    return parent;
  }

  private void showButtonsSelectMenu(View buttonView, List<ButtonType> types, String prefsKey)
  {
    final PopupMenu popup = new PopupMenu(requireContext(), buttonView);
    final Menu menu = popup.getMenu();

    for (int i=0; i < types.size(); i++)
    {
      PlacePageButton buttonData = PlacePageButtonFactory.createButton(types.get(i), requireContext());
      MenuItem item = menu.add(Menu.NONE, i, i, buttonData.getTitle());
      item.setIcon(Graphics.tint(getContext(), buttonData.getIcon(), R.attr.iconTint));
    }

    popup.setOnMenuItemClickListener(item -> {
      final int selectedFinishOption = item.getItemId();
      MwmApplication.prefs(requireContext())
          .edit()
          .putInt(prefsKey, selectedFinishOption)
          .apply();

      final ButtonType selectedType = types.get(selectedFinishOption);
      final PlacePageButton selectedButton = PlacePageButtonFactory.createButton(selectedType, requireContext());
      setButtonType(buttonView, selectedType, selectedButton.getTitle(), selectedButton.getIcon());

      return true;
    });

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
      popup.setForceShowIcon(true);

    popup.show();
  }

  private void setButtonType(@NonNull View buttonView, @NonNull ButtonType buttonType,
                             int title, int icon) {
    ImageView iconImg = buttonView.findViewById(R.id.icon);
    TextView titleTv = buttonView.findViewById(R.id.title);

    titleTv.setText(title);
    @AttrRes final int tint = buttonType == ButtonType.BOOKMARK_DELETE
                              ? R.attr.iconTintActive
                              : R.attr.iconTint;
    iconImg.setImageDrawable(Graphics.tint(getContext(), icon, tint));

    buttonView.setOnClickListener((view) -> {
      if (buttonType == ButtonType.MORE)
        showMoreBottomSheet();
      else
        mItemListener.onPlacePageButtonClick(buttonType);
    });
  }

  @Override
  public void onChanged(List<ButtonType> buttonTypes)
  {
    createButtons(buttonTypes);
  }

  public enum ButtonType
  {
    BACK,
    BOOKMARK_SAVE,
    BOOKMARK_DELETE,
    ROUTE_FROM,
    ROUTE_TO,
    ROUTE_ADD,
    ROUTE_REMOVE,
    ROUTE_AVOID_TOLL,
    ROUTE_AVOID_FERRY,
    ROUTE_AVOID_UNPAVED,
    ROUTE_CONTINUE,
    ROUTE_TO_OR_CONTINUE,
    SHARE,
    MORE
  }

  public interface PlacePageButtonClickListener
  {
    void onPlacePageButtonClick(ButtonType item);
  }
}
