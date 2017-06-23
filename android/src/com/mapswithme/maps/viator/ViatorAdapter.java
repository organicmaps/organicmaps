package com.mapswithme.maps.viator;

import android.content.Context;
import android.support.annotation.CallSuper;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.RatingBar;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

public final class ViatorAdapter extends RecyclerView.Adapter<ViatorAdapter.ViewHolder>
{
  private static final int MAX_ITEMS = 5;

  private static final int TYPE_PRODUCT = 0;
  private static final int TYPE_MORE = 1;

  private static final String MORE = MwmApplication.get().getString(R.string.placepage_more_button);

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_PRODUCT, TYPE_MORE })
  private @interface ViewType{}

  @NonNull
  private final List<Item> mItems;
  @Nullable
  private final ItemSelectedListener mListener;

  public ViatorAdapter(@NonNull ViatorProduct[] items, @NonNull String cityUrl,
                       @Nullable ItemSelectedListener listener)
  {
    mItems = new ArrayList<>();
    mListener = listener;
    boolean showMoreItem = items.length >= MAX_ITEMS;
    int size = showMoreItem ? MAX_ITEMS : items.length;
    for (int i = 0; i < size; i++)
    {
      ViatorProduct product = items[i];
      mItems.add(new Item(TYPE_PRODUCT, product.getPhotoUrl(), product.getTitle(),
                          product.getDuration(), product.getRating(), product.getPriceFormatted(),
                          product.getPageUrl()));
    }
    if (showMoreItem)
      mItems.add(new Item(TYPE_MORE, null, MORE, null, 0.0, null, cityUrl));
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, @ViewType int viewType)
  {
    switch (viewType)
    {
      case TYPE_PRODUCT:
        return new ProductViewHolder(LayoutInflater.from(parent.getContext())
                                                   .inflate(R.layout.item_viator_product,
                                                            parent, false), this);
      case TYPE_MORE:
        return new ViewHolder(LayoutInflater.from(parent.getContext())
                                            .inflate(R.layout.item_viator_more,
                                                     parent, false), this);
    }
    return null;
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  @Override
  @ViewType
  public int getItemViewType(int position)
  {
    return mItems.get(position).mType;
  }

  static class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
    @NonNull
    TextView mTitle;
    @NonNull
    ViatorAdapter mAdapter;

    ViewHolder(@NonNull View itemView, @NonNull ViatorAdapter adapter)
    {
      super(itemView);
      mTitle = (TextView) itemView.findViewById(R.id.tv__title);
      mAdapter = adapter;
      itemView.setOnClickListener(this);
    }

    @CallSuper
    void bind(@NonNull Item item)
    {
      mTitle.setText(item.mTitle);
    }

    @Override
    public void onClick(View v)
    {
      int position = getAdapterPosition();
      if (position == RecyclerView.NO_POSITION)
        return;

      onItemSelected(mAdapter.mItems.get(position));
    }

    void onItemSelected(@NonNull Item item)
    {
      if (mAdapter.mListener != null)
        mAdapter.mListener.onViatorItemSelected(item.mUrl);
    }
  }

  private static class ProductViewHolder extends ViewHolder
  {
    @NonNull
    ImageView mImage;
    @NonNull
    TextView mDuration;
    @NonNull
    RatingBar mRating;
    @NonNull
    TextView mPrice;

    @NonNull
    Context mContext;

    ProductViewHolder(@NonNull View itemView, @NonNull ViatorAdapter adapter)
    {
      super(itemView, adapter);
      mContext = itemView.getContext();
      mImage = (ImageView) itemView.findViewById(R.id.iv__image);
      mDuration = (TextView) itemView.findViewById(R.id.tv__duration);
      mRating = (RatingBar) itemView.findViewById(R.id.rb__rate);
      mPrice = (TextView) itemView.findViewById(R.id.tv__price);
    }

    @Override
    void bind(@NonNull Item item)
    {
      super.bind(item);

      if (item.mPhotoUrl != null)
      {
        Glide.with(mContext)
             .load(item.mPhotoUrl)
             .centerCrop()
             .into(mImage);
      }

      UiUtils.setTextAndHideIfEmpty(mDuration, item.mDuration);
      UiUtils.setTextAndHideIfEmpty(mPrice, item.mPrice);
      mRating.setRating((float) item.mRating);
    }
  }

  private static final class Item
  {
    @ViewType
    private final int mType;
    @Nullable
    private final String mPhotoUrl;
    @NonNull
    private final String mTitle;
    @Nullable
    private final String mDuration;
    private final double mRating;
    @Nullable
    private final String mPrice;
    @NonNull
    private final String mUrl;

    private Item(int type, @Nullable String photoUrl, @NonNull String title,
                 @Nullable String duration, double rating, @Nullable String price,
                 @NonNull String url)
    {
      mType = type;
      mPhotoUrl = photoUrl;
      mTitle = title;
      mDuration = duration;
      mRating = rating;
      mPrice = price;
      mUrl = url;
    }
  }

  public interface ItemSelectedListener
  {
    void onViatorItemSelected(@NonNull String url);
  }
}
