package com.mapswithme.maps.gallery;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;

import android.os.Parcel;
import android.os.Parcelable;
import android.view.ViewGroup;

import java.util.List;

import static com.mapswithme.maps.gallery.Constants.TYPE_MORE;
import static com.mapswithme.maps.gallery.Constants.TYPE_PRODUCT;

public abstract class RegularAdapterStrategy<T extends RegularAdapterStrategy.Item>
    extends AdapterStrategy<Holders.BaseViewHolder<T>, T>
{
  private static final int MAX_ITEMS_BY_DEFAULT = 5;

  public RegularAdapterStrategy(@NonNull List<T> items, @Nullable T moreItem,
                                @Nullable ItemSelectedListener<T> listener)
  {
    this(items, moreItem, listener, MAX_ITEMS_BY_DEFAULT);
  }

  public RegularAdapterStrategy(@NonNull List<T> items, @Nullable T moreItem,
                                @Nullable ItemSelectedListener<T> listener, int maxItems)
  {
    super(listener);
    boolean showMoreItem = moreItem != null && items.size() >= maxItems;
    int size = showMoreItem ? maxItems : items.size();
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
  Holders.BaseViewHolder<T> createViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    switch (viewType)
    {
      case TYPE_PRODUCT:
        return createProductViewHolder(parent, viewType);
      case TYPE_MORE:
        return createMoreProductsViewHolder(parent, viewType);
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
                                                                       int viewType);
  @NonNull
  protected abstract Holders.BaseViewHolder<T> createMoreProductsViewHolder(@NonNull ViewGroup parent,
                                                                            int viewType);

  public static class Item extends Items.Item
  {
    @Constants.ViewType
    private final int mType;

    public Item(@Constants.ViewType int type, @StringRes int titleId,
                @Nullable String subtitle, @Nullable String url)
    {
      super(titleId, url, subtitle);
      mType = type;
    }

    public Item(@Constants.ViewType int type, @Nullable String title,
                @Nullable String subtitle, @Nullable String url)
    {
      super(title, url, subtitle);
      mType = type;
    }

    protected Item(Parcel in)
    {
      super(in);
      mType = in.readInt();
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      super.writeToParcel(dest, flags);
      dest.writeInt(mType);
    }

    public int getType()
    {
      return mType;
    }
  }
}
