
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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.ThemeUtils;

final class PlacePageButtons
{
  private final int MAX_BUTTONS;

  private final PlacePageView mPlacePage;
  private final ViewGroup mFrame;
  private final ItemListener mItemListener;

  private List<Item> mPrevItems;

  enum Item
  {
    BOOKING
    {
      @Override
      int getTitle()
      {
        return R.string.book_button;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_booking;
      }
    },

    BOOKING_SEARCH
    {
      @Override
      int getTitle()
      {
        return R.string.booking_search;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_menu_search;
      }
    },

    OPENTABLE
    {
      @Override
      int getTitle()
      {
        return R.string.book_button;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_opentable;
      }
    },

    THOR
    {
      @Override
      int getTitle()
      {
        return R.string.sponsored_thor_gather;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_24px_logo_thor;
      }
    },

    BACK
    {
      @Override
      int getTitle()
      {
        return R.string.back;
      }

      @Override
      int getIcon()
      {
        return ThemeUtils.getResource(MwmApplication.get(), android.R.attr.homeAsUpIndicator);
      }
    },

    BOOKMARK
    {
      @Override
      int getTitle()
      {
        return R.string.bookmark;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_bookmarks_off;
      }
    },

    ROUTE_FROM
    {
      @Override
      int getTitle()
      {
        return R.string.p2p_from_here;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_route_from;
      }
    },

    ROUTE_TO
    {
      @Override
      int getTitle()
      {
        return R.string.p2p_to_here;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_route_to;
      }
    },

    ROUTE_ADD
    {
      @Override
      int getTitle()
      {
        return R.string.placepage_add_stop;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_route_via;
      }
    },

    ROUTE_REMOVE
    {
      @Override
      int getTitle()
      {
        return R.string.placepage_remove_stop;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_route_remove;
      }
    },

    SHARE
    {
      @Override
      int getTitle()
      {
        return R.string.share;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_share;
      }
    },

    // Must not be used outside
    MORE
    {
      @Override
      int getTitle()
      {
        return R.string.placepage_more_button;
      }

      @Override
      int getIcon()
      {
        return R.drawable.bs_ic_more;
      }
    },
    CALL
    {
      @Override
      int getTitle()
      {
        return R.string.placepage_call_button;
      }

      @Override
      int getIcon()
      {
        return R.drawable.ic_place_page_phone;
      }
    };

    abstract @StringRes int getTitle();
    abstract @DrawableRes int getIcon();
  }

  interface ItemListener
  {
    void onPrepareVisibleView(Item item, View frame, ImageView icon, TextView title);
    void onItemClick(Item item);
  }

  PlacePageButtons(PlacePageView placePage, ViewGroup frame, ItemListener itemListener)
  {
    mPlacePage = placePage;
    mFrame = frame;
    mItemListener = itemListener;

    MAX_BUTTONS = mPlacePage.getContext().getResources().getInteger(R.integer.pp_buttons_max);
  }

  private @NonNull List<Item> collectButtons(List<Item> items)
  {
    List<Item> res = new ArrayList<>(items);
    if (res.size() > MAX_BUTTONS)
      res.add(MAX_BUTTONS - 1, Item.MORE);

    // Swap ROUTE_FROM and ROUTE_TO if the latter one was pressed out to bottomsheet
    int from = res.indexOf(Item.ROUTE_FROM);
    if (from > -1)
    {
      int addStop = res.indexOf(Item.ROUTE_ADD);
      int to = res.indexOf(Item.ROUTE_TO);
      if ((to > from && to >= MAX_BUTTONS) || (to > from && addStop >= MAX_BUTTONS))
        Collections.swap(res, from, to);

      if (addStop >= MAX_BUTTONS)
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
      if (from < MAX_BUTTONS && from > to)
        Collections.swap(res, to, from);
    }

    return res;
  }

  private void preserveRoutingButtons(@NonNull List<Item> items, @NonNull Item itemToShift)
  {
    if (!RoutingController.get().isNavigating() && !RoutingController.get().isPlanning())
      return;

    int pos = items.indexOf(itemToShift);
    if (pos > -1)
    {
      items.remove(pos);
      items.add(MAX_BUTTONS, itemToShift);
      int to = items.indexOf(Item.ROUTE_TO);
      if (items.indexOf(Item.ROUTE_ADD) > -1)
      {
        items.remove(Item.ROUTE_ADD);
        items.remove(Item.ROUTE_FROM);
        items.add(to + 1, Item.ROUTE_ADD);
        items.add(MAX_BUTTONS, Item.ROUTE_FROM);
      }
      else
      {
        items.remove(Item.ROUTE_FROM);
        items.add(to + 1, Item.ROUTE_FROM);
      }
    }
  }

  private void showPopup(final List<Item> buttons)
  {
    BottomSheetHelper.Builder bs = new BottomSheetHelper.Builder(mPlacePage.getActivity());
    for (int i = MAX_BUTTONS; i < buttons.size(); i++)
    {
      Item bsItem = buttons.get(i);
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

  private View createButton(final List<Item> items, final Item current)
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

  void setItems(List<Item> items)
  {
    final List<Item> buttons = collectButtons(items);
    if (buttons.equals(mPrevItems))
      return;

    mFrame.removeAllViews();
    int count = Math.min(buttons.size(), MAX_BUTTONS);
    for (int i = 0; i < count; i++)
      mFrame.addView(createButton(buttons, buttons.get(i)));

    mPrevItems = buttons;
  }
}
