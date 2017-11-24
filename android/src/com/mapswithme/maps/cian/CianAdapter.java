package com.mapswithme.maps.cian;

import android.content.Context;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseSponsoredAdapter;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.List;

public final class CianAdapter extends BaseSponsoredAdapter
{
  private static final String LOADING_TITLE = MwmApplication
      .get().getString(R.string.preloader_cian_title);
  private static final String LOADING_SUBTITLE = MwmApplication
      .get().getString(R.string.preloader_cian_message);

  public CianAdapter(@NonNull String url, boolean hasError, @Nullable ItemSelectedListener listener)
  {
    super(url, hasError, listener);
  }

  public CianAdapter(@NonNull RentPlace[] items, @NonNull String url,
                       @Nullable ItemSelectedListener listener)
  {
    super(convertItems(items), url, listener, true);
  }

  @NonNull
  private static List<Item> convertItems(@NonNull RentPlace[] items)
  {
    List<Item> viewItems = new ArrayList<>();
    for (RentPlace place : items)
    {
      if (place.getOffers().isEmpty())
        continue;

      RentOffer product = place.getOffers().get(0);
      Context context = MwmApplication.get();
      String title = context.getString(R.string.room, Integer.toString(product.getRoomsCount()));
      String price = Integer.toString((int) product.getPrice()) + " "
                     + context.getString(R.string.rub_month);
      viewItems.add(new Item(title, product.getUrl(), price, product.getAddress()));
    }

    return viewItems;
  }

  @Override
  @NonNull
  protected BaseSponsoredAdapter.ViewHolder createViewHolder(@NonNull LayoutInflater inflater,
                                                             @NonNull ViewGroup parent)
  {
    return new ProductViewHolder(inflater.inflate(R.layout.item_cian_product, parent, false),
                                 this);
  }

  @NonNull
  @Override
  protected View inflateLoadingView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.item_cian_loading, parent, false);
  }

  @Override
  protected int getMoreLabelForLoadingView()
  {
    return R.string.preloader_cian_button;
  }

  @NonNull
  @Override
  protected String getLoadingTitle()
  {
    return LOADING_TITLE;
  }

  @Nullable
  @Override
  protected String getLoadingSubtitle()
  {
    return LOADING_SUBTITLE;
  }

  @LayoutRes
  @Override
  protected int getMoreLayout()
  {
    return R.layout.item_cian_more;
  }

  private static final class ProductViewHolder extends ViewHolder
  {
    @NonNull
    TextView mPrice;
    @NonNull
    TextView mAddress;

    ProductViewHolder(@NonNull View itemView, @NonNull CianAdapter adapter)
    {
      super(itemView, adapter);
      mPrice = (TextView) itemView.findViewById(R.id.tv__price);
      mAddress = (TextView) itemView.findViewById(R.id.tv__address);
    }

    @Override
    public void bind(@NonNull BaseSponsoredAdapter.Item item)
    {
      super.bind(item);

      Item product = (Item) item;
      UiUtils.setTextAndHideIfEmpty(mPrice, product.mPrice);
      UiUtils.setTextAndHideIfEmpty(mAddress, product.mAddress);
    }
  }

  private static final class Item extends BaseSponsoredAdapter.Item
  {
    @NonNull
    private final String mPrice;
    @NonNull
    private final String mAddress;

    private Item(@NonNull String title, @NonNull String url, @NonNull String price,
                 @NonNull String address)
    {
      super(TYPE_PRODUCT, title, url, null, false, false);
      mPrice = price;
      mAddress = address;
    }
  }
}
