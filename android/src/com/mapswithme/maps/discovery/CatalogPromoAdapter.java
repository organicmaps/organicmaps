package com.mapswithme.maps.discovery;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.mapswithme.maps.R;

import java.util.Collections;
import java.util.List;

public class CatalogPromoAdapter extends RecyclerView.Adapter<CatalogPromoAdapter.CatalogPromoHolder>
{
  @NonNull
  private List<CatalogPromoItem> mCatalogPromoItems = Collections.emptyList();

  @NonNull
  @Override
  public CatalogPromoHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View container = inflater.inflate(R.layout.catalog_promo_item_card, parent, false);
    return new CatalogPromoHolder(container);
  }

  @Override
  public void onBindViewHolder(@NonNull CatalogPromoHolder holder, int position)
  {
    CatalogPromoItem item = mCatalogPromoItems.get(position);
    holder.mSubTitle.setText(item.getDescription());
    holder.mTitle.setText(item.getTitle());
    Glide.with(holder.itemView.getContext())
         .load(Uri.parse(item.getImgUrl()))
         .placeholder(R.drawable.img_guides_gallery_placeholder)
         .into(holder.mImage);
  }

  @Override
  public int getItemCount()
  {
    return mCatalogPromoItems.size();
  }

  public void setData(@NonNull List<CatalogPromoItem> items)
  {
    mCatalogPromoItems = items;
  }

  static class CatalogPromoHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final ImageView mImage;

    @NonNull
    private final TextView mTitle;

    @NonNull
    private final TextView mSubTitle;

    CatalogPromoHolder(@NonNull View itemView)
    {
      super(itemView);
      mImage = itemView.findViewById(R.id.image);
      mTitle = itemView.findViewById(R.id.title);
      mSubTitle = itemView.findViewById(R.id.subtitle);
    }
  }
}
