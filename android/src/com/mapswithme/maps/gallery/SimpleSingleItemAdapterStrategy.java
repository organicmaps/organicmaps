package com.mapswithme.maps.gallery;

import android.content.Context;
import android.content.res.Resources;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.MwmApplication;

public abstract class SimpleSingleItemAdapterStrategy<T extends Holders.BaseViewHolder<Items.Item>>
    extends SingleItemAdapterStrategy<T>
{
  protected SimpleSingleItemAdapterStrategy(@NonNull Context context,
                                            @Nullable ItemSelectedListener<Items.Item> listener,
                                            @Nullable String url)
  {
    super(context, url, listener);
  }

  protected SimpleSingleItemAdapterStrategy(@NonNull Context context,
                                            @Nullable ItemSelectedListener<Items.Item> listener)
  {
    this(context, listener, null);
  }

  @Override
  protected void buildItem(@NonNull Context context, @Nullable String url)
  {
    Resources res = MwmApplication.from(context).getResources();
    mItems.add(new Items.Item(res.getString(getTitle()), null, null));
  }

  @NonNull
  @Override
  protected T createViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    View itemView = inflateView(LayoutInflater.from(parent.getContext()), parent);
    return createViewHolder(itemView);
  }

  @Override
  protected final int getSubtitle()
  {
    throw new UnsupportedOperationException("Subtitle is not supported by this strategy!");
  }

  @Override
  protected final int getLabelForDetailsView()
  {
    throw new UnsupportedOperationException("Details button is not supported by this strategy!");
  }
}
