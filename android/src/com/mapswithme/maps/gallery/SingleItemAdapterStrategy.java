package com.mapswithme.maps.gallery;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.MwmApplication;

abstract class SingleItemAdapterStrategy<T extends Holders.BaseViewHolder<Items.Item>>
    extends AdapterStrategy<T, Items.Item>
{
  SingleItemAdapterStrategy(@Nullable String url)
  {
    Resources res = MwmApplication.get().getResources();
    mItems.add(new Items.Item(res.getString(getTitle()), url,
                              res.getString(getSubtitle())));
  }

  @StringRes
  protected abstract int getTitle();

  @StringRes
  protected abstract int getSubtitle();

  @NonNull
  protected abstract View inflateView(@NonNull LayoutInflater inflater,
                                      @NonNull ViewGroup parent);

  @StringRes
  protected abstract int getLabelForDetailsView();

  @Override
  protected void onBindViewHolder(Holders.BaseViewHolder<Items.Item> holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  int getItemViewType(int position)
  {
    return 0;
  }
}
