package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.ViewGroup;

import java.util.List;

import static com.mapswithme.maps.gallery.Constants.TYPE_MORE;
import static com.mapswithme.maps.gallery.Constants.TYPE_PRODUCT;

public abstract class RegularAdapterStrategy<T extends RegularAdapterStrategy.Item>
    extends AdapterStrategy<Holders.BaseViewHolder<T>, T>
{
  private static final int MAX_ITEMS = 5;

  public RegularAdapterStrategy(@NonNull List<T> items, @Nullable T moreItem)
  {
    boolean showMoreItem = moreItem != null && items.size() >= MAX_ITEMS;
    int size = showMoreItem ? MAX_ITEMS : items.size();
    for (int i = 0; i < size; i++)
    {
      T product = items.get(i);
      mItems.add(product);
    }
    if (showMoreItem)
      mItems.add(moreItem);
  }

  @NonNull
  @Override
  Holders.BaseViewHolder<T> createViewHolder(@NonNull ViewGroup parent, int viewType,
                                             @NonNull GalleryAdapter<?, T> adapter)
  {
    switch (viewType)
    {
      case TYPE_PRODUCT:
        return createProductViewHolder(parent, viewType, adapter);
      case TYPE_MORE:
        return createMoreProductsViewHolder(parent, viewType, adapter);
      default:
        throw new UnsupportedOperationException("This strategy doesn't support specified view type: "
                                                + viewType);
    }
  }

  @Override
  protected void onBindViewHolder(Holders.BaseViewHolder<T> holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  protected int getItemViewType(int position)
  {
    return mItems.get(position).getType();
  }

  @NonNull
  protected abstract Holders.BaseViewHolder<T> createProductViewHolder(@NonNull ViewGroup parent,
                                                                       int viewType,
                                                                       @NonNull GalleryAdapter<?, T> adapter);
  @NonNull
  protected abstract Holders.BaseViewHolder<T> createMoreProductsViewHolder(@NonNull ViewGroup parent,
                                                                            int viewType,
                                                                            @NonNull GalleryAdapter<?, T> adapter);

  public static class Item extends Items.Item
  {
    @Constants.ViewType
    private final int mType;

    protected Item(@Constants.ViewType int type, @NonNull String title,
                   @Nullable String url, @Nullable String subtitle)
    {
      super(title, url, subtitle);
      mType = type;
    }

    public int getType()
    {
      return mType;
    }
  }
}
