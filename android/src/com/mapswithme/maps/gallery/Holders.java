package com.mapswithme.maps.gallery;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawableFactory;
import android.support.v7.widget.RecyclerView;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.BitmapImageViewTarget;
import com.mapswithme.HotelUtils;
import com.mapswithme.maps.R;
import com.mapswithme.maps.ugc.Impress;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.maps.widget.RatingView;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import java.util.List;

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

  public static class GenericMoreHolder<T extends RegularAdapterStrategy.Item>
      extends BaseViewHolder<T>
  {

    public GenericMoreHolder(@NonNull View itemView, @NonNull List<T> items, @NonNull GalleryAdapter<?, T>
        adapter)
    {
      super(itemView, items, adapter);
    }

    @Override
    protected void onItemSelected(@NonNull T item, int position)
    {
      ItemSelectedListener<T> listener = mAdapter.getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onMoreItemSelected(item);
    }
  }

  public static class SearchMoreHolder extends GenericMoreHolder<Items.SearchItem>
  {

    public SearchMoreHolder(@NonNull View itemView, @NonNull List<Items.SearchItem> items, @NonNull
        GalleryAdapter<?, Items.SearchItem> adapter)
    {
      super(itemView, items, adapter);
    }

    @Override
    protected void onItemSelected(@NonNull Items.SearchItem item, int position)
    {
      ItemSelectedListener<Items.SearchItem> listener = mAdapter.getListener();
      if (listener != null)
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

  public static abstract class ActionButtonViewHolder<T extends RegularAdapterStrategy.Item>
      extends BaseViewHolder<T>
  {
    @NonNull
    private final TextView mButton;

    ActionButtonViewHolder(@NonNull View itemView, @NonNull List<T> items, @NonNull
        GalleryAdapter<?, T> adapter)
    {
      super(itemView, items, adapter);
      mButton = itemView.findViewById(R.id.button);
      mButton.setOnClickListener(this);
      itemView.findViewById(R.id.infoLayout).setOnClickListener(this);
      mButton.setText(R.string.p2p_to_here);
    }

    @Override
    public void onClick(View v)
    {
      int position = getAdapterPosition();
      if (position == RecyclerView.NO_POSITION || mItems.isEmpty())
        return;

      ItemSelectedListener<T> listener = mAdapter.getListener();
      if (listener == null)
        return;

      T item = mItems.get(position);
      switch (v.getId())
      {
        case R.id.infoLayout:
          listener.onItemSelected(item, position);
          break;
        case R.id.button:
          listener.onActionButtonSelected(item, position);
          break;
      }
    }
  }

  public static final class SearchViewHolder extends ActionButtonViewHolder<Items.SearchItem>
  {
    @NonNull
    private final TextView mSubtitle;
    @NonNull
    private final TextView mDistance;

    public SearchViewHolder(@NonNull View itemView, @NonNull List<Items.SearchItem> items,
                            @NonNull GalleryAdapter<?, Items.SearchItem> adapter)
    {
      super(itemView, items, adapter);
      mTitle = itemView.findViewById(R.id.title);
      mSubtitle = itemView.findViewById(R.id.subtitle);
      mDistance = itemView.findViewById(R.id.distance);
    }

    @Override
    public void bind(@NonNull Items.SearchItem item)
    {
      super.bind(item);
      UiUtils.setTextAndHideIfEmpty(mTitle, item.getTitle());
      UiUtils.setTextAndHideIfEmpty(mSubtitle, item.getSubtitle());
      UiUtils.setTextAndHideIfEmpty(mDistance, item.getDistance());
    }
  }

  public static final class HotelViewHolder extends ActionButtonViewHolder<Items.SearchItem>
  {
    @NonNull
    private final TextView mTitle;
    @NonNull
    private final TextView mSubtitle;
    @NonNull
    private final RatingView mRatingView;
    @NonNull
    private final TextView mDistance;

    public HotelViewHolder(@NonNull View itemView, @NonNull List<Items.SearchItem> items, @NonNull
        GalleryAdapter<?, Items.SearchItem> adapter)
    {
      super(itemView, items, adapter);
      mTitle = itemView.findViewById(R.id.title);
      mSubtitle = itemView.findViewById(R.id.subtitle);
      mRatingView = itemView.findViewById(R.id.ratingView);
      mDistance = itemView.findViewById(R.id.distance);
    }

    @Override
    public void bind(@NonNull Items.SearchItem item)
    {
      UiUtils.setTextAndHideIfEmpty(mTitle, item.getTitle());
      UiUtils.setTextAndHideIfEmpty(mSubtitle, formatDescription(item.getStars(),
                                                                 item.getFeatureType(),
                                                                 item.getPrice(),
                                                                 mSubtitle.getResources()));

      float rating = formatRating(item.getRating());
      Impress impress = Impress.values()[UGC.nativeToImpress(rating)];
      mRatingView.setRating(impress, UGC.nativeFormatRating(rating));
      UiUtils.setTextAndHideIfEmpty(mDistance, item.getDistance());
    }

    //TODO: refactor rating to float in core
    private static float formatRating(@Nullable String rating)
    {
      if (TextUtils.isEmpty(rating))
        return -1;

      try
      {
        return Float.valueOf(rating);
      }
      catch (NumberFormatException e)
      {
        return -1;
      }
    }

    @NonNull
    private static CharSequence formatDescription(int stars, @Nullable String type,
                                                  @Nullable String priceCategory,
                                                  @NonNull Resources res)
    {
      final SpannableStringBuilder sb = new SpannableStringBuilder();
      if (stars > 0)
        sb.append(HotelUtils.formatStars(stars, res));
      else if (!TextUtils.isEmpty(type))
        sb.append(type);

      if (!TextUtils.isEmpty(priceCategory))
      {
        sb.append(" • ");
        sb.append(priceCategory);
      }

      return sb;
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
      mTitle = itemView.findViewById(R.id.tv__title);
      itemView.setOnClickListener(this);
      mItems = items;
      mAdapter = adapter;
    }

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

      onItemSelected(mItems.get(position), position);
    }

    protected void onItemSelected(@NonNull I item, int position)
    {
      ItemSelectedListener<I> listener = mAdapter.getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onItemSelected(item, position);
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

      onItemSelected(mItems.get(position), position);
    }

    @Override
    protected void onItemSelected(@NonNull Items.Item item, int position)
    {
      if (mAdapter.getListener() == null || TextUtils.isEmpty(item.getUrl()))
        return;

      mAdapter.getListener().onActionButtonSelected(item, position);
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
    protected void onItemSelected(@NonNull Items.Item item, int position)
    {
      if (mAdapter.getListener() == null)
        return;

      mAdapter.getListener().onActionButtonSelected(item, position);
    }
  }
}
