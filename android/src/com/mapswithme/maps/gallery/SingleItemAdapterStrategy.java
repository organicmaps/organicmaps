package com.mapswithme.maps.gallery;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

abstract class SingleItemAdapterStrategy<T extends Holders.BaseViewHolder<Items.Item>>
    extends AdapterStrategy<T, Items.Item>
{
  SingleItemAdapterStrategy(@Nullable String url)
  {
    buildItem(url);
  }

  protected void buildItem(@Nullable String url)
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

  @NonNull
  @Override
  protected T createViewHolder(@NonNull ViewGroup parent, int viewType,
                               @NonNull GalleryAdapter<?, Items.Item> adapter)
  {
    View itemView = inflateView(LayoutInflater.from(parent.getContext()), parent);
    TextView button = (TextView) itemView.findViewById(R.id.button);
    button.setText(getLabelForDetailsView());
    return createViewHolder(itemView, adapter);
  }

  protected abstract T createViewHolder(@NonNull View itemView, @NonNull GalleryAdapter<?, Items
      .Item> adapter);

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
