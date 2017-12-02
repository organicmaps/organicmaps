package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;

public abstract class LoadingAdapterStrategy
    extends SingleItemAdapterStrategy<Holders.LoadingViewHolder>
{

  protected LoadingAdapterStrategy(@Nullable String url)
  {
    super(url);
  }

  @NonNull
  @Override
  protected Holders.LoadingViewHolder createViewHolder(@NonNull ViewGroup parent, int viewType,
                                                       @NonNull GalleryAdapter adapter)
  {
    View loadingView = inflateView(LayoutInflater.from(parent.getContext()), parent);
    TextView moreView = (TextView) loadingView.findViewById(R.id.tv__more);
    moreView.setText(getLabelForDetailsView());
    return new Holders.LoadingViewHolder(loadingView, mItems, adapter);
  }
}
