package com.mapswithme.maps.widget.placepage;

import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.mapswithme.maps.widget.placepage.PlacePageButtons.PARTNERS_ITEMS;

public final class PlacePageButtons
{
  private static final Map<Integer, PartnerItem> PARTNERS_ITEMS = new HashMap<Integer, PartnerItem>()
  {{
    put(PartnerItem.PARTNER1.getIndex(), PartnerItem.PARTNER1);
    put(PartnerItem.PARTNER3.getIndex(), PartnerItem.PARTNER3);
    put(PartnerItem.PARTNER4.getIndex(), PartnerItem.PARTNER4);
    put(PartnerItem.PARTNER5.getIndex(), PartnerItem.PARTNER5);
    put(PartnerItem.PARTNER6.getIndex(), PartnerItem.PARTNER6);
    put(PartnerItem.PARTNER7.getIndex(), PartnerItem.PARTNER7);
    put(PartnerItem.PARTNER8.getIndex(), PartnerItem.PARTNER8);
    put(PartnerItem.PARTNER9.getIndex(), PartnerItem.PARTNER9);
    put(PartnerItem.PARTNER10.getIndex(), PartnerItem.PARTNER10);
    put(PartnerItem.PARTNER11.getIndex(), PartnerItem.PARTNER11);
    put(PartnerItem.PARTNER12.getIndex(), PartnerItem.PARTNER12);
    put(PartnerItem.PARTNER13.getIndex(), PartnerItem.PARTNER13);
    put(PartnerItem.PARTNER14.getIndex(), PartnerItem.PARTNER14);
    put(PartnerItem.PARTNER15.getIndex(), PartnerItem.PARTNER15);
    put(PartnerItem.PARTNER16.getIndex(), PartnerItem.PARTNER16);
    put(PartnerItem.PARTNER17.getIndex(), PartnerItem.PARTNER17);
  }};

  private final int mMaxButtons;

  private final PlacePageView mPlacePage;
  private final ViewGroup mFrame;
  private final ItemListener mItemListener;

  private List<PlacePageButton> mPrevItems;

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
      public int getEnabledStateResId()
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
        public int getEnabledStateResId()
        {
          throw new UnsupportedOperationException("Not supported here");
        }
      }
    }
  }

  enum ButtonType {
    PARTNER1,
    PARTNER3,
    PARTNER4,
    PARTNER5,
    PARTNER6,
    PARTNER7,
    PARTNER8,
    PARTNER9,
    PARTNER10,
    PARTNER11,
    PARTNER12,
    PARTNER13,
    PARTNER14,
    PARTNER15,
    PARTNER16,
    PARTNER17,
    BOOKING,
    BOOKING_SEARCH,
    OPENTABLE,
    BACK,
    BOOKMARK,
    ROUTE_FROM,
    ROUTE_TO,
    ROUTE_ADD,
    ROUTE_REMOVE,
    SHARE,
    MORE,
    CALL
  }

  enum PartnerItem implements PlacePageButtons.PlacePageButton
  {
    PARTNER1(
        1,
        R.string.sponsored_partner1_action,
        new ImageResources(R.drawable.ic_24px_logo_partner1),
        R.drawable.button_partner1,
        ButtonType.PARTNER1),
    PARTNER3(
        3,
        R.string.sponsored_partner3_action,
        new ImageResources(R.drawable.ic_24px_logo_partner3),
        R.drawable.button_partner3,
        ButtonType.PARTNER3),
    PARTNER4(
        4,
        R.string.sponsored_partner4_action,
        new ImageResources(R.drawable.ic_24px_logo_partner4),
        R.drawable.button_partner4,
        ButtonType.PARTNER4),
    PARTNER5(
        5,
        R.string.sponsored_partner5_action,
        new ImageResources(R.drawable.ic_24px_logo_partner5),
        R.drawable.button_partner5,
        ButtonType.PARTNER5),
    PARTNER6(
        6,
        R.string.sponsored_partner6_action,
        new ImageResources(R.drawable.ic_24px_logo_partner6),
        R.drawable.button_partner6,
        ButtonType.PARTNER6),
    PARTNER7(
        7,
        R.string.sponsored_partner7_action,
        new ImageResources(R.drawable.ic_24px_logo_partner7),
        R.drawable.button_partner7,
        ButtonType.PARTNER7),
    PARTNER8(
        8,
        R.string.sponsored_partner8_action,
        new ImageResources(R.drawable.ic_24px_logo_partner8),
        R.drawable.button_partner8,
        ButtonType.PARTNER8),
    PARTNER9(
        9,
        R.string.sponsored_partner9_action,
        new ImageResources(R.drawable.ic_24px_logo_partner9),
        R.drawable.button_partner9,
        ButtonType.PARTNER9),
    PARTNER10(
        10,
        R.string.sponsored_partner10_action,
        new ImageResources(R.drawable.ic_24px_logo_partner10),
        R.drawable.button_partner10,
        ButtonType.PARTNER10),
    PARTNER11(
        11,
        R.string.sponsored_partner11_action,
        new ImageResources(R.drawable.ic_24px_logo_partner11),
        R.drawable.button_partner11,
        ButtonType.PARTNER11),
    PARTNER12(
        12,
        R.string.sponsored_partner12_action,
        new ImageResources(R.drawable.ic_24px_logo_partner12),
        R.drawable.button_partner12,
        ButtonType.PARTNER12),
    PARTNER13(
        13,
        R.string.sponsored_partner13_action,
        new ImageResources(R.drawable.ic_24px_logo_partner13),
        R.drawable.button_partner13,
        ButtonType.PARTNER13),
    PARTNER14(
        14,
        R.string.sponsored_partner14_action,
        new ImageResources(R.drawable.ic_24px_logo_partner14),
        R.drawable.button_partner14,
        ButtonType.PARTNER14),
    PARTNER15(
        15,
        R.string.sponsored_partner15_action,
        new ImageResources(R.drawable.ic_24px_logo_partner15),
        R.drawable.button_partner15,
        ButtonType.PARTNER15),
    PARTNER16(
        16,
        R.string.sponsored_partner16_action,
        new ImageResources(R.drawable.ic_24px_logo_partner16),
        R.drawable.button_partner16,
        ButtonType.PARTNER16),
    PARTNER17(
        17,
        R.string.sponsored_partner17_action,
        new ImageResources(R.drawable.ic_24px_logo_partner17),
        R.drawable.button_partner17,
        ButtonType.PARTNER17);

    private final int mIndex;
    @StringRes
    private final int mTitleId;
    private final ImageResources mIconId;
    @DrawableRes
    private final int mBackgroundId;
    @NonNull
    private final ButtonType mButtonType;

    PartnerItem(int index, @StringRes int titleId, @NonNull ImageResources iconId,
                @DrawableRes int backgroundId, @NonNull ButtonType buttonType)
    {
      mIndex = index;
      mTitleId = titleId;
      mIconId = iconId;
      mBackgroundId = backgroundId;
      mButtonType = buttonType;
    }

    public int getIndex()
    {
      return mIndex;
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
      return mBackgroundId;
    }
  }

  enum Item implements PlacePageButtons.PlacePageButton
  {
    BOOKING(
        R.string.book_button,
        new ImageResources(R.drawable.ic_booking),
        ButtonType.BOOKING)
        {
          @DrawableRes
          @Override
          public int getBackgroundResource()
          {
            return R.drawable.button_booking;
          }
        },

    BOOKING_SEARCH(
        R.string.booking_search,
        new ImageResources(R.drawable.ic_menu_search),
        ButtonType.BOOKING_SEARCH)
        {
          @DrawableRes
          @Override
          public int getBackgroundResource()
          {
            return R.drawable.button_booking;
          }
        },

    OPENTABLE(
        R.string.book_button,
        new ImageResources(R.drawable.ic_opentable),
        ButtonType.OPENTABLE)
        {
          @DrawableRes
          @Override
          public int getBackgroundResource()
          {
            return R.drawable.button_opentable;
          }
        },

    BACK(
        R.string.back,
        new ImageResources.Stub()
        {
          @Override
          public int getEnabledStateResId()
          {
            return ThemeUtils.getResource(MwmApplication.get(), android.R.attr.homeAsUpIndicator);
          }
        },
        ButtonType.BACK),

    BOOKMARK(
        R.string.bookmark,
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

    SHARE(
        R.string.share,
        new ImageResources(R.drawable.ic_share),
        ButtonType.SHARE),

    // Must not be used outside
    MORE(
        R.string.placepage_more_button,
        new ImageResources(R.drawable.bs_ic_more),
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

  @NonNull
  static PlacePageButtons.PlacePageButton getPartnerItem(int partnerIndex)
  {
    PlacePageButtons.PlacePageButton item = PARTNERS_ITEMS.get(partnerIndex);
    if (item == null)
      throw new AssertionError("Wrong partner index: " + partnerIndex);
    return item;
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
      preserveRoutingButtons(res, Item.BOOKING);
      preserveRoutingButtons(res, Item.BOOKING_SEARCH);
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

  private void showPopup(final List<PlacePageButton> buttons)
  {
    BottomSheetHelper.Builder bs = new BottomSheetHelper.Builder(mPlacePage.getActivity());
    for (int i = mMaxButtons; i < buttons.size(); i++)
    {
      PlacePageButton bsItem = buttons.get(i);
      int iconRes = bsItem.getIcon().getEnabledStateResId();
      bs.sheet(i, iconRes, bsItem.getTitle());
    }

    bs.listener(new MenuItem.OnMenuItemClickListener()
    {
      @Override
      public boolean onMenuItemClick(MenuItem item)
      {
        mItemListener.onItemClick(buttons.get(item.getItemId()));
        return true;
      }
    });

    bs.tint().show();
  }

  private View createButton(@NonNull final List<PlacePageButton> items,
                            @NonNull final PlacePageButton current)
  {
    LayoutInflater inflater = LayoutInflater.from(mPlacePage.getContext());
    View parent = inflater.inflate(R.layout.place_page_button, mFrame, false);

    ImageView icon = (ImageView) parent.findViewById(R.id.icon);
    TextView title = (TextView) parent.findViewById(R.id.title);

    title.setText(current.getTitle());
    icon.setImageResource(current.getIcon().getEnabledStateResId());
    mItemListener.onPrepareVisibleView(current, parent, icon, title);
    parent.setOnClickListener(new ShowPopupClickListener(current, items));
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
    @NonNull
    private final List<PlacePageButton> mItems;

    public ShowPopupClickListener(@NonNull PlacePageButton current,
                                  @NonNull List<PlacePageButton> items)
    {
      mCurrent = current;
      mItems = items;
    }

    @Override
    public void onClick(View v)
    {
      if (mCurrent == Item.MORE)
        showPopup(mItems);
      else
        mItemListener.onItemClick(mCurrent);
    }
  }
}
