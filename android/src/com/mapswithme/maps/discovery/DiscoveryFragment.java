package com.mapswithme.maps.discovery;

import android.content.Context;
import android.os.Bundle;
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
import com.mapswithme.maps.gallery.GalleryAdapter;
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
  private static final int[] ITEM_TYPES = { DiscoveryParams.ITEM_TYPE_VIATOR };
  private static final GalleryAdapter.ItemSelectedListener LISTENER = new BaseItemSelectedListener();
  private boolean mOnlineMode;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView mThingsToDo;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View view = inflater.inflate(R.layout.fragment_discovery, container, false);

    initViatorGallery(view);
    return view;
  }

  private void initViatorGallery(@NonNull View view)
  {
    view.findViewById(R.id.viatorLogo).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        Utils.openUrl(getActivity(), DiscoveryManager.nativeGetViatorUrl());
      }
    });
    mThingsToDo = (RecyclerView) view.findViewById(R.id.thingsToDo);
    mThingsToDo.setLayoutManager(new LinearLayoutManager(getContext(),
                                                         LinearLayoutManager.HORIZONTAL,
                                                         false));
    mThingsToDo.addItemDecoration(
        ItemDecoratorFactory.createSponsoredGalleryDecorator(getContext(), LinearLayoutManager.HORIZONTAL));
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
    requestDiscoveryInfoAndInitAdapters();
  }

  private void requestDiscoveryInfoAndInitAdapters()
  {
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

  private void initNetworkBasedAdapters()
  {
    if (mOnlineMode)
    {
      // TODO: set loading adapter for local experts here.
      mThingsToDo.setAdapter(Factory.createViatorLoadingAdapter(DiscoveryManager.nativeGetViatorUrl(),
                                                                LISTENER));
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
      mThingsToDo.setAdapter(Factory.createViatorOfflineAdapter(new ViatorOfflineSelectedListener()));
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

  }

  @MainThread
  @Override
  public void onCafesReceived(@Nullable SearchResult[] results)
  {

  }

  @MainThread
  @Override
  public void onViatorProductsReceived(@Nullable ViatorProduct[] products)
  {
    if (products != null)
      mThingsToDo.setAdapter(Factory.createViatorAdapter(products, DiscoveryManager.nativeGetViatorUrl(),
                                                         LISTENER));
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
        mThingsToDo.setAdapter(Factory.createViatorErrorAdapter(DiscoveryManager.nativeGetViatorUrl(),
                                                                LISTENER));
        break;

      // TODO: processing for other adapters is coming soon.
    }
  }

  private static class BaseItemSelectedListener implements GalleryAdapter.ItemSelectedListener
  {

    @Override
    public void onItemSelected(@NonNull Context context, @NonNull String url)
    {
      Utils.openUrl(context, url);
    }

    @Override
    public void onMoreItemSelected(@NonNull Context context, @NonNull String url)
    {
      Utils.openUrl(context, url);
    }

    @Override
    public void onDetailsSelected(@NonNull Context context, @Nullable String url)
    {
      if (TextUtils.isEmpty(url))
        return;

      Utils.openUrl(context, url);
    }
  }

  private static class ViatorOfflineSelectedListener extends BaseItemSelectedListener
  {
    @Override
    public void onDetailsSelected(@NonNull Context context, @Nullable String url)
    {
      Utils.showWirelessSettings(context);
    }
  }
}
