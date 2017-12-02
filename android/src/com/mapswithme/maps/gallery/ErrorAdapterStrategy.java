package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;

public abstract class ErrorAdapterStrategy extends SingleItemAdapterStrategy<Holders.ErrorViewHolder>
{
  protected ErrorAdapterStrategy(@Nullable String url)
  {
    super(url);
  }

  @NonNull
  @Override
  Holders.ErrorViewHolder createViewHolder(@NonNull ViewGroup parent, int viewType,
                                           @NonNull GalleryAdapter adapter)
  {
    View errorView = inflateView(LayoutInflater.from(parent.getContext()), parent);
    TextView moreView = (TextView) errorView.findViewById(R.id.tv__more);
    moreView.setText(getLabelForDetailsView());
    return new Holders.ErrorViewHolder(errorView, mItems, adapter);
  }
}
