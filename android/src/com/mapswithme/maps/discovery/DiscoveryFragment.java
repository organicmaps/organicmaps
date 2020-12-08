package com.mapswithme.maps.discovery;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.os.Bundle;
import androidx.annotation.CallSuper;
import androidx.annotation.IdRes;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.gallery.impl.Factory;
import com.mapswithme.maps.gallery.impl.LoggableItemSelectedListener;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.maps.promo.PromoCityGallery;
import com.mapswithme.maps.promo.PromoEntity;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.widget.PlaceholderView;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.maps.widget.placepage.ErrorCatalogPromoListener;
import com.mapswithme.maps.gallery.impl.RegularCatalogPromoListener;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Language;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.GalleryType;
import com.mapswithme.util.statistics.Statistics;

import static com.mapswithme.util.statistics.Destination.EXTERNAL;
import static com.mapswithme.util.statistics.Destination.PLACEPAGE;
import static com.mapswithme.util.statistics.Destination.ROUTING;
import static com.mapswithme.util.statistics.GalleryPlacement.DISCOVERY;
import static com.mapswithme.util.statistics.GalleryType.LOCAL_EXPERTS;
import static com.mapswithme.util.statistics.GalleryType.PROMO;
import static com.mapswithme.util.statistics.GalleryType.SEARCH_ATTRACTIONS;
import static com.mapswithme.util.statistics.GalleryType.SEARCH_HOTELS;
import static com.mapswithme.util.statistics.GalleryType.SEARCH_RESTAURANTS;

public class DiscoveryFragment extends BaseMwmToolbarFragment implements DiscoveryResultReceiver
{
  private static final int ITEMS_COUNT = 5;
  private static final int[] ITEM_TYPES = { DiscoveryParams.ITEM_TYPE_HOTELS,
                                            DiscoveryParams.ITEM_TYPE_ATTRACTIONS,
                                            DiscoveryParams.ITEM_TYPE_CAFES,
                                            DiscoveryParams.ITEM_TYPE_PROMO};
  private boolean mOnlineMode;
  @Nullable
  private CustomNavigateUpListener mNavigateUpListener;
  @Nullable
  private DiscoveryListener mDiscoveryListener;
  @NonNull
  private final BroadcastReceiver mNetworkStateReceiver = new BroadcastReceiver()
  {
    @Override
    public void onReceive(Context context, Intent intent)
    {
      if (mOnlineMode)
        return;

      if (ConnectionState.INSTANCE.isConnected())
        NetworkPolicy.checkNetworkPolicy(getFragmentManager(), DiscoveryFragment.this::onNetworkPolicyResult);
    }
  };

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);

    if (context instanceof CustomNavigateUpListener)
      mNavigateUpListener = (CustomNavigateUpListener) context;

    if (context instanceof DiscoveryListener)
      mDiscoveryListener = (DiscoveryListener) context;
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    mNavigateUpListener = null;
    mDiscoveryListener = null;
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_discovery, container, false);
    UserActionsLogger.logDiscoveryShownEvent();
    return root;
  }

  private void initHotelGallery()
  {
    setLayoutManagerAndItemDecoration(getContext(), getGallery(R.id.hotels));
  }

  private void initAttractionsGallery()
  {
    setLayoutManagerAndItemDecoration(getContext(), getGallery(R.id.attractions));
  }

  private void initFoodGallery()
  {
    setLayoutManagerAndItemDecoration(getContext(), getGallery(R.id.food));
  }

  private void initCatalogPromoGallery()
  {
    RecyclerView catalogPromoRecycler = getGallery(R.id.catalog_promo_recycler);
    setLayoutManagerAndItemDecoration(requireContext(), catalogPromoRecycler);
  }

  private static void setLayoutManagerAndItemDecoration(@NonNull Context context,
                                                        @NonNull RecyclerView rv)
  {
    rv.setLayoutManager(new LinearLayoutManager(context, LinearLayoutManager.HORIZONTAL,
                                                false));
    rv.addItemDecoration(
        ItemDecoratorFactory.createSponsoredGalleryDecorator(context,
                                                             LinearLayoutManager.HORIZONTAL));
  }

  @NonNull
  private RecyclerView getGallery(@IdRes int id)
  {
    View view = getView();
    if (view == null)
      throw new AssertionError("Root view is not initialized yet!");

    RecyclerView rv = view.findViewById(id);
    if (rv == null)
      throw new AssertionError("RecyclerView must be within root view!");
    return rv;
  }

  @Override
  public void onStart()
  {
    super.onStart();
    IntentFilter filter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
    getActivity().registerReceiver(mNetworkStateReceiver, filter);
    DiscoveryManager.INSTANCE.attach(this);
  }

  @Override
  public void onStop()
  {
    getActivity().unregisterReceiver(mNetworkStateReceiver);
    DiscoveryManager.INSTANCE.detach();
    super.onStop();
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.discovery_button_title);
    initHotelGallery();
    initAttractionsGallery();
    initFoodGallery();
    initSearchBasedAdapters();
    initCatalogPromoGallery();
    NetworkPolicy.checkNetworkPolicy(getFragmentManager(), this::onNetworkPolicyResult);
  }

  private void onNetworkPolicyResult(@NonNull NetworkPolicy policy)
  {
    mOnlineMode = policy.canUseNetwork();
    initNetworkBasedAdapters();
    requestDiscoveryInfo();
  }

  private void initSearchBasedAdapters()
  {
    Context context = requireContext();
    getGallery(R.id.hotels).setAdapter(Factory.createSearchBasedLoadingAdapter(context));
    getGallery(R.id.attractions).setAdapter(Factory.createSearchBasedLoadingAdapter(context));
    getGallery(R.id.food).setAdapter(Factory.createSearchBasedLoadingAdapter(context));
  }

  private void initNetworkBasedAdapters()
  {
    RecyclerView promoRecycler = getGallery(R.id.catalog_promo_recycler);
    ItemSelectedListener<Items.Item> listener = mOnlineMode
                                                ?
                                                new CatalogPromoSelectedListener(requireActivity())
                                                : new ErrorCatalogPromoListener<>(requireActivity(),
                                                                                  this::onNetworkPolicyResult);

    GalleryAdapter adapter = mOnlineMode ? Factory.createCatalogPromoLoadingAdapter(requireContext())
                                         : Factory.createCatalogPromoErrorAdapter(requireContext(), listener);
    promoRecycler.setAdapter(adapter);
  }

  private void requestDiscoveryInfo()
  {
    DiscoveryParams params;
    if (mOnlineMode)
    {
      params = new DiscoveryParams(Utils.getCurrencyCode(), Language.getDefaultLocale(),
                                   ITEMS_COUNT, ITEM_TYPES);
    }
    else
    {
      params = new DiscoveryParams(Utils.getCurrencyCode(), Language.getDefaultLocale(),
                                   ITEMS_COUNT,
                                   DiscoveryParams.ITEM_TYPE_HOTELS,
                                   DiscoveryParams.ITEM_TYPE_ATTRACTIONS,
                                   DiscoveryParams.ITEM_TYPE_CAFES);
    }
    DiscoveryManager.INSTANCE.discover(params);
  }

  @MainThread
  @Override
  public void onAttractionsReceived(@NonNull SearchResult[] results)
  {
    updateViewsVisibility(results, R.id.attractionsTitle, R.id.attractions);
    ItemSelectedListener<Items.SearchItem> listener = new SearchBasedListener(this,
                                                                              SEARCH_ATTRACTIONS,
                                                                              ItemType.ATTRACTIONS);
    RecyclerView gallery = getGallery(R.id.attractions);
    GalleryAdapter adapter = Factory.createSearchBasedAdapter(results, listener, SEARCH_ATTRACTIONS,
                                                              DISCOVERY,
                                                              new Items.MoreSearchItem(requireContext()));
    gallery.setAdapter(adapter);
  }

  @MainThread
  @Override
  public void onCafesReceived(@NonNull SearchResult[] results)
  {
    updateViewsVisibility(results, R.id.eatAndDrinkTitle, R.id.food);
    ItemSelectedListener<Items.SearchItem> listener = new SearchBasedListener(this,
                                                                              SEARCH_RESTAURANTS,
                                                                              ItemType.CAFES);
    RecyclerView gallery = getGallery(R.id.food);
    gallery.setAdapter(Factory.createSearchBasedAdapter(results, listener, SEARCH_RESTAURANTS,
                                                        DISCOVERY,
                                                        new Items.MoreSearchItem(requireContext())));
  }

  @Override
  public void onHotelsReceived(@NonNull SearchResult[] results)
  {
    updateViewsVisibility(results, R.id.hotelsTitle, R.id.hotels);
    ItemSelectedListener<Items.SearchItem> listener = new HotelListener(this);
    GalleryAdapter adapter = Factory.createHotelAdapter(requireContext(), results, listener,
                                                        SEARCH_HOTELS, DISCOVERY);
    RecyclerView gallery = getGallery(R.id.hotels);
    gallery.setAdapter(adapter);
  }

  @MainThread
  @Override
  public void onLocalExpertsReceived(@NonNull LocalExpert[] experts)
  {
    updateViewsVisibility(experts, R.id.localGuidesTitle, R.id.localGuides);
    String url = DiscoveryManager.nativeGetLocalExpertsUrl();

    ItemSelectedListener<Items.LocalExpertItem> listener
        = createOnlineProductItemListener(LOCAL_EXPERTS, ItemType.LOCAL_EXPERTS);

    RecyclerView gallery = getGallery(R.id.localGuides);
    GalleryAdapter adapter = Factory.createLocalExpertsAdapter(experts, url, listener, DISCOVERY);
    gallery.setAdapter(adapter);
  }

  @Override
  public void onCatalogPromoResultReceived(@NonNull PromoCityGallery gallery)
  {
    updateViewsVisibility(gallery.getItems(), R.id.catalog_promo_recycler,
                          R.id.catalog_promo_title);
    if (gallery.getItems().length == 0)
      return;

    String url = gallery.getMoreUrl();
    ItemSelectedListener<PromoEntity> listener =
        new RegularCatalogPromoListener(requireActivity(), DISCOVERY);
    GalleryAdapter adapter = Factory.createCatalogPromoAdapter(requireContext(), gallery, url,
                                                               listener, DISCOVERY);
    RecyclerView recycler = getGallery(R.id.catalog_promo_recycler);
    recycler.setAdapter(adapter);
  }

  @Override
  public void onError(@NonNull ItemType type)
  {
    switch (type)
    {
      case HOTELS:
        getGallery(R.id.hotels).setAdapter(Factory.createSearchBasedErrorAdapter(requireContext()));
        Statistics.INSTANCE.trackGalleryError(SEARCH_HOTELS, DISCOVERY, null);
        break;
      case ATTRACTIONS:
        getGallery(R.id.attractions).setAdapter(Factory.createSearchBasedErrorAdapter(requireContext()));
        Statistics.INSTANCE.trackGalleryError(SEARCH_ATTRACTIONS, DISCOVERY, null);
        break;
      case CAFES:
        getGallery(R.id.food).setAdapter(Factory.createSearchBasedErrorAdapter(requireContext()));
        Statistics.INSTANCE.trackGalleryError(SEARCH_RESTAURANTS, DISCOVERY, null);
        break;
      case LOCAL_EXPERTS:
        getGallery(R.id.localGuides).setAdapter(Factory.createLocalExpertsErrorAdapter(requireContext()));
        Statistics.INSTANCE.trackGalleryError(LOCAL_EXPERTS, DISCOVERY, null);
        break;
      case PROMO:
        GalleryAdapter adapter = Factory.createCatalogPromoErrorAdapter(requireContext(),
            new ErrorCatalogPromoListener<>(requireActivity(), this::onNetworkPolicyResult));
        RecyclerView gallery = getGallery(R.id.catalog_promo_recycler);
        gallery.setAdapter(adapter);
        Statistics.INSTANCE.trackGalleryError(PROMO, DISCOVERY, null);
        break;
      default:
        throw new AssertionError("Unknown item type: " + type);
    }
  }

  @Override
  public void onNotFound()
  {
    View view = getRootView();

    UiUtils.hide(view, R.id.galleriesLayout);

    PlaceholderView placeholder = (PlaceholderView) view.findViewById(R.id.placeholder);
    placeholder.setContent(R.drawable.img_cactus, R.string.discovery_button_404_error_title,
                           R.string.discovery_button_404_error_message);
    UiUtils.show(placeholder);
  }

  private <T> void updateViewsVisibility(@NonNull T[] results, @IdRes int... viewsId)
  {
    for (@IdRes int id : viewsId)
      UiUtils.showIf(results.length != 0, getRootView().findViewById(id));
  }

  @NonNull
  public View getRootView()
  {
    View view = getView();
    if (view == null)
      throw new AssertionError("Don't call getRootView when view is not created yet!");
    return view;
  }

  private void routeTo(@NonNull Items.SearchItem item)
  {
    if (mDiscoveryListener == null)
      return;

    mDiscoveryListener.onRouteToDiscoveredObject(createMapObject(item));
  }

  private void showSimilarItems(@NonNull Items.SearchItem item, @NonNull ItemType type)
  {
    if (mDiscoveryListener != null)
      mDiscoveryListener.onShowSimilarObjects(item, type);
  }

  private void showOnMap(@NonNull Items.SearchItem item)
  {
    if (mDiscoveryListener != null)
      mDiscoveryListener.onShowDiscoveredObject(createMapObject(item));
  }

  private void showFilter()
  {
    if (mDiscoveryListener != null)
      mDiscoveryListener.onShowFilter();
  }

  @NonNull
  private MapObject createMapObject(@NonNull Items.SearchItem item)
  {
    String featureType = item.getFeatureType();
    Context context = requireContext();
    String subtitle = Utils.getLocalizedFeatureType(context, featureType);

    String title = TextUtils.isEmpty(item.getTitle(context)) ? subtitle : item.getTitle(context);

    return MapObject.createMapObject(item.getFeatureId(), MapObject.SEARCH, title, subtitle,
                                     item.getLat(), item.getLon());
  }

  @NonNull
  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(getRootView(), requireActivity())
    {
      @Override
      public void onUpClick()
      {
        if (mNavigateUpListener == null)
          return;

        mNavigateUpListener.customOnNavigateUp();
      }
    };
  }

  private <I extends Items.Item> ItemSelectedListener<I> createOnlineProductItemListener(@NonNull GalleryType galleryType,
                                                                                         @NonNull ItemType itemType)
  {
    return new LoggableItemSelectedListener<I>(getActivity(), itemType)
    {
      @Override
      public void onItemSelectedInternal(@NonNull I item, int position)
      {
        Statistics.INSTANCE.trackGalleryProductItemSelected(galleryType, DISCOVERY, position,
                                                            EXTERNAL);
      }

      @Override
      public void onMoreItemSelectedInternal(@NonNull I item)
      {
        Statistics.INSTANCE.trackGalleryEvent(Statistics.EventName.PP_SPONSOR_MORE_SELECTED,
                                              galleryType,
                                              DISCOVERY);
      }
    };
  }

  private static class SearchBasedListener extends LoggableItemSelectedListener<Items.SearchItem>
  {
    @NonNull
    private final DiscoveryFragment mFragment;

    @NonNull
    private final GalleryType mType;

    private SearchBasedListener(@NonNull DiscoveryFragment fragment,
                                @NonNull GalleryType galleryType,
                                @NonNull ItemType itemType)
    {
      super(fragment.getActivity(), itemType);
      mFragment = fragment;
      mType = galleryType;
    }

    @Override
    protected void openUrl(@NonNull Items.SearchItem item)
    {
      /* Do nothing */
    }

    @Override
    public void onMoreItemSelectedInternal(@NonNull Items.SearchItem item)
    {
      mFragment.showSimilarItems(item, getType());
    }

    @Override
    @CallSuper
    public void onItemSelectedInternal(@NonNull Items.SearchItem item, int position)
    {
      mFragment.showOnMap(item);
      Statistics.INSTANCE.trackGalleryProductItemSelected(mType, DISCOVERY, position, PLACEPAGE);
    }

    @Override
    @CallSuper
    public void onActionButtonSelected(@NonNull Items.SearchItem item, int position)
    {
      mFragment.routeTo(item);
      Statistics.INSTANCE.trackGalleryProductItemSelected(mType, DISCOVERY, position,
                                                          ROUTING);
    }

    @NonNull
    DiscoveryFragment getFragment()
    {
      return mFragment;
    }
  }

  private static class HotelListener extends SearchBasedListener
  {
    private HotelListener(@NonNull DiscoveryFragment fragment)
    {
      super(fragment, SEARCH_HOTELS, ItemType.HOTELS);
    }

    @Override
    public void onMoreItemSelectedInternal(@NonNull Items.SearchItem item)
    {
      getFragment().showFilter();
      Statistics.INSTANCE.trackGalleryEvent(Statistics.EventName.PP_SPONSOR_MORE_SELECTED,
                                            SEARCH_HOTELS, DISCOVERY);
    }
  }

  public interface DiscoveryListener {
    void onRouteToDiscoveredObject(@NonNull MapObject object);
    void onShowDiscoveredObject(@NonNull MapObject object);
    void onShowFilter();
    void onShowSimilarObjects(@NonNull Items.SearchItem item, @NonNull ItemType type);
  }


  private static class CatalogPromoSelectedListener extends LoggableItemSelectedListener<Items.Item>
  {
    public CatalogPromoSelectedListener(@NonNull Activity activity)
    {
      super(activity, ItemType.PROMO);
    }

    @Override
    protected void onMoreItemSelectedInternal(@NonNull Items.Item item)
    {
    }

    @Override
    protected void onItemSelectedInternal(@NonNull Items.Item item, int position)
    {
    }
  }
}
