
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

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

final class PlacePageButtons
{
  private static final Map<Integer, PartnerItem> PARTNERS_ITEMS = new HashMap<Integer, PartnerItem>()
  {{
    put(PartnerItem.PARTNER1.getIndex(), PartnerItem.PARTNER1);
    put(PartnerItem.PARTNER3.getIndex(), PartnerItem.PARTNER3);
  }};

  private final int mMaxButtons;

  private final PlacePageView mPlacePage;
  private final ViewGroup mFrame;
  private final ItemListener mItemListener;

  private List<ButtonInterface> mPrevItems;

  interface ButtonInterface
  {
    @StringRes
    int getTitle();

    @DrawableRes
    int getIcon();

    @NonNull
    ButtonType getType();
  }

  enum ButtonType
  {
    PARTNER1, PARTNER3, BOOKING, BOOKING_SEARCH, OPENTABLE, BACK, BOOKMARK,
    ROUTE_FROM, ROUTE_TO, ROUTE_ADD, ROUTE_REMOVE, SHARE, MORE, CALL
  }

  private enum PartnerItem implements ButtonInterface
  {
    PARTNER1(1)
    {
      @Override
      public int getTitle()
      {
        return R.string.sponsored_partner1_action;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_24px_logo_partner1;
      }

      @NonNull
      public ButtonType getType()
      {
        return ButtonType.PARTNER1;
      }
    },

    PARTNER3(3)
    {
      @Override
      public int getTitle()
      {
        return R.string.sponsored_partner3_action;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_24px_logo_partner3;
      }

      @Override
      @NonNull
      public ButtonType getType()
      {
        return ButtonType.PARTNER3;
      }
    };

    private final int mIndex;

    PartnerItem(int index)
    {
      mIndex = index;
    }

    public int getIndex()
    {
      return mIndex;
    }
  }

  enum Item implements ButtonInterface
  {
    BOOKING
    {
      @Override
      public int getTitle()
      {
        return R.string.book_button;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_booking;
      }

      @Override
      @NonNull
      public ButtonType getType()
      {
        return ButtonType.BOOKING;
      }
    },

    BOOKING_SEARCH
    {
      @Override
      public int getTitle()
      {
        return R.string.booking_search;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_menu_search;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.BOOKING_SEARCH;
      }
    },

    OPENTABLE
    {
      @Override
      public int getTitle()
      {
        return R.string.book_button;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_opentable;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.OPENTABLE;
      }
    },

    BACK
    {
      @Override
      public int getTitle()
      {
        return R.string.back;
      }

      @Override
      public int getIcon()
      {
        return ThemeUtils.getResource(MwmApplication.get(), android.R.attr.homeAsUpIndicator);
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.BACK;
      }
    },

    BOOKMARK
    {
      @Override
      public int getTitle()
      {
        return R.string.bookmark;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_bookmarks_off;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.BOOKMARK;
      }
    },

    ROUTE_FROM
    {
      @Override
      public int getTitle()
      {
        return R.string.p2p_from_here;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_route_from;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.ROUTE_FROM;
      }
    },

    ROUTE_TO
    {
      @Override
      public int getTitle()
      {
        return R.string.p2p_to_here;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_route_to;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.ROUTE_TO;
      }
    },

    ROUTE_ADD
    {
      @Override
      public int getTitle()
      {
        return R.string.placepage_add_stop;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_route_via;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.ROUTE_ADD;
      }
    },

    ROUTE_REMOVE
    {
      @Override
      public int getTitle()
      {
        return R.string.placepage_remove_stop;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_route_remove;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.ROUTE_REMOVE;
      }
    },

    SHARE
    {
      @Override
      public int getTitle()
      {
        return R.string.share;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_share;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.SHARE;
      }
    },

    // Must not be used outside
    MORE
    {
      @Override
      public int getTitle()
      {
        return R.string.placepage_more_button;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.bs_ic_more;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.MORE;
      }
    },

    CALL
    {
      @Override
      public int getTitle()
      {
        return R.string.placepage_call_button;
      }

      @Override
      public int getIcon()
      {
        return R.drawable.ic_place_page_phone;
      }

      @NonNull
      @Override
      public ButtonType getType()
      {
        return ButtonType.CALL;
      }
    };

    @StringRes
    public int getTitle()
    {
      throw new UnsupportedOperationException("Not supported!");
    }

    @DrawableRes
    public int getIcon()
    {
      throw new UnsupportedOperationException("Not supported!");
    }
  }

  interface ItemListener
  {
    void onPrepareVisibleView(ButtonInterface item, View frame, ImageView icon, TextView title);
    void onItemClick(ButtonInterface item);
  }

  PlacePageButtons(PlacePageView placePage, ViewGroup frame, ItemListener itemListener)
  {
    mPlacePage = placePage;
    mFrame = frame;
    mItemListener = itemListener;

    mMaxButtons = mPlacePage.getContext().getResources().getInteger(R.integer.pp_buttons_max);
  }

  @NonNull
  static ButtonInterface getPartnerItem(int partnerIndex)
  {
    ButtonInterface item = PARTNERS_ITEMS.get(partnerIndex);
    if (item == null)
      throw new AssertionError("Wrong partner index: " + partnerIndex);
    return item;
  }

  private @NonNull List<ButtonInterface> collectButtons(List<ButtonInterface> items)
  {
    List<ButtonInterface> res = new ArrayList<>(items);
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

  private void preserveRoutingButtons(@NonNull List<ButtonInterface> items, @NonNull Item itemToShift)
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

  private void showPopup(final List<ButtonInterface> buttons)
  {
    BottomSheetHelper.Builder bs = new BottomSheetHelper.Builder(mPlacePage.getActivity());
    for (int i = mMaxButtons; i < buttons.size(); i++)
    {
      ButtonInterface bsItem = buttons.get(i);
      bs.sheet(i, bsItem.getIcon(), bsItem.getTitle());
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

  private View createButton(final List<ButtonInterface> items, final ButtonInterface current)
  {
    View res = LayoutInflater.from(mPlacePage.getContext()).inflate(R.layout.place_page_button, mFrame, false);

    ImageView icon = (ImageView) res.findViewById(R.id.icon);
    TextView title = (TextView) res.findViewById(R.id.title);

    icon.setImageResource(current.getIcon());
    title.setText(current.getTitle());
    mItemListener.onPrepareVisibleView(current, res, icon, title);

    res.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (current == Item.MORE)
          showPopup(items);
        else
          mItemListener.onItemClick(current);
      }
    });


    return res;
  }

  void setItems(List<ButtonInterface> items)
  {
    final List<ButtonInterface> buttons = collectButtons(items);
    if (buttons.equals(mPrevItems))
      return;

    mFrame.removeAllViews();
    int count = Math.min(buttons.size(), mMaxButtons);
    for (int i = 0; i < count; i++)
      mFrame.addView(createButton(buttons, buttons.get(i)));

    mPrevItems = buttons;
  }
}
