package com.mapswithme.maps.gallery;

import android.content.Context;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.mapswithme.maps.R;
import com.mapswithme.maps.ugc.Impress;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.maps.widget.RatingView;
import com.mapswithme.util.UiUtils;

import java.util.List;

import static com.mapswithme.maps.gallery.Items.CianItem;
import static com.mapswithme.maps.gallery.Items.ViatorItem;

public class Holders
{
  public static final class ViatorProductViewHolder
      extends BaseViewHolder<ViatorItem>
  {
    @NonNull
    ImageView mImage;
    @NonNull
    TextView mDuration;
    @NonNull
    RatingView mRating;
    @NonNull
    TextView mPrice;

    @NonNull
    Context mContext;

    public ViatorProductViewHolder(@NonNull View itemView, @NonNull List<ViatorItem> items,
                                   @NonNull GalleryAdapter adapter)
    {
      super(itemView, items, adapter);
      mContext = itemView.getContext();
      mImage = (ImageView) itemView.findViewById(R.id.iv__image);
      mDuration = (TextView) itemView.findViewById(R.id.tv__duration);
      mRating = (RatingView) itemView.findViewById(R.id.ratingView);
      mPrice = (TextView) itemView.findViewById(R.id.tv__price);
    }

    @Override
    public void bind(@NonNull ViatorItem item)
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
      UiUtils.setTextAndHideIfEmpty(mPrice, mContext.getString(R.string.place_page_starting_from,
                                                               item.mPrice));
      float rating = (float) item.mRating;
      Impress impress = Impress.values()[UGC.nativeToImpress(rating)];
      mRating.setRating(impress, String.valueOf(rating));
    }

    @Override
    protected void onItemSelected(@NonNull ViatorItem item)
    {
      GalleryAdapter.ItemSelectedListener listener = mAdapter.getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onItemSelected(item.getUrl());
    }
  }

  public static final class ViatorMoreItemViewHolder extends BaseViewHolder<Items.ViatorItem>
  {

    public ViatorMoreItemViewHolder(@NonNull View itemView, @NonNull List<Items.ViatorItem> items,
                                    @NonNull GalleryAdapter adapter)
    {
      super(itemView, items, adapter);
    }

    @Override
    protected void onItemSelected(@NonNull Items.ViatorItem item)
    {
      GalleryAdapter.ItemSelectedListener listener = mAdapter.getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onMoreItemSelected(item.getUrl());
    }
  }

  public static final class CianProductViewHolder extends BaseViewHolder<CianItem>
  {
    @NonNull
    TextView mPrice;
    @NonNull
    TextView mAddress;

    public CianProductViewHolder(@NonNull View itemView, @NonNull List<CianItem> items, @NonNull GalleryAdapter adapter)
    {
      super(itemView, items, adapter);
      mPrice = (TextView) itemView.findViewById(R.id.tv__price);
      mAddress = (TextView) itemView.findViewById(R.id.tv__address);
    }

    @Override
    public void bind(@NonNull CianItem item)
    {
      super.bind(item);
      UiUtils.setTextAndHideIfEmpty(mPrice, item.mPrice);
      UiUtils.setTextAndHideIfEmpty(mAddress, item.mAddress);
    }
  }

  public static final class CianMoreItemViewHolder<T extends CianItem> extends BaseViewHolder<T>
  {

    public CianMoreItemViewHolder(@NonNull View itemView, @NonNull List<T> items,
                                  @NonNull GalleryAdapter adapter)
    {
      super(itemView, items, adapter);
    }

    @Override
    protected void onItemSelected(@NonNull T item)
    {
      GalleryAdapter.ItemSelectedListener listener = mAdapter.getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onMoreItemSelected(item.getUrl());
    }
  }

  public static class BaseViewHolder<I extends Items.Item> extends RecyclerView.ViewHolder
      implements View.OnClickListener
  {
    @NonNull
    TextView mTitle;
    @NonNull
    protected List<I> mItems;
    @NonNull
    GalleryAdapter mAdapter;

    protected BaseViewHolder(@NonNull View itemView, @NonNull List<I> items,
                             @NonNull GalleryAdapter adapter)
    {
      super(itemView);
      mTitle = (TextView) itemView.findViewById(R.id.tv__title);
      itemView.setOnClickListener(this);
      mItems = items;
      mAdapter = adapter;
    }

    @CallSuper
    public void bind(@NonNull I item)
    {
      mTitle.setText(item.getTitle());
    }

    @Override
    public void onClick(View v)
    {
      int position = getAdapterPosition();
      if (position == RecyclerView.NO_POSITION || mItems.isEmpty())
        return;

      onItemSelected(mItems.get(position));
    }

    protected void onItemSelected(@NonNull I item)
    {
    }
  }

  public static class LoadingViewHolder extends BaseViewHolder<Items.Item>
      implements View.OnClickListener
  {
    @NonNull
    ProgressBar mProgressBar;
    @NonNull
    TextView mSubtitle;
    @NonNull
    TextView mMore;

    LoadingViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items, @NonNull GalleryAdapter adapter)
    {
      super(itemView, items, adapter);
      mProgressBar = (ProgressBar) itemView.findViewById(R.id.pb__progress);
      mSubtitle = (TextView) itemView.findViewById(R.id.tv__subtitle);
      mMore = (TextView) itemView.findViewById(R.id.tv__more);
    }

    @CallSuper
    @Override
    public void bind(@NonNull Items.Item item)
    {
      super.bind(item);
      UiUtils.setTextAndHideIfEmpty(mSubtitle, item.getSubtitle());
    }

    @Override
    public void onClick(View v)
    {
      int position = getAdapterPosition();
      if (position == RecyclerView.NO_POSITION)
        return;

      onItemSelected(mItems.get(position));
    }

    @Override
    protected void onItemSelected(@NonNull Items.Item item)
    {
      if (mAdapter.getListener() == null || TextUtils.isEmpty(item.getUrl()))
        return;

      mAdapter.getListener().onItemSelected(item.getUrl());
    }
  }

  public static class ErrorViewHolder extends LoadingViewHolder
  {

    ErrorViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items, @NonNull GalleryAdapter adapter)
    {
      super(itemView, items, adapter);
      UiUtils.hide(mProgressBar);
    }
  }

  public static class OfflineViewHolder extends LoadingViewHolder
  {
    OfflineViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                      @NonNull GalleryAdapter adapter)
    {
      super(itemView, items, adapter);
      UiUtils.hide(mProgressBar);
    }

    @CallSuper
    @Override
    public void bind(@NonNull Items.Item item)
    {
      super.bind(item);
      UiUtils.setTextAndHideIfEmpty(mSubtitle, item.getSubtitle());
    }

    @Override
    protected void onItemSelected(@NonNull Items.Item item)
    {
      if (mAdapter.getListener() == null)
        return;

      // TODO: coming soon.
    }
  }
}
