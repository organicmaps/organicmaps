package com.mapswithme.maps.discovery;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.IdRes;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.gallery.impl.BaseItemSelectedListener;
import com.mapswithme.maps.gallery.impl.Factory;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.viator.ViatorProduct;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Language;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class DiscoveryFragment extends BaseMwmToolbarFragment implements UICallback
{
  private static final int ITEMS_COUNT = 5;
  private static final int[] ITEM_TYPES = { DiscoveryParams.ITEM_TYPE_VIATOR,
                                            DiscoveryParams.ITEM_TYPE_ATTRACTIONS,
                                            DiscoveryParams.ITEM_TYPE_CAFES };
  @Nullable
  private BaseItemSelectedListener<Items.Item> mDefaultListener;
  private boolean mOnlineMode;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_discovery, container, false);
  }

  private void initViatorGallery()
  {
    setLayoutManagerAndItemDecoration(getContext(), getGallery(R.id.thingsToDo));
  }

  private void initAttractionsGallery()
  {
    setLayoutManagerAndItemDecoration(getContext(), getGallery(R.id.attractions));
  }

  private void initFoodGallery()
  {
    setLayoutManagerAndItemDecoration(getContext(), getGallery(R.id.food));
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

    RecyclerView rv = (RecyclerView) view.findViewById(id);
    if (rv == null)
      throw new AssertionError("RecyclerView must be within root view!");
    return rv;
  }

  @Override
  public void onStart()
  {
    super.onStart();
    DiscoveryManager.INSTANCE.attach(this);
  }

  @Override
  public void onStop()
  {
    DiscoveryManager.INSTANCE.detach();
    super.onStop();
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController.setTitle(R.string.discovery_button_title);
    mDefaultListener = new BaseItemSelectedListener<>(getActivity());
    view.findViewById(R.id.viatorLogo).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        Utils.openUrl(getActivity(), DiscoveryManager.nativeGetViatorUrl());
      }
    });
    initViatorGallery();
    initAttractionsGallery();
    initFoodGallery();
    requestDiscoveryInfoAndInitAdapters();
  }

  private void requestDiscoveryInfoAndInitAdapters()
  {
    initSearchBasedAdapters();

    NetworkPolicy.checkNetworkPolicy(getFragmentManager(), new NetworkPolicy.NetworkPolicyListener()
    {
      @Override
      public void onResult(@NonNull NetworkPolicy policy)
      {
        mOnlineMode = policy.сanUseNetwork();
        initNetworkBasedAdapters();
        requestDiscoveryInfo();
      }
    });
  }

  private void initSearchBasedAdapters()
  {
    getGallery(R.id.attractions).setAdapter(Factory.createSearchBasedLoadingAdapter());
    getGallery(R.id.food).setAdapter(Factory.createSearchBasedLoadingAdapter());
  }

  private void initNetworkBasedAdapters()
  {
    if (mOnlineMode)
    {
      // TODO: set loading adapter for local experts here.
      RecyclerView thinsToDo = getGallery(R.id.thingsToDo);
      thinsToDo.setAdapter(Factory.createViatorLoadingAdapter(DiscoveryManager.nativeGetViatorUrl(),
                                                              mDefaultListener));
      return;
    }

    // It means that the user doesn't permit mobile network usage, so network based galleries UI
    // should be hidden in this case.
    if (ConnectionState.isMobileConnected())
    {
      UiUtils.hide(getView(), R.id.thingsToDoLayout, R.id.thingsToDo,
                   R.id.localGuidesTitle, R.id.localGuides);
    }
    else
    {
      UiUtils.show(getView(), R.id.thingsToDoLayout, R.id.thingsToDo);
      RecyclerView thinsToDo = getGallery(R.id.thingsToDo);
      thinsToDo.setAdapter(Factory.createViatorOfflineAdapter(new ViatorOfflineSelectedListener
                                                                  (getActivity())));
    }
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
                                   ITEMS_COUNT, DiscoveryParams.ITEM_TYPE_ATTRACTIONS,
                                   DiscoveryParams.ITEM_TYPE_CAFES);
    }
    DiscoveryManager.INSTANCE.discover(params);
  }

  @MainThread
  @Override
  public void onAttractionsReceived(@Nullable SearchResult[] results)
  {
    if (results == null)
      return;

    ItemSelectedListener<Items.SearchItem> listener = new SearchBasedListener(getActivity());
    getGallery(R.id.attractions).setAdapter(Factory.createSearchBasedAdapter(results, listener));
  }

  @MainThread
  @Override
  public void onCafesReceived(@Nullable SearchResult[] results)
  {
    if (results == null)
      return;

    ItemSelectedListener<Items.SearchItem> listener = new SearchBasedListener(getActivity());
    getGallery(R.id.food).setAdapter(Factory.createSearchBasedAdapter(results, listener));
  }

  @MainThread
  @Override
  public void onViatorProductsReceived(@Nullable ViatorProduct[] products)
  {
    if (products == null)
      return;

    String url = DiscoveryManager.nativeGetViatorUrl();
    ItemSelectedListener<Items.ViatorItem> listener = new BaseItemSelectedListener<>(getActivity());
    getGallery(R.id.thingsToDo).setAdapter(Factory.createViatorAdapter(products, url, listener));
  }

  @MainThread
  @Override
  public void onLocalExpertsReceived(@Nullable LocalExpert[] experts)
  {

  }

  @Override
  public void onError(@NonNull ItemType type)
  {
    switch (type)
    {
      case VIATOR:
        String url = DiscoveryManager.nativeGetViatorUrl();
        getGallery(R.id.thingsToDo).setAdapter(Factory.createViatorErrorAdapter(url, mDefaultListener));
        break;
      case ATTRACTIONS:
        getGallery(R.id.attractions).setAdapter(Factory.createSearchBasedErrorAdapter());
        break;
      case CAFES:
        getGallery(R.id.food).setAdapter(Factory.createSearchBasedErrorAdapter());
        break;
      case LOCAL_EXPERTS:
        //TODO: coming soon.
        break;
      default:
        throw new AssertionError("Unknown item type: " + type);
    }
  }

  private static class ViatorOfflineSelectedListener extends BaseItemSelectedListener<Items.Item>
  {
    private ViatorOfflineSelectedListener(@NonNull Activity context)
    {
      super(context);
    }

    @Override
    public void onActionButtonSelected(@NonNull Items.Item item)
    {
      Utils.showWirelessSettings(getContext());
    }
  }

  private static class SearchBasedListener extends BaseItemSelectedListener<Items.SearchItem>
  {
    private SearchBasedListener(@NonNull Activity context)
    {
      super(context);

    }

    @Override
    public void onItemSelected(@NonNull Items.SearchItem item)
    {
      // TODO: Show point on the map. Coming soon.
    }

    @Override
    public void onActionButtonSelected(@NonNull Items.SearchItem item)
    {
      Intent intent = new Intent();
      String title = TextUtils.isEmpty(item.getTitle()) ? "" : item.getTitle();
      String subtitle = TextUtils.isEmpty(item.getSubtitle()) ? "" : item.getSubtitle();
      MapObject poi = MapObject.createMapObject(FeatureId.EMPTY, MapObject.SEARCH, title, subtitle,
                                                item.getLat(), item.getLon());
      intent.putExtra(DiscoveryActivity.EXTRA_DISCOVERY_OBJECT, poi);
      getContext().setResult(Activity.RESULT_OK, intent);
      getContext().finish();
    }
  }
}
