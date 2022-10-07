package com.mapswithme.maps.widget.placepage;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.bottomsheet.MenuBottomSheetFragment;
import com.mapswithme.util.bottomsheet.MenuBottomSheetItem;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public final class PlacePageButtons
{
  public static final String PLACEPAGE_MORE_MENU_ID = "PLACEPAGE_MORE_MENU_BOTTOM_SHEET";
  private final int mMaxButtons;

  private final PlacePageView mPlacePage;
  private final ViewGroup mFrame;
  private final ItemListener mItemListener;

  private List<PlacePageButton> mPrevItems;
  private List<PlacePageButton>  mMoreItems;

  interface PlacePageButton
  {
    @StringRes
    int getTitle();

    ImageResources getIcon();

    @NonNull
    ButtonType getType();

    @DrawableRes
    int getBackgroundResource();

    class ImageResources
    {
      @DrawableRes
      private final int mEnabledStateResId;
      @DrawableRes
      private final int mDisabledStateResId;

      public ImageResources(@DrawableRes int enabledStateResId, @DrawableRes int disabledStateResId)
      {
        mEnabledStateResId = enabledStateResId;
        mDisabledStateResId = disabledStateResId;
      }

      public ImageResources(@DrawableRes int enabledStateResId)
      {
        this(enabledStateResId, enabledStateResId);
      }

      @DrawableRes
      public int getDisabledStateResId()
      {
        return mDisabledStateResId;
      }

      @DrawableRes
      public int getEnabledStateResId(@NonNull Context context)
      {
        return mEnabledStateResId;
      }

      public static class Stub extends ImageResources
      {
        public Stub()
        {
          super(UiUtils.NO_ID);
        }

        @Override
        public int getDisabledStateResId()
        {
          throw new UnsupportedOperationException("Not supported here");
        }

        @Override
        public int getEnabledStateResId(@NonNull Context context)
        {
          throw new UnsupportedOperationException("Not supported here");
        }
      }
    }
  }

  enum ButtonType {
    BACK,
    BOOKMARK,
    ROUTE_FROM,
    ROUTE_TO,
    ROUTE_ADD,
    ROUTE_REMOVE,
    ROUTE_AVOID_TOLL,
    ROUTE_AVOID_FERRY,
    ROUTE_AVOID_UNPAVED,
    SHARE,
    MORE,
    CALL
  }

  enum Item implements PlacePageButtons.PlacePageButton
  {
    BACK(
        R.string.back,
        new ImageResources.Stub()
        {
          @Override
          public int getEnabledStateResId(@NonNull Context context)
          {
            return ThemeUtils.getResource(MwmApplication.from(context),
                                          android.R.attr.homeAsUpIndicator);
          }
        },
        ButtonType.BACK),

    BOOKMARK_SAVE(
        R.string.save,
        new ImageResources(R.drawable.ic_bookmarks_on),
        ButtonType.BOOKMARK),

    BOOKMARK_DELETE(
        R.string.delete,
        new ImageResources(R.drawable.ic_bookmarks_off),
        ButtonType.BOOKMARK),

    ROUTE_FROM(
        R.string.p2p_from_here,
        new ImageResources(R.drawable.ic_route_from),
        ButtonType.ROUTE_FROM),

    ROUTE_TO(
        R.string.p2p_to_here,
        new ImageResources(R.drawable.ic_route_to),
        ButtonType.ROUTE_TO),

    ROUTE_ADD(
        R.string.placepage_add_stop,
        new ImageResources(R.drawable.ic_route_via),
        ButtonType.ROUTE_ADD),

    ROUTE_REMOVE(
        R.string.placepage_remove_stop,
        new ImageResources(R.drawable.ic_route_remove),
        ButtonType.ROUTE_REMOVE),

    ROUTE_AVOID_TOLL(
        R.string.avoid_toll_roads_placepage,
        new ImageResources(R.drawable.ic_avoid_tolls),
        ButtonType.ROUTE_AVOID_TOLL),

    ROUTE_AVOID_UNPAVED(
        R.string.avoid_unpaved_roads_placepage,
        new ImageResources(R.drawable.ic_avoid_unpaved),
        ButtonType.ROUTE_AVOID_UNPAVED),

    ROUTE_AVOID_FERRY(
        R.string.avoid_ferry_crossing_placepage,
        new ImageResources(R.drawable.ic_avoid_ferry),
        ButtonType.ROUTE_AVOID_FERRY),

    SHARE(
        R.string.share,
        new ImageResources(R.drawable.ic_share),
        ButtonType.SHARE),

    // Must not be used outside
    MORE(
        R.string.placepage_more_button,
        new ImageResources(R.drawable.ic_more),
        ButtonType.MORE),

    CALL(
        R.string.placepage_call_button,
        new ImageResources(R.drawable.ic_place_page_phone),
        ButtonType.CALL);

    @StringRes
    private final int mTitleId;

    @NonNull
    private final ImageResources mIconId;

    @NonNull
    private final ButtonType mButtonType;

    Item(@StringRes int titleId, @NonNull ImageResources iconId, @NonNull ButtonType buttonType)
    {
      mTitleId = titleId;
      mIconId = iconId;
      mButtonType = buttonType;
    }

    @StringRes
    @Override
    public int getTitle()
    {
      return mTitleId;
    }

    @NonNull
    @Override
    public ImageResources getIcon()
    {
      return mIconId;
    }

    @NonNull
    @Override
    public ButtonType getType()
    {
      return mButtonType;
    }

    @DrawableRes
    @Override
    public int getBackgroundResource()
    {
      throw new UnsupportedOperationException("Not supported!");
    }
  }

  interface ItemListener
  {
    void onPrepareVisibleView(@NonNull PlacePageButton item, @NonNull View frame,
                              @NonNull ImageView icon, @NonNull TextView title);
    void onItemClick(PlacePageButton item);
  }

  PlacePageButtons(PlacePageView placePage, ViewGroup frame, ItemListener itemListener)
  {
    mPlacePage = placePage;
    mFrame = frame;
    mItemListener = itemListener;

    mMaxButtons = mPlacePage.getContext().getResources().getInteger(R.integer.pp_buttons_max);
  }

  private @NonNull List<PlacePageButtons.PlacePageButton> collectButtons(List<PlacePageButtons.PlacePageButton> items)
  {
    List<PlacePageButtons.PlacePageButton> res = new ArrayList<>(items);
    if (res.size() > mMaxButtons)
      res.add(mMaxButtons - 1, Item.MORE);

    // Swap ROUTE_FROM and ROUTE_TO if the latter one was pressed out to bottomsheet
    int from = res.indexOf(Item.ROUTE_FROM);
    if (from > -1)
    {
      int addStop = res.indexOf(Item.ROUTE_ADD);
      int to = res.indexOf(Item.ROUTE_TO);
      if ((to > from && to >= mMaxButtons) || (to > from && addStop >= mMaxButtons))
        Collections.swap(res, from, to);

      if (addStop >= mMaxButtons)
      {
        from = res.indexOf(Item.ROUTE_FROM);
        if (addStop > from)
          Collections.swap(res, from, addStop);
      }

      preserveRoutingButtons(res, Item.CALL);
      from = res.indexOf(Item.ROUTE_FROM);
      to = res.indexOf(Item.ROUTE_TO);
      if (from < mMaxButtons && from > to)
        Collections.swap(res, to, from);
    }

    return res;
  }

  private void preserveRoutingButtons(@NonNull List<PlacePageButton> items, @NonNull Item itemToShift)
  {
    if (!RoutingController.get().isNavigating() && !RoutingController.get().isPlanning())
      return;

    int pos = items.indexOf(itemToShift);
    if (pos > -1)
    {
      items.remove(pos);
      items.add(mMaxButtons, itemToShift);
      int to = items.indexOf(Item.ROUTE_TO);
      if (items.indexOf(Item.ROUTE_ADD) > -1)
      {
        items.remove(Item.ROUTE_ADD);
        items.remove(Item.ROUTE_FROM);
        items.add(to + 1, Item.ROUTE_ADD);
        items.add(mMaxButtons, Item.ROUTE_FROM);
      }
      else
      {
        items.remove(Item.ROUTE_FROM);
        items.add(to + 1, Item.ROUTE_FROM);
      }
    }
  }

  @Nullable
  public ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems()
  {
    if (mMoreItems == null)
      return null;
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    for (int i = mMaxButtons; i < mMoreItems.size(); i++)
    {
      final PlacePageButton bsItem = mMoreItems.get(i);
      int iconRes = bsItem.getIcon().getEnabledStateResId(mPlacePage.getContext());
      items.add(new MenuBottomSheetItem(bsItem.getTitle(), iconRes, () -> mItemListener.onItemClick(bsItem)));
    }
    return items;
  }

  private void showPopup()
  {
    MenuBottomSheetFragment.newInstance(PLACEPAGE_MORE_MENU_ID)
        .show(mPlacePage.requireActivity().getSupportFragmentManager(), PLACEPAGE_MORE_MENU_ID);
  }

  private View createButton(@NonNull final List<PlacePageButton> items,
                            @NonNull final PlacePageButton current)
  {
    Context context = mPlacePage.getContext();
    LayoutInflater inflater = LayoutInflater.from(context);
    View parent = inflater.inflate(R.layout.place_page_button, mFrame, false);

    ImageView icon = parent.findViewById(R.id.icon);
    TextView title = parent.findViewById(R.id.title);

    title.setText(current.getTitle());
    icon.setImageResource(current.getIcon().getEnabledStateResId(context));
    mItemListener.onPrepareVisibleView(current, parent, icon, title);
    if (current == Item.MORE)
      mMoreItems = items;
    parent.setOnClickListener(new ShowPopupClickListener(current));
    return parent;
  }

  void setItems(List<PlacePageButton> items)
  {
    final List<PlacePageButton> buttons = collectButtons(items);
    if (buttons.equals(mPrevItems))
      return;

    mFrame.removeAllViews();
    int count = Math.min(buttons.size(), mMaxButtons);
    for (int i = 0; i < count; i++)
      mFrame.addView(createButton(buttons, buttons.get(i)));

    mPrevItems = buttons;
  }

  private class ShowPopupClickListener implements View.OnClickListener
  {
    @NonNull
    private final PlacePageButton mCurrent;

    public ShowPopupClickListener(@NonNull PlacePageButton current)
    {
      mCurrent = current;
    }

    @Override
    public void onClick(View v)
    {
      if (mCurrent == Item.MORE)
        showPopup();
      else
        mItemListener.onItemClick(mCurrent);
    }
  }
}
