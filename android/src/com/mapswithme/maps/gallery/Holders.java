package com.mapswithme.maps.gallery;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RectShape;
import android.net.Uri;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.CallSuper;
import androidx.annotation.ColorInt;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.graphics.drawable.RoundedBitmapDrawable;
import androidx.core.graphics.drawable.RoundedBitmapDrawableFactory;
import androidx.recyclerview.widget.RecyclerView;
import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.BitmapImageViewTarget;
import com.mapswithme.HotelUtils;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.guides.GuidesGallery;
import com.mapswithme.maps.promo.PromoCityGallery;
import com.mapswithme.maps.promo.PromoEntity;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.search.Popularity;
import com.mapswithme.maps.ugc.Impress;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.maps.widget.RatingView;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;
import java.util.Objects;

public class Holders
{
  public static class GenericMoreHolder<T extends RegularAdapterStrategy.Item>
      extends BaseViewHolder<T>
  {

    public GenericMoreHolder(@NonNull View itemView, @NonNull List<T> items,
                             @Nullable ItemSelectedListener<T> listener)
    {
      super(itemView, items, listener);
    }

    @Override
    protected void onItemSelected(@NonNull T item, int position)
    {
      ItemSelectedListener<T> listener = getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onMoreItemSelected(item);
    }
  }

  public static class SearchMoreHolder extends GenericMoreHolder<Items.SearchItem>
  {

    public SearchMoreHolder(@NonNull View itemView, @NonNull List<Items.SearchItem> items,
                            @Nullable ItemSelectedListener<Items.SearchItem> listener)
    {
      super(itemView, items, listener);
    }

    @Override
    protected void onItemSelected(@NonNull Items.SearchItem item, int position)
    {
      ItemSelectedListener<Items.SearchItem> listener = getListener();
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
                                 @Nullable ItemSelectedListener<Items.LocalExpertItem> listener)
    {
      super(itemView, items, listener);
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

    ActionButtonViewHolder(@NonNull View itemView, @NonNull List<T> items,
                           @Nullable ItemSelectedListener<T> listener)
    {
      super(itemView, items, listener);
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

      ItemSelectedListener<T> listener = getListener();
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
    @NonNull
    private final RatingView mNumberRating;
    @NonNull
    private final RatingView mPopularTagRating;

    public SearchViewHolder(@NonNull View itemView, @NonNull List<Items.SearchItem> items,
                            @Nullable ItemSelectedListener<Items.SearchItem> adapter)
    {
      super(itemView, items, adapter);
      mSubtitle = itemView.findViewById(R.id.subtitle);
      mDistance = itemView.findViewById(R.id.distance);
      mNumberRating = itemView.findViewById(R.id.counter_rating_view);
      mPopularTagRating = itemView.findViewById(R.id.popular_rating_view);
    }

    @Override
    public void bind(@NonNull Items.SearchItem item)
    {
      super.bind(item);

      String featureType = item.getFeatureType();
      Context context = mSubtitle.getContext();
      String localizedType = Utils.getLocalizedFeatureType(context, featureType);
      String title = TextUtils.isEmpty(item.getTitle(context)) ? localizedType : item.getTitle(context);

      UiUtils.setTextAndHideIfEmpty(getTitle(), title);
      UiUtils.setTextAndHideIfEmpty(mSubtitle, localizedType);
      UiUtils.setTextAndHideIfEmpty(mDistance, item.getDistance());
      UiUtils.showIf(item.getPopularity().getType() == Popularity.Type.POPULAR, mPopularTagRating);

      float rating = item.getRating();
      Impress impress = Impress.values()[UGC.nativeToImpress(rating)];
      mNumberRating.setRating(impress, UGC.nativeFormatRating(rating));
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

    public HotelViewHolder(@NonNull View itemView, @NonNull List<Items.SearchItem> items, @Nullable
        ItemSelectedListener<Items.SearchItem> listener)
    {
      super(itemView, items, listener);
      mTitle = itemView.findViewById(R.id.title);
      mSubtitle = itemView.findViewById(R.id.subtitle);
      mRatingView = itemView.findViewById(R.id.ratingView);
      mDistance = itemView.findViewById(R.id.distance);
    }

    @Override
    public void bind(@NonNull Items.SearchItem item)
    {
      String featureType = item.getFeatureType();
      Context context = mSubtitle.getContext();
      String localizedType = Utils.getLocalizedFeatureType(context, featureType);
      String title = TextUtils.isEmpty(item.getTitle(context)) ? localizedType : item.getTitle(context);

      UiUtils.setTextAndHideIfEmpty(mTitle, title);
      UiUtils.setTextAndHideIfEmpty(mSubtitle, formatDescription(item.getStars(),
                                                                 localizedType,
                                                                 item.getPrice(),
                                                                 mSubtitle.getResources()));

      float rating = item.getRating();
      Impress impress = Impress.values()[UGC.nativeToImpress(rating)];
      mRatingView.setRating(impress, UGC.nativeFormatRating(rating));
      UiUtils.setTextAndHideIfEmpty(mDistance, item.getDistance());
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
    private final TextView mTitle;
    @Nullable
    private final ItemSelectedListener<I> mListener;
    @NonNull
    protected final List<I> mItems;

    public BaseViewHolder(@NonNull View itemView, @NonNull List<I> items,
                          @Nullable ItemSelectedListener<I> listener)
    {
      super(itemView);
      mTitle = itemView.findViewById(R.id.title);
      mListener = listener;
      itemView.setOnClickListener(this);
      mItems = items;
    }

    public void bind(@NonNull I item)
    {
      mTitle.setText(item.getTitle(mTitle.getContext()));
    }

    @Override
    public void onClick(View v)
    {
      int position = getAdapterPosition();
      if (position == RecyclerView.NO_POSITION || mItems.isEmpty())
        return;

      onItemSelected(mItems.get(position), position);
    }

    @NonNull
    protected TextView getTitle()
    {
      return mTitle;
    }

    protected void onItemSelected(@NonNull I item, int position)
    {
      ItemSelectedListener<I> listener = getListener();
      if (listener == null || TextUtils.isEmpty(item.getUrl()))
        return;

      listener.onItemSelected(item, position);
    }

    @Nullable
    protected ItemSelectedListener<I> getListener()
    {
      return mListener;
    }
  }

  public static class LoadingViewHolder extends BaseViewHolder<Items.Item>
      implements View.OnClickListener
  {
    @NonNull
    final ProgressBar mProgressBar;
    @NonNull
    final TextView mSubtitle;

    LoadingViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                      @Nullable ItemSelectedListener<Items.Item> listener)
    {
      super(itemView, items, listener);
      mProgressBar = (ProgressBar) itemView.findViewById(R.id.pb__progress);
      mSubtitle = (TextView) itemView.findViewById(R.id.tv__subtitle);
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
      if (getListener() == null || TextUtils.isEmpty(item.getUrl()))
        return;

      getListener().onActionButtonSelected(item, position);
    }
  }

  public static class SimpleViewHolder extends BaseViewHolder<Items.Item>
  {
    public SimpleViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                            @Nullable ItemSelectedListener<Items.Item> listener)
    {
      super(itemView, items, listener);
    }
  }

  static class ErrorViewHolder extends LoadingViewHolder
  {

    ErrorViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                    @Nullable ItemSelectedListener<Items.Item> listener)
    {
      super(itemView, items, listener);
      UiUtils.hide(mProgressBar);
    }
  }

  public static class OfflineViewHolder extends LoadingViewHolder
  {
    OfflineViewHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                      @Nullable ItemSelectedListener<Items.Item> listener)
    {
      super(itemView, items, listener);
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

    }
  }


  public static class CatalogPromoHolder extends BaseViewHolder<PromoEntity>
  {
    @NonNull
    private final ImageView mImage;

    @NonNull
    private final TextView mSubTitle;

    @NonNull
    private final TextView mProLabel;

    public CatalogPromoHolder(@NonNull View itemView,
                              @NonNull List<PromoEntity> items,
                              @Nullable ItemSelectedListener<PromoEntity> listener)
    {
      super(itemView, items, listener);
      mImage = itemView.findViewById(R.id.image);
      mSubTitle = itemView.findViewById(R.id.subtitle);
      mProLabel = itemView.findViewById(R.id.label);
    }

    @Override
    public void bind(@NonNull PromoEntity item)
    {
      super.bind(item);

      bindProLabel(item);
      bindSubTitle(item);
      bindImage(item);
    }

    private void bindSubTitle(@NonNull PromoEntity item)
    {
      mSubTitle.setText(item.getSubtitle());
    }

    private void bindImage(@NonNull PromoEntity item)
    {
      Glide.with(itemView.getContext())
           .load(Uri.parse(item.getImageUrl()))
           .placeholder(R.drawable.img_guides_gallery_placeholder)
           .into(mImage);
    }

    private void bindProLabel(@NonNull PromoEntity item)
    {
      PromoCityGallery.LuxCategory category = item.getCategory();
      UiUtils.showIf(category != null && !TextUtils.isEmpty(category.getName()), mProLabel);
      if (item.getCategory() == null)
        return;

      mProLabel.setText(item.getCategory().getName());
      ShapeDrawable shapeDrawable = new ShapeDrawable(new RectShape());
      shapeDrawable.getPaint().setColor(item.getCategory().getColor());
      mProLabel.setBackgroundDrawable(shapeDrawable);
    }
  }

  public static class CrossPromoLoadingHolder extends SimpleViewHolder
  {
    @NonNull
    private final TextView mSubTitle;

    @NonNull
    private final TextView mButton;

    public CrossPromoLoadingHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                                   @Nullable ItemSelectedListener<Items.Item> listener)
    {
      super(itemView, items, listener);
      mSubTitle = itemView.findViewById(R.id.subtitle);
      mButton = itemView.findViewById(R.id.button);
    }

    @NonNull
    protected TextView getButton()
    {
      return mButton;
    }

    @Override
    public void bind(@NonNull Items.Item item)
    {
      super.bind(item);
      getTitle().setText(R.string.gallery_pp_download_guides_offline_title);
      mSubTitle.setText(R.string.gallery_pp_download_guides_offline_subtitle);
      UiUtils.invisible(getButton());
    }
  }

  public static class CatalogErrorHolder extends CrossPromoLoadingHolder
  {
    public CatalogErrorHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                              @Nullable ItemSelectedListener<Items.Item> listener)
    {
      super(itemView, items, listener);
      View progress = itemView.findViewById(R.id.progress);
      UiUtils.invisible(progress);
    }

    public void bind(@NonNull Items.Item item)
    {
      super.bind(item);
      getButton().setText(R.string.gallery_pp_download_guides_offline_cta);
      boolean isBtnInvisible = ConnectionState.INSTANCE.isConnected() &&
                               NetworkPolicy.newInstance(NetworkPolicy.getCurrentNetworkUsageStatus()).canUseNetwork();

      if (isBtnInvisible)
        UiUtils.invisible(getButton());
      else
        UiUtils.show(getButton());
    }

    @Override
    protected void onItemSelected(@NonNull Items.Item item, int position)
    {
      ItemSelectedListener<Items.Item> listener = getListener();
      if (listener == null)
        return;

      listener.onItemSelected(item, position);
    }
  }

  public static class GuideHodler extends BaseViewHolder<GuidesGallery.Item>
  {
    @NonNull
    private final ImageView mImage;
    @NonNull
    private final TextView mSubtitle;
    // TODO: handle city and outdoor content properly.
    @NonNull
    private final View mCityContent;
    @NonNull
    private final View mOutdoorContent;
    @NonNull
    private final View mItemView;
    @NonNull
    private final TextView mAltitide;
    @NonNull
    private final TextView mTime;
    @NonNull
    private final TextView mDistance;
    @NonNull
    private final TextView mDesc;
    @NonNull
    private final View mDivider;
    @NonNull
    private final View mBoughtContent;
    @NonNull
    private final TextView mBoughtContentBtn;
    @NonNull
    private final ImageView mBoughtContentCheckbox;

    public GuideHodler(@NonNull View itemView, @NonNull List<GuidesGallery.Item> items, @Nullable ItemSelectedListener<GuidesGallery.Item> listener)
    {
      super(itemView, items, listener);
      mItemView = itemView;
      mImage = mItemView.findViewById(R.id.image);
      mSubtitle = mItemView.findViewById(R.id.subtitle);
      mCityContent = mItemView.findViewById(R.id.city_content);
      mOutdoorContent = mItemView.findViewById(R.id.outdoor_content);
      mDistance = mOutdoorContent.findViewById(R.id.distance);
      mTime = mOutdoorContent.findViewById(R.id.time);
      mAltitide = mOutdoorContent.findViewById(R.id.altitude);
      mDesc = mCityContent.findViewById(R.id.description);
      mDivider = mItemView.findViewById(R.id.divider);
      mBoughtContent = mItemView.findViewById(R.id.bought_content);
      mBoughtContentBtn = mBoughtContent.findViewById(R.id.text);
      mBoughtContentBtn.setOnClickListener(v -> toggleBoughtContentBtnVisibility());
      mBoughtContentCheckbox = mItemView.findViewById(R.id.downloaded);
    }

    private void toggleBoughtContentBtnVisibility()
    {
      int index = getAdapterPosition();
      GuidesGallery.Item item = mItems.get(index);
      BookmarkCategory category =
          BookmarkManager.INSTANCE.getCategoryByServerId(item.getGuideId());
      boolean isVisible = category.isVisible();
      BookmarkManager.INSTANCE.setVisibility(category.getId(), !isVisible);
      Statistics.INSTANCE.trackBookmarksVisibility(Statistics.ParamValue.MAP_GALLERY,
                                                   isVisible ? Statistics.ParamValue.HIDE
                                                             : Statistics.ParamValue.SHOW,
                                                   item.getGuideId());
      mBoughtContentBtn.setText(!isVisible ? R.string.hide : R.string.show);
    }

    @Override
    public void bind(@NonNull GuidesGallery.Item item)
    {
      super.bind(item);
      bindImage(item);
      bindSubtitle(item);
      bindBottomBlock(item);
      bindActivationState(item);
    }

    private void bindBottomBlock(@NonNull GuidesGallery.Item item)
    {
      UiUtils.hideIf(item.isDownloaded(), mDivider, mOutdoorContent, mCityContent);
      UiUtils.showIf(item.isDownloaded(), mBoughtContent, mBoughtContentCheckbox);

      if (item.isDownloaded())
      {
        bindBoughtContentBlock(item);
        return;
      }

      boolean isCity = item.getGuideType() == GuidesGallery.Type.City;
      if (isCity)
        bindCityBlock(Objects.requireNonNull(item.getCityParams()));
      else
        bindOutdoorBlock(Objects.requireNonNull(item.getOutdoorParams()));
    }

    private void bindBoughtContentBlock(@NonNull GuidesGallery.Item item)
    {
      BookmarkCategory category =
          BookmarkManager.INSTANCE.getCategoryByServerId(item.getGuideId());
      mBoughtContentBtn.setText(category.isVisible() ? R.string.hide : R.string.show);
      boolean isCity = item.getGuideType() == GuidesGallery.Type.City;
      Context context = mBoughtContentBtn.getContext();
      @DrawableRes
      int drawableRes =
          ThemeUtils.getResource(context, isCity ? R.attr.cityCheckbox
                                                 : R.attr.outdoorCheckbox);
      mBoughtContentCheckbox.setImageResource(drawableRes);
    }

    private void bindOutdoorBlock(@NonNull GuidesGallery.OutdoorParams params)
    {
      UiUtils.show(mOutdoorContent);
      UiUtils.hide(mCityContent);

      Context context = mAltitide.getContext();

      mAltitide.setText(Framework.nativeFormatAltitude(params.getAscent()));
      mDistance.setText(StringUtils.nativeFormatDistance(params.getDistance()));
      mTime.setText(makeTime(context, params));
    }

    private void bindCityBlock(@NonNull GuidesGallery.CityParams cityParams)
    {
      UiUtils.show(mCityContent);
      UiUtils.hide(mOutdoorContent);

      Context context = mAltitide.getContext();
      String poiCount = String.valueOf(cityParams.getBookmarksCount());
      String text = context.getString(R.string.routes_card_number_of_points, poiCount);
      if (cityParams.isTrackAvailable())
        text += " " + context.getString(R.string.routes_card_plus_track);

      mDesc.setText(text);
    }

    private void bindSubtitle(@NonNull GuidesGallery.Item item)
    {
      Spannable subtitle = makeSubtitle(mItemView.getContext(), item);
      mSubtitle.setText(subtitle);
    }

    private void bindImage(@NonNull GuidesGallery.Item item)
    {
      Glide.with(mImage.getContext())
           .load(item.getImageUrl())
           .asBitmap()
           .placeholder(ThemeUtils.getResource(mImage.getContext(), R.attr.guidesPlaceholder))
           .centerCrop()
           .into(mImage);
      mSubtitle.setText(item.getSubtitle());
    }

    private void bindActivationState(@NonNull GuidesGallery.Item item)
    {
      boolean activated = mItemView.isActivated();
      if (activated != item.isActivated())
      {
        mItemView.post(() -> mItemView.setActivated(item.isActivated()));
      }
    }

    @NonNull
    private static CharSequence makeTime(@NonNull Context context,
                                         @NonNull GuidesGallery.OutdoorParams params)
    {
      return RoutingController.formatRoutingTime(context, (int) params.getDuration(),
                                                 R.dimen.text_size_body_4,
                                                 R.dimen.text_size_body_4);
    }

    @NonNull
    private static Spannable makeSubtitle(@NonNull Context context,
                                          @NonNull GuidesGallery.Item item)
    {
      boolean isCity = item.getGuideType() == GuidesGallery.Type.City;
      SpannableStringBuilder builder =
          new SpannableStringBuilder(isCity ? context.getString(R.string.routes_card_city)
                                            : context.getString(R.string.routes_card_outdoor));

      Resources res = context.getResources();

      @ColorInt
      int color = isCity ? ThemeUtils.getColor(context, R.attr.subtitleCityColor)
                         : ThemeUtils.getColor(context, R.attr.subtitleOutdoorColor);

      builder.setSpan(new ForegroundColorSpan(color), 0, builder.length(),
                      Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

      boolean isItinerary = isCity && Objects.requireNonNull(item.getCityParams())
                                             .isTrackAvailable();
      String text = res.getString(isItinerary ? R.string.routes_card_routes_tag : R.string.routes_card_set_tag);

      if (!isCity)
        text = TextUtils.isEmpty(Objects.requireNonNull(item.getOutdoorParams())
                                        .getString()) ? "" : item.getOutdoorParams()
                                                                 .getString();

      builder.append(TextUtils.isEmpty(text) ? text : UiUtils.WIDE_PHRASE_SEPARATOR + text);
      return builder;
    }
  }
}
