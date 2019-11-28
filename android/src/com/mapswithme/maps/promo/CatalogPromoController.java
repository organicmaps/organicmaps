package com.mapswithme.maps.promo;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.Resources;
import android.text.Html;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.bumptech.glide.Glide;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.impl.Factory;
import com.mapswithme.maps.gallery.impl.RegularCatalogPromoListener;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.UTM;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Destination;
import com.mapswithme.util.statistics.GalleryPlacement;
import com.mapswithme.util.statistics.GalleryState;
import com.mapswithme.util.statistics.GalleryType;
import com.mapswithme.util.statistics.Statistics;

public class CatalogPromoController implements Promo.Listener, Detachable<Activity>
{
  @Nullable
  private Activity mActivity;
  @NonNull
  private RecyclerView mRecycler;
  @NonNull
  private TextView mTitle;
  @NonNull
  private final PlacePageView mPlacePageView;
  @Nullable
  private PromoRequester mRequester;

  public CatalogPromoController(@NonNull PlacePageView placePageView)
  {
    mPlacePageView = placePageView;
    mRecycler = mPlacePageView.findViewById(R.id.catalog_promo_recycler);
    mTitle = mPlacePageView.findViewById(R.id.catalog_promo_title);
    mRecycler.setNestedScrollingEnabled(false);
    LinearLayoutManager layoutManager = new LinearLayoutManager(mPlacePageView.getContext(),
                                                                LinearLayoutManager.HORIZONTAL,
                                                                false);
    mRecycler.setLayoutManager(layoutManager);
    RecyclerView.ItemDecoration decor =
        ItemDecoratorFactory.createPlacePagePromoGalleryDecorator(mPlacePageView.getContext(),
                                                                  LinearLayoutManager.HORIZONTAL);
    mRecycler.addItemDecoration(decor);
  }

  @Override
  public void onCityGalleryReceived(@NonNull PromoCityGallery promo)
  {
    Sponsored sponsored = mPlacePageView.getSponsored();
    if (sponsored == null)
      return;

    PromoResponseHandler handler = createPromoResponseHandler(sponsored.getType(), promo);
    if (handler == null)
      return;

    handler.handleResponse(promo);
  }

  @Override
  public void onErrorReceived()
  {
    if (mRequester == null)
      return;

    GalleryPlacement placement = getGalleryPlacement(mRequester.getSponsoredType());

    Statistics.INSTANCE.trackGalleryError(GalleryType.PROMO, placement,
                                          Statistics.ParamValue.NO_PRODUCTS);
  }

  @SuppressLint("SwitchIntDef")
  @NonNull
  private static GalleryPlacement getGalleryPlacement(@Sponsored.SponsoredType int type)
  {
    switch (type)
    {
      case Sponsored.TYPE_PROMO_CATALOG_CITY:
        return GalleryPlacement.PLACEPAGE_LARGE_TOPONYMS;
      case Sponsored.TYPE_PROMO_CATALOG_SIGHTSEEINGS:
        return GalleryPlacement.PLACEPAGE_SIGHTSEEINGS;
      case Sponsored.TYPE_PROMO_CATALOG_OUTDOOR:
        return GalleryPlacement.PLACEPAGE_OUTDOOR;
      default:
        throw new AssertionError("Unsupported catalog gallery type '" + type + "'!");
    }
  }

  public void updateCatalogPromo(@NonNull NetworkPolicy policy, @Nullable MapObject mapObject)
  {
    UiUtils.hide(mPlacePageView, R.id.catalog_promo_container);

    if (MapObject.isOfType(MapObject.BOOKMARK, mapObject))
      return;

    Sponsored sponsored = mPlacePageView.getSponsored();
    if (sponsored == null || mapObject == null)
      return;

    mRequester = createPromoRequester(sponsored.getType());
    if (mRequester == null)
      return;

    mRequester.requestPromo(policy, mapObject);
  }

  @Override
  public void attach(@NonNull Activity object)
  {
    mActivity = object;
    Promo.INSTANCE.setListener(this);
  }

  @Override
  public void detach()
  {
    mActivity = null;
    Promo.INSTANCE.setListener(null);
  }

  @SuppressLint("SwitchIntDef")
  @Nullable
  private static PromoRequester createPromoRequester(@Sponsored.SponsoredType int type)
  {
    switch (type)
    {
      case Sponsored.TYPE_PROMO_CATALOG_SIGHTSEEINGS:
        return new SightseeingsPromoRequester();
      case Sponsored.TYPE_PROMO_CATALOG_CITY:
        return new CityPromoRequester();
      case Sponsored.TYPE_PROMO_CATALOG_OUTDOOR:
        return new OutdoorPromoRequester();
      default:
        return null;
    }
  }

  @Nullable
  private PromoResponseHandler createPromoResponseHandler(@Sponsored.SponsoredType int type,
                                                          @NonNull PromoCityGallery promo)
  {
    if (type != Sponsored.TYPE_PROMO_CATALOG_CITY
        && type != Sponsored.TYPE_PROMO_CATALOG_SIGHTSEEINGS
        && type != Sponsored.TYPE_PROMO_CATALOG_OUTDOOR)
      return null;

    PromoCityGallery.Item[] items = promo.getItems();
    if (items.length <= 0)
      return null;

    if (items.length == 1)
      return new SinglePromoResponseHandler(type);

    return new GalleryPromoResponseHandler(type);
  }

  @NonNull
  private Activity requireActivity()
  {
    if (mActivity == null)
      throw new AssertionError("Activity must be non-null at this point!");

    return mActivity;
  }

  interface PromoRequester
  {
    void requestPromo(@NonNull NetworkPolicy policy, @NonNull MapObject mapObject);
    @Sponsored.SponsoredType
    int getSponsoredType();
  }

  static class SightseeingsPromoRequester implements PromoRequester
  {
    @Override
    public void requestPromo(@NonNull NetworkPolicy policy, @NonNull MapObject mapObject)
    {
      Promo.INSTANCE.nativeRequestPoiGallery(policy, mapObject.getLat(), mapObject.getLon(),
                                             mapObject.getRawTypes(), UTM.UTM_SIGHTSEEINGS_PLACEPAGE_GALLERY);
    }

    @Override
    public int getSponsoredType()
    {
      return Sponsored.TYPE_PROMO_CATALOG_SIGHTSEEINGS;
    }
  }

  static class OutdoorPromoRequester implements PromoRequester
  {
    @Override
    public void requestPromo(@NonNull NetworkPolicy policy, @NonNull MapObject mapObject)
    {
      Promo.INSTANCE.nativeRequestPoiGallery(policy, mapObject.getLat(), mapObject.getLon(),
                                             mapObject.getRawTypes(), UTM.UTM_OUTDOOR_PLACEPAGE_GALLERY);
    }

    @Override
    public int getSponsoredType()
    {
      return Sponsored.TYPE_PROMO_CATALOG_OUTDOOR;
    }
  }

  static class CityPromoRequester implements PromoRequester
  {
    @Override
    public void requestPromo(@NonNull NetworkPolicy policy, @NonNull MapObject mapObject)
    {
      Promo.INSTANCE.nativeRequestCityGallery(policy, mapObject.getLat(), mapObject.getLon(),
                                              UTM.UTM_LARGE_TOPONYMS_PLACEPAGE_GALLERY);
    }

    @Override
    public int getSponsoredType()
    {
      return Sponsored.TYPE_PROMO_CATALOG_CITY;
    }
  }

  interface PromoResponseHandler
  {
    void handleResponse(@NonNull PromoCityGallery promo);
  }

  class SinglePromoResponseHandler implements PromoResponseHandler
  {
    @Sponsored.SponsoredType
    private final int mSponsoredType;

    SinglePromoResponseHandler(int sponsoredType)
    {
      mSponsoredType = sponsoredType;
    }

    @Override
    public void handleResponse(@NonNull PromoCityGallery promo)
    {
      PromoCityGallery.Item[] items = promo.getItems();
      if (items.length <= 0)
        return;

      UiUtils.show(mPlacePageView, R.id.catalog_promo_container,
                   R.id.promo_poi_description_container, R.id.promo_poi_description_divider,
                   R.id.promo_poi_card);
      UiUtils.hide(mRecycler);
      mTitle.setText(R.string.pp_discovery_place_related_header);

      final PromoCityGallery.Item item = items[0];

      ImageView poiImage = mPlacePageView.findViewById(R.id.promo_poi_image);
      Glide.with(poiImage.getContext())
           .load(item.getImageUrl())
           .centerCrop()
           .placeholder(R.drawable.img_guidespp_placeholder)
           .into(poiImage);
      TextView bookmarkName = mPlacePageView.findViewById(R.id.place_single_bookmark_name);
      bookmarkName.setText(item.getName());
      TextView subtitle = mPlacePageView.findViewById(R.id.place_single_bookmark_subtitle);
      subtitle.setText(TextUtils.isEmpty(item.getTourCategory()) ? item.getAuthor().getName()
                                                                 : item.getTourCategory());
      View cta = mPlacePageView.findViewById(R.id.place_single_bookmark_cta);
      final GalleryPlacement placement = getGalleryPlacement(mSponsoredType);
      cta.setOnClickListener(v -> onCtaClicked(placement, item.getUrl()));

      PromoCityGallery.Place place = item.getPlace();

      UiUtils.hide(mPlacePageView, R.id.poi_description_container);
      TextView poiName = mPlacePageView.findViewById(R.id.promo_poi_name);
      poiName.setText(place.getName());
      TextView poiDescription = mPlacePageView.findViewById(R.id.promo_poi_description);
      poiDescription.setText(Html.fromHtml(place.getDescription()));
      View more = mPlacePageView.findViewById(R.id.promo_poi_more);
      more.setOnClickListener(v -> onMoreDescriptionClicked(item.getUrl()));

      Statistics.INSTANCE.trackGalleryShown(GalleryType.PROMO, GalleryState.ONLINE, placement, 1);
    }

    private void onCtaClicked(@NonNull GalleryPlacement placement, @NonNull String url)
    {
      String utmContentUrl = BookmarkManager.INSTANCE.injectCatalogUTMContent(url,
                                                                              UTM.UTM_CONTENT_VIEW);
      BookmarksCatalogActivity.startForResult(requireActivity(),
                                              BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                              utmContentUrl);
      Statistics.INSTANCE.trackGalleryProductItemSelected(GalleryType.PROMO, placement, 0,
                                                          Destination.CATALOGUE);
    }

    private void onMoreDescriptionClicked(@NonNull String url)
    {
      String utmContentUrl = BookmarkManager.INSTANCE.injectCatalogUTMContent(url,
                                                                         UTM.UTM_CONTENT_DESCRIPTION);
      BookmarksCatalogActivity.startForResult(requireActivity(),
                                              BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                              utmContentUrl);
    }
  }

  class GalleryPromoResponseHandler implements PromoResponseHandler
  {
    @Sponsored.SponsoredType
    private final int mSponsoredType;

    GalleryPromoResponseHandler(int sponsoredType)
    {
      mSponsoredType = sponsoredType;
    }

    @Override
    public void handleResponse(@NonNull PromoCityGallery promo)
    {
      UiUtils.show(mPlacePageView, R.id.catalog_promo_container,
                   R.id.catalog_promo_title_divider, R.id.catalog_promo_recycler);
      UiUtils.hide(mPlacePageView, R.id.promo_poi_description_container,
                   R.id.promo_poi_description_divider, R.id.promo_poi_card);

      Resources resources = mPlacePageView.getResources();
      String category = promo.getCategory();
      boolean showCategoryHeader = !TextUtils.isEmpty(category)
                                   && (mSponsoredType == Sponsored.TYPE_PROMO_CATALOG_SIGHTSEEINGS
                                       || mSponsoredType == Sponsored.TYPE_PROMO_CATALOG_OUTDOOR);
      String galleryHeader;
      if (showCategoryHeader)
      {
        galleryHeader = resources.getString(R.string.pp_discovery_place_related_tag_header,
                                    promo.getCategory());
      }
      else
      {
        galleryHeader = resources.getString(R.string.guides);
      }
      mTitle.setText(galleryHeader);

      String url = promo.getMoreUrl();
      GalleryPlacement placement = getGalleryPlacement(mSponsoredType);
      RegularCatalogPromoListener promoListener = new RegularCatalogPromoListener(requireActivity(),
                                                                                  placement);
      GalleryAdapter adapter = Factory.createCatalogPromoAdapter(requireActivity(), promo, url,
                                                                 promoListener, placement);
      mRecycler.setAdapter(adapter);
    }
  }
}
