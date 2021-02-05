package com.mapswithme.maps.gallery;

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
  protected SimpleSingleItemAdapterStrategy(@Nullable ItemSelectedListener<Items.Item> listener,
                                            @Nullable String url)
  {
    super(url, listener);
  }

  protected SimpleSingleItemAdapterStrategy(@Nullable ItemSelectedListener<Items.Item> listener)
  {
    this(listener, null);
  }

  @Override
  protected void buildItem(@Nullable String url)
  {
    Resources res = MwmApplication.get().getResources();
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
