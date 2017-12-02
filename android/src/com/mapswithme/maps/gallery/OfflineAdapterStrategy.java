package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;

public abstract class OfflineAdapterStrategy extends SingleItemAdapterStrategy<Holders.OfflineViewHolder>
{
  protected OfflineAdapterStrategy(@Nullable String url)
  {
    super(url);
  }

  @NonNull
  @Override
  Holders.OfflineViewHolder createViewHolder(@NonNull ViewGroup parent, int viewType, @NonNull GalleryAdapter adapter)
  {
    View offlineView = inflateView(LayoutInflater.from(parent.getContext()), parent);
    TextView moreView = (TextView) offlineView.findViewById(R.id.tv__more);
    moreView.setText(getLabelForDetailsView());
    return new Holders.OfflineViewHolder(offlineView, mItems, adapter);
  }

  @Override
  protected int getLabelForDetailsView()
  {
    return R.string.details;
  }
}
