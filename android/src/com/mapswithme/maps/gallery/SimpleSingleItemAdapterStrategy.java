package com.mapswithme.maps.gallery;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.MwmApplication;

public abstract class SimpleSingleItemAdapterStrategy<T extends Holders.BaseViewHolder<Items.Item>>
    extends SingleItemAdapterStrategy<T>
{
  protected SimpleSingleItemAdapterStrategy()
  {
    super(null);
  }

  @Override
  protected void buildItem(@Nullable String url)
  {
    Resources res = MwmApplication.get().getResources();
    mItems.add(new Items.Item(res.getString(getTitle()), null, null));
  }

  @NonNull
  @Override
  protected T createViewHolder(@NonNull ViewGroup parent, int viewType,
                               @NonNull GalleryAdapter<?, Items.Item> adapter)
  {
    View itemView = inflateView(LayoutInflater.from(parent.getContext()), parent);
    return createViewHolder(itemView, adapter);
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
