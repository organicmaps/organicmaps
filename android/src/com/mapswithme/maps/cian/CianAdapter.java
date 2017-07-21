package com.mapswithme.maps.cian;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseSponsoredAdapter;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.List;

public final class CianAdapter extends BaseSponsoredAdapter
{
  public CianAdapter(@NonNull String url, boolean hasError, @Nullable ItemSelectedListener listener)
  {
    super(Sponsored.TYPE_CIAN, url, hasError, listener);
  }

  public CianAdapter(@NonNull RentPlace[] items, @NonNull String url,
                       @Nullable ItemSelectedListener listener)
  {
    super(Sponsored.TYPE_CIAN, convertItems(items), url, listener);
  }

  @NonNull
  private static List<Item> convertItems(@NonNull RentPlace[] items)
  {
    List<Item> viewItems = new ArrayList<>();
    for (RentPlace place : items)
    {
      RentOffer product = place.getOffers().get(0);
//      Context context = MwmApplication.get();
//      String title = context.getString(R.string.room, Integer.toString(product.getRoomsCount()))
//          + " " + Integer.toString(product.getFloorNumber()) + context.getString(R.string.area);
//      String price = context.getString(R.string.rub_month);
//      viewItems.add(new Item(title, product.getUrl(), price, product.getAddress()));
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
  protected ViewHolder createLoadingViewHolder(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return new LoadingViewHolder(inflater.inflate(R.layout.item_cian_loading, parent, false),
                                 this);
  }

  @NonNull
  @Override
  protected String getLoadingTitle()
  {
    return null;
  }

  @Nullable
  @Override
  protected String getLoadingSubtitle()
  {
    return null;
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
      super(TYPE_PRODUCT, Sponsored.TYPE_CIAN, title, url, null, false, false);
      mPrice = price;
      mAddress = address;
    }
  }
}
