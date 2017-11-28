package com.mapswithme.maps.viator;

import android.content.Context;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseSponsoredAdapter;
import com.mapswithme.maps.ugc.Impress;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.maps.widget.RatingView;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.List;

public final class ViatorAdapter extends BaseSponsoredAdapter
{
  private static final String LOADING_TITLE = MwmApplication
      .get().getString(R.string.preloader_viator_title);
  private static final String LOADING_SUBTITLE = MwmApplication
      .get().getString(R.string.preloader_viator_message);
  public ViatorAdapter(@Nullable String url, boolean hasError,
                       @Nullable ItemSelectedListener listener)
  {
    super(url, hasError, listener);
  }

  public ViatorAdapter(@NonNull ViatorProduct[] items, @Nullable String cityUrl,
                       @Nullable ItemSelectedListener listener, boolean shouldShowMoreItem)
  {
    super(convertItems(items), cityUrl, listener, shouldShowMoreItem);
  }

  @NonNull
  private static List<Item> convertItems(@NonNull ViatorProduct[] items)
  {
    List<Item> viewItems = new ArrayList<>();
    for (ViatorProduct product : items)
    {
      viewItems.add(new Item(product.getPhotoUrl(), product.getTitle(),
                             product.getDuration(), product.getRating(), product.getPriceFormatted(),
                             product.getPageUrl()));
    }

    return viewItems;
  }

  @Override
  @NonNull
  protected BaseSponsoredAdapter.ViewHolder createViewHolder(@NonNull LayoutInflater inflater,
                                                             @NonNull ViewGroup parent)
  {
    return new ProductViewHolder(inflater.inflate(R.layout.item_viator_product, parent, false),
                                 this);
  }

  @NonNull
  @Override
  protected View inflateLoadingView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.item_viator_loading, parent, false);
  }

  @Override
  protected int getMoreLabelForLoadingView()
  {
    return R.string.preloader_viator_button;
  }

  @NonNull
  @Override
  protected String getLoadingTitle()
  {
    return LOADING_TITLE;
  }

  @Nullable
  @Override
  protected String getLoadingSubtitle()
  {
    return LOADING_SUBTITLE;
  }

  @LayoutRes
  @Override
  protected int getMoreLayout()
  {
    return R.layout.item_viator_more;
  }

  private static final class ProductViewHolder extends ViewHolder
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

    ProductViewHolder(@NonNull View itemView, @NonNull ViatorAdapter adapter)
    {
      super(itemView, adapter);
      mContext = itemView.getContext();
      mImage = (ImageView) itemView.findViewById(R.id.iv__image);
      mDuration = (TextView) itemView.findViewById(R.id.tv__duration);
      mRating = (RatingView) itemView.findViewById(R.id.ratingView);
      mPrice = (TextView) itemView.findViewById(R.id.tv__price);
    }

    @Override
    public void bind(@NonNull BaseSponsoredAdapter.Item item)
    {
      super.bind(item);

      Item product = (Item) item;
      if (product.mPhotoUrl != null)
      {
        Glide.with(mContext)
             .load(product.mPhotoUrl)
             .centerCrop()
             .into(mImage);
      }

      UiUtils.setTextAndHideIfEmpty(mDuration, product.mDuration);
      UiUtils.setTextAndHideIfEmpty(mPrice, mContext.getString(R.string.place_page_starting_from,
                                                               product.mPrice));
      float rating = (float) product.mRating;
      Impress impress = Impress.values()[UGC.nativeToImpress(rating)];
      mRating.setRating(impress, String.valueOf(rating));
    }
  }

  private static final class Item extends BaseSponsoredAdapter.Item
  {
    @Nullable
    private final String mPhotoUrl;
    @Nullable
    private final String mDuration;
    private final double mRating;
    @Nullable
    private final String mPrice;

    private Item(@Nullable String photoUrl, @NonNull String title,
                 @Nullable String duration, double rating, @Nullable String price,
                 @NonNull String url)
    {
      super(TYPE_PRODUCT, title, url, null, false, false);
      mPhotoUrl = photoUrl;
      mDuration = duration;
      mRating = rating;
      mPrice = price;
    }
  }
}
