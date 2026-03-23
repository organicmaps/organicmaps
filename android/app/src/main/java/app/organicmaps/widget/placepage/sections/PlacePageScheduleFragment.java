package app.organicmaps.widget.placepage.sections;

import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import app.organicmaps.widget.placepage.sections.schedule.ArrivalStatus;
import app.organicmaps.widget.placepage.sections.schedule.GroupedScheduleAdapter;
import app.organicmaps.widget.placepage.sections.schedule.PinnedRoutesManager;
import app.organicmaps.widget.placepage.sections.schedule.ScheduleArrival;
import app.organicmaps.widget.placepage.sections.schedule.ScheduleDetailBottomSheet;
import app.organicmaps.widget.placepage.sections.schedule.ScheduleDirection;
import app.organicmaps.widget.placepage.sections.schedule.ScheduleRoute;
import app.organicmaps.widget.placepage.sections.schedule.ScheduleSection;
import app.organicmaps.widget.placepage.sections.schedule.TransportType;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

public class PlacePageScheduleFragment extends Fragment implements Observer<MapObject>
{
  private static final long LOADING_DELAY_MS = 2000;

  // Set to true to test error state
  private static final boolean SIMULATE_ERROR = false;

  private PlacePageViewModel mViewModel;
  private GroupedScheduleAdapter mAdapter;
  private RecyclerView mRecyclerView;
  private ViewGroup mShimmerContainer;
  private ViewGroup mErrorContainer;
  private View mRetryButton;
  private final Handler mHandler = new Handler(Looper.getMainLooper());
  private PinnedRoutesManager mPinnedRoutesManager;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_schedule_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mRecyclerView = view.findViewById(R.id.rv_schedule);
    mShimmerContainer = view.findViewById(R.id.shimmer_container);
    mErrorContainer = view.findViewById(R.id.error_container);
    mRetryButton = view.findViewById(R.id.btn_retry);

    mPinnedRoutesManager = new PinnedRoutesManager(requireContext());

    mAdapter = new GroupedScheduleAdapter();
    mAdapter.setOnPinClickListener(this::onPinClicked);
    mAdapter.setOnRouteDetailsClickListener(this::onDetailsClicked);
    mRecyclerView.setAdapter(mAdapter);

    mRetryButton.setOnClickListener(v -> retryLoading());

    loadSchedule();
  }

  private void onPinClicked(ScheduleRoute route)
  {
    mPinnedRoutesManager.togglePin(route.routeId);
    // Refresh the list to reflect pinned state changes
    refreshScheduleDisplay();
  }

  private void onDetailsClicked(ScheduleRoute route, ScheduleDirection direction)
  {
    ScheduleDetailBottomSheet bottomSheet = ScheduleDetailBottomSheet.newInstance(route, direction);
    bottomSheet.show(getChildFragmentManager(), "schedule_detail");
  }

  private void loadSchedule()
  {
    showLoading();
    startShimmerAnimation();
    simulateLoading();
  }

  private void retryLoading()
  {
    loadSchedule();
  }

  private void showLoading()
  {
    mShimmerContainer.setVisibility(View.VISIBLE);
    mErrorContainer.setVisibility(View.GONE);
    mRecyclerView.setVisibility(View.GONE);
  }

  private void showError()
  {
    stopShimmerAnimation();
    mShimmerContainer.setVisibility(View.GONE);
    mErrorContainer.setVisibility(View.VISIBLE);
    mRecyclerView.setVisibility(View.GONE);
  }

  private void showContent()
  {
    stopShimmerAnimation();
    mShimmerContainer.setVisibility(View.GONE);
    mErrorContainer.setVisibility(View.GONE);
    mRecyclerView.setVisibility(View.VISIBLE);
  }

  private void startShimmerAnimation()
  {
    for (int i = 0; i < mShimmerContainer.getChildCount(); i++)
    {
      View child = mShimmerContainer.getChildAt(i);
      Animation animation = AnimationUtils.loadAnimation(requireContext(), R.anim.shimmer_animation);
      animation.setStartOffset(i * 150L);
      child.startAnimation(animation);
    }
  }

  private void simulateLoading()
  {
    mHandler.postDelayed(() -> {
      if (!isAdded())
        return;

      if (SIMULATE_ERROR)
        showError();
      else
      {
        showContent();
        refreshScheduleDisplay();
      }
    }, LOADING_DELAY_MS);
  }

  private void refreshScheduleDisplay()
  {
    List<ScheduleRoute> routes = generateMockRoutes();
    List<ScheduleSection> sections = createSections(routes);
    mAdapter.setSections(sections);
  }

  private List<ScheduleSection> createSections(List<ScheduleRoute> routes)
  {
    List<ScheduleSection> sections = new ArrayList<>();
    Set<String> pinnedIds = mPinnedRoutesManager.getPinnedRouteIds();

    // Apply pinned state to routes
    for (ScheduleRoute route : routes)
      route.isPinned = pinnedIds.contains(route.routeId);

    // Separate pinned and regular routes
    List<ScheduleRoute> pinnedRoutes = new ArrayList<>();
    List<ScheduleRoute> regularRoutes = new ArrayList<>();

    for (ScheduleRoute route : routes)
    {
      if (route.isPinned)
        pinnedRoutes.add(route);
      else
        regularRoutes.add(route);
    }

    // Add pinned section without header
    if (!pinnedRoutes.isEmpty())
      sections.add(new ScheduleSection(pinnedRoutes));

    // Add regular routes without section header
    if (!regularRoutes.isEmpty())
      sections.add(new ScheduleSection(regularRoutes));

    return sections;
  }

  private void stopShimmerAnimation()
  {
    for (int i = 0; i < mShimmerContainer.getChildCount(); i++)
      mShimmerContainer.getChildAt(i).clearAnimation();
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mViewModel.getMapObject().removeObserver(this);
    mHandler.removeCallbacksAndMessages(null);
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mHandler.removeCallbacksAndMessages(null);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    // Data is loaded in onViewCreated, observer kept for future real API
  }

  private List<ScheduleRoute> generateMockRoutes()
  {
    List<ScheduleRoute> routes = new ArrayList<>();

    // Route 114 - Bus with two directions
    List<ScheduleDirection> directions114 = new ArrayList<>();
    List<ScheduleArrival> arrivals114a = new ArrayList<>();
    arrivals114a.add(new ScheduleArrival("2 min", true));
    arrivals114a.add(new ScheduleArrival("12 min", false));
    directions114.add(new ScheduleDirection("Central Station", arrivals114a));

    List<ScheduleArrival> arrivals114b = new ArrayList<>();
    arrivals114b.add(new ScheduleArrival("6 min", true));
    arrivals114b.add(new ScheduleArrival("16 min", false));
    directions114.add(new ScheduleDirection("Shopping Mall", arrivals114b));

    routes.add(new ScheduleRoute("114", "114", "114", Color.parseColor("#E53935"), TransportType.BUS, directions114));

    // Route 303 - Bus with one direction
    List<ScheduleDirection> directions303 = new ArrayList<>();
    List<ScheduleArrival> arrivals303 = new ArrayList<>();
    arrivals303.add(new ScheduleArrival("5 min", true));
    arrivals303.add(new ScheduleArrival("15 min", false));
    directions303.add(new ScheduleDirection("Victory Square", arrivals303));

    routes.add(new ScheduleRoute("303", "303", "303", Color.parseColor("#1E88E5"), TransportType.BUS, directions303));

    // Route N41 - Night bus with delayed arrival
    List<ScheduleDirection> directionsN41 = new ArrayList<>();
    List<ScheduleArrival> arrivalsN41 = new ArrayList<>();
    arrivalsN41.add(new ScheduleArrival("8 min", "6 min", ArrivalStatus.DELAYED, true));
    arrivalsN41.add(new ScheduleArrival("18 min", true));
    directionsN41.add(new ScheduleDirection("Airport", arrivalsN41));

    routes.add(new ScheduleRoute("N41", "N41", "N41", Color.parseColor("#43A047"), TransportType.BUS, directionsN41));

    // Route 128 - Tram with two directions
    List<ScheduleDirection> directions128 = new ArrayList<>();
    List<ScheduleArrival> arrivals128a = new ArrayList<>();
    arrivals128a.add(new ScheduleArrival("3 min", false));
    arrivals128a.add(new ScheduleArrival("20 min", false));
    directions128.add(new ScheduleDirection("University", arrivals128a));

    List<ScheduleArrival> arrivals128b = new ArrayList<>();
    arrivals128b.add(new ScheduleArrival("7 min", false));
    arrivals128b.add(new ScheduleArrival("24 min", false));
    directions128.add(new ScheduleDirection("Central Station", arrivals128b));

    routes.add(new ScheduleRoute("128", "128", "128", Color.parseColor("#FB8C00"), TransportType.TRAM, directions128));

    return routes;
  }
}
