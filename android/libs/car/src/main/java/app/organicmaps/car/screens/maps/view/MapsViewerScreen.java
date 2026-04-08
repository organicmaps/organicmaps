package app.organicmaps.car.screens.maps.view;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.Tab;
import androidx.car.app.model.TabTemplate;
import androidx.car.app.model.Template;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.screens.BaseScreen;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

public class MapsViewerScreen extends BaseScreen implements TabTemplate.TabCallback
{
  @NonNull
  private final List<TabController> mTabs;
  @NonNull
  private final Map<String, TabController> mContentIdToTab;
  @NonNull
  private final TabController mBackTab;
  @NonNull
  private TabController mActiveTab;

  public MapsViewerScreen(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext)
  {
    super(carContext, organicMapsContext);

    mTabs = new ArrayList<>();
    mContentIdToTab = new HashMap<>();
    mBackTab = new BackTabController(this);
    mActiveTab = initTabs();
    setupCrossTabRefresh();
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final TabTemplate.Builder builder = new TabTemplate.Builder(this);
    builder.setHeaderAction(Action.APP_ICON);

    addTabs(builder);
    setCurrentTab(builder);

    return builder.build();
  }

  private void addTabs(@NonNull TabTemplate.Builder builder)
  {
    for (final TabController tab : mTabs)
    {
      builder.addTab(new Tab.Builder()
                         .setTitle(getCarContext().getString(tab.getTabName()))
                         .setContentId(tab.getTabContentId())
                         .setIcon(tab.getTabIcon())
                         .build());
    }
  }

  private void setCurrentTab(@NonNull TabTemplate.Builder builder)
  {
    builder.setActiveTabContentId(mActiveTab.getTabContentId());
    builder.setTabContents(mActiveTab.getTabContents());
  }

  @Override
  public void onTabSelected(@NonNull String tabContentId)
  {
    if (mBackTab.getTabContentId().equals(tabContentId))
    {
      final boolean hasCurrentTabProcessedBackPress = mActiveTab.onBackPressed();
      if (!hasCurrentTabProcessedBackPress)
        mBackTab.onBackPressed();
      return;
    }

    mActiveTab = Objects.requireNonNull(mContentIdToTab.get(tabContentId));
    invalidate();
  }

  /** Registers all content tabs and returns the default active tab (All Maps). */
  @NonNull
  private TabController initTabs()
  {
    // mBackTab is intentionally NOT added to mContentIdToTab: its selection is handled
    // before the map lookup in onTabSelected, so a map entry would be dead code.
    mTabs.add(mBackTab);

    final TabController allMapsTab = new AllMapsTabController(this);
    registerTab(allMapsTab);

    final TabController myMapsTab = new MyMapsTabController(this);
    registerTab(myMapsTab);

    final TabController updatableMapsTab = new UpdatableMapsTabController(this);
    registerTab(updatableMapsTab);

    return allMapsTab;
  }

  private void registerTab(@NonNull TabController tab)
  {
    mTabs.add(tab);
    mContentIdToTab.put(tab.getTabContentId(), tab);
  }

  /**
   * Registers a shared callback on every content tab so that when a map download/removal
   * completes in any one tab, all tabs refresh their data automatically.
   * The back tab is excluded as it has no data to refresh.
   */
  private void setupCrossTabRefresh()
  {
    final Runnable refreshAll = () ->
    {
      for (final TabController tab : mContentIdToTab.values())
        tab.refreshCurrentFolder();
    };
    for (final TabController tab : mContentIdToTab.values())
      tab.setOnMapChangedCallback(refreshAll);
  }
}
