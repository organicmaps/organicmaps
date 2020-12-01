package com.mapswithme.maps.gallery;

import android.content.Context;
import android.content.res.Resources;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

abstract class SingleItemAdapterStrategy<T extends Holders.BaseViewHolder<Items.Item>>
    extends AdapterStrategy<T, Items.Item>
{
  SingleItemAdapterStrategy(@NonNull Context context,
                            @Nullable String url,
                            @Nullable ItemSelectedListener<Items.Item> listener)
  {
    super(listener);
    buildItem(context, url);
  }

  protected void buildItem(@NonNull Context context, @Nullable String url)
  {
    Resources res = MwmApplication.from(context).getResources();
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
  protected T createViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    View itemView = inflateView(LayoutInflater.from(parent.getContext()), parent);
    TextView button = (TextView) itemView.findViewById(R.id.button);
    button.setText(getLabelForDetailsView());
    return createViewHolder(itemView);
  }

  protected abstract T createViewHolder(@NonNull View itemView);

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
