package com.mapswithme.maps.gallery;

import android.content.Context;
import android.graphics.Bitmap;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawableFactory;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.BitmapImageViewTarget;
import com.mapswithme.maps.R;
import com.mapswithme.maps.ugc.Impress;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.maps.widget.RatingView;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

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
                                   @NonNull GalleryAdapter<?, ViatorItem> adapter)
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
  }

  public static final class ViatorMoreItemViewHolder extends BaseViewHolder<Items.ViatorItem>
  {

    public ViatorMoreItemViewHolder(@NonNull View itemView, @NonNull List<Items.ViatorItem> items,
                                    @NonNull GalleryAdapter<?, Items.ViatorItem> adapter)
    {
      super(itemView, items, adapter);
    }

    @Override
    protected void onItemSelected(@NonNull Items.ViatorItem item)
    {
      ItemSelectedListener<ViatorItem> listener = mAdapter.getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onMoreItemSelected(item);
    }
  }

  public static final class CianProductViewHolder extends BaseViewHolder<CianItem>
  {
    @NonNull
    TextView mPrice;
    @NonNull
    TextView mAddress;

    public CianProductViewHolder(@NonNull View itemView, @NonNull List<CianItem> items,
                                 @NonNull GalleryAdapter<?, CianItem> adapter)
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
                                  @NonNull GalleryAdapter<?, T> adapter)
    {
      super(itemView, items, adapter);
    }

    @Override
    protected void onItemSelected(@NonNull T item)
    {
      ItemSelectedListener<T> listener = mAdapter.getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onMoreItemSelected(item);
    }
  }

  public static class LocalExpertViewHolder extends BaseViewHolder<Items.LocalExpertItem>
  {
    @NonNull
    private final ImageView mAvatar;
    @NonNull
    private final RatingView mRating;
    @NonNull
    private final TextView mButton;

    public LocalExpertViewHolder(@NonNull View itemView, @NonNull List<Items.LocalExpertItem> items,
                                 @NonNull GalleryAdapter<?, Items.LocalExpertItem> adapter)
    {
      super(itemView, items, adapter);
      mTitle = (TextView) itemView.findViewById(R.id.name);
      mAvatar = (ImageView) itemView.findViewById(R.id.avatar);
      mRating = (RatingView) itemView.findViewById(R.id.ratingView);
      mButton = (TextView) itemView.findViewById(R.id.button);
    }

    @Override
    public void bind(@NonNull Items.LocalExpertItem item)
    {
      super.bind(item);

      Glide.with(mAvatar.getContext())
           .load(item.getPhotoUrl())
           .asBitmap()
           .centerCrop()
           .placeholder(R.drawable.ic_local_expert_default)
           .into(new BitmapImageViewTarget(mAvatar)
           {
             @Override
             protected void setResource(Bitmap resource)
             {
               RoundedBitmapDrawable circularBitmapDrawable =
                   RoundedBitmapDrawableFactory.create(mAvatar.getContext().getResources(),
                                                       resource);
               circularBitmapDrawable.setCircular(true);
               mAvatar.setImageDrawable(circularBitmapDrawable);
             }
           });

      Context context = mButton.getContext();
      String priceLabel;
      if (item.getPrice() == 0 && TextUtils.isEmpty(item.getCurrency()))
      {
        priceLabel = context.getString(R.string.free);
      }
      else
      {
        String formattedPrice = Utils.formatCurrencyString(String.valueOf(item.getPrice()),
                                                           item.getCurrency());
        priceLabel = context.getString(R.string.price_per_hour, formattedPrice);
      }
      UiUtils.setTextAndHideIfEmpty(mButton, priceLabel);
      float rating = (float) item.getRating();
      Impress impress = Impress.values()[UGC.nativeToImpress(rating)];
      mRating.setRating(impress, UGC.nativeFormatRating(rating));
    }
  }

  public static class LocalExpertMoreItemViewHolder extends BaseViewHolder<Items.LocalExpertItem>
  {
    public LocalExpertMoreItemViewHolder(@NonNull View itemView,
                                         @NonNull List<Items.LocalExpertItem> items,
                                         @NonNull GalleryAdapter<?, Items.LocalExpertItem> adapter)
    {
      super(itemView, items, adapter);
    }

    @Override
    protected void onItemSelected(@NonNull Items.LocalExpertItem item)
    {
      ItemSelectedListener<Items.LocalExpertItem> listener = mAdapter.getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onMoreItemSelected(item);
    }
  }

  public static final class SearchViewHolder extends BaseViewHolder<Items.SearchItem>
  {
    @NonNull
    private final TextView mSubtitle;
    @NonNull
    private final TextView mDistance;
    @NonNull
    private final TextView mButton;

    public SearchViewHolder(@NonNull View itemView, @NonNull List<Items.SearchItem> items,
                            @NonNull GalleryAdapter<?, Items.SearchItem> adapter)
    {
      super(itemView, items, adapter);
      mTitle = (TextView) itemView.findViewById(R.id.title);
      mSubtitle = (TextView) itemView.findViewById(R.id.subtitle);
      mDistance = (TextView) itemView.findViewById(R.id.distance);
      mButton = (TextView) itemView.findViewById(R.id.button);
      mButton.setOnClickListener(this);
      itemView.findViewById(R.id.infoLayout).setOnClickListener(this);
      mButton.setText(R.string.p2p_to_here);
    }

    @Override
    public void bind(@NonNull Items.SearchItem item)
    {
      super.bind(item);
      UiUtils.setTextAndHideIfEmpty(mTitle, item.getTitle());
      UiUtils.setTextAndHideIfEmpty(mSubtitle, item.getSubtitle());
      UiUtils.setTextAndHideIfEmpty(mDistance, item.getDistance());
    }

    @Override
    public void onClick(View v)
    {
      int position = getAdapterPosition();
      if (position == RecyclerView.NO_POSITION || mItems.isEmpty())
        return;

      ItemSelectedListener<Items.SearchItem> listener = mAdapter.getListener();
      if (listener == null)
        return;

      Items.SearchItem item = mItems.get(position);
      switch (v.getId())
      {
        case R.id.infoLayout:
          listener.onItemSelected(item);
          break;
        case R.id.button:
          listener.onActionButtonSelected(item);
          break;
      }
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
    GalleryAdapter<?, I> mAdapter;

    BaseViewHolder(@NonNull View itemView, @NonNull List<I> items,
                             @NonNull GalleryAdapter<?, I> adapter)
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
      ItemSelectedListener<I> listener = mAdapter.getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onItemSelected(item);
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

    LoadingViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                      @NonNull GalleryAdapter<?, Items.Item> adapter)
    {
      super(itemView, items, adapter);
      mProgressBar = (ProgressBar) itemView.findViewById(R.id.pb__progress);
      mSubtitle = (TextView) itemView.findViewById(R.id.tv__subtitle);
      mMore = (TextView) itemView.findViewById(R.id.button);
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

      mAdapter.getListener().onActionButtonSelected(item);
    }
  }

  public static class SimpleViewHolder extends BaseViewHolder<Items.Item>
  {
    public SimpleViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                            @NonNull GalleryAdapter<?, Items.Item> adapter)
    {
      super(itemView, items, adapter);
      mTitle = (TextView) itemView.findViewById(R.id.message);
    }
  }

  static class ErrorViewHolder extends LoadingViewHolder
  {

    ErrorViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                    @NonNull GalleryAdapter<?, Items.Item> adapter)
    {
      super(itemView, items, adapter);
      UiUtils.hide(mProgressBar);
    }
  }

  public static class OfflineViewHolder extends LoadingViewHolder
  {
    OfflineViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                      @NonNull GalleryAdapter<?, Items.Item> adapter)
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

      mAdapter.getListener().onActionButtonSelected(item);
    }
  }
}
