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
import java.util.ArrayList;
import java.util.List;

public class PlacePageScheduleFragment extends Fragment implements Observer<MapObject>
{
  private static final long LOADING_DELAY_MS = 2000;

  // Set to true to test error state
  private static final boolean SIMULATE_ERROR = false;

  private PlacePageViewModel mViewModel;
  private ScheduleAdapter mAdapter;
  private RecyclerView mRecyclerView;
  private ViewGroup mShimmerContainer;
  private ViewGroup mErrorContainer;
  private View mRetryButton;
  private final Handler mHandler = new Handler(Looper.getMainLooper());

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

    mAdapter = new ScheduleAdapter();
    mRecyclerView.setAdapter(mAdapter);

    mRetryButton.setOnClickListener(v -> retryLoading());

    loadSchedule();
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
        List<ScheduleAdapter.ScheduleItem> items = generateMockSchedule();
        showContent();
        mAdapter.setItems(items);
      }
    }, LOADING_DELAY_MS);
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

  private List<ScheduleAdapter.ScheduleItem> generateMockSchedule()
  {
    List<ScheduleAdapter.ScheduleItem> items = new ArrayList<>();

    // Routes matching original refs: 114, 303, N41, N91
    // isDynamic=true for real-time tracked routes
    items.add(new ScheduleAdapter.ScheduleItem("114", "Central Station", "2 min", Color.parseColor("#E53935"),
                                               ScheduleAdapter.TransportType.BUS, true));

    items.add(new ScheduleAdapter.ScheduleItem("303", "Victory Square", "5 min", Color.parseColor("#1E88E5"),
                                               ScheduleAdapter.TransportType.BUS, true));

    // Delayed arrival (dynamic)
    items.add(new ScheduleAdapter.ScheduleItem("N41", "Airport", "8 min", "6 min",
                                               ScheduleAdapter.ArrivalStatus.DELAYED, Color.parseColor("#43A047"),
                                               ScheduleAdapter.TransportType.BUS, true));

    // New route NOT in original refs - static schedule (no real-time tracking)
    items.add(new ScheduleAdapter.ScheduleItem("128", "University", "3 min", Color.parseColor("#FB8C00"),
                                               ScheduleAdapter.TransportType.TRAM, false));

    // N91 has no schedule data - will show without time

    // More arrivals - static schedule
    items.add(new ScheduleAdapter.ScheduleItem("114", "Central Station", "12 min", Color.parseColor("#E53935"),
                                               ScheduleAdapter.TransportType.BUS, false));

    items.add(new ScheduleAdapter.ScheduleItem("303", "Victory Square", "15 min", Color.parseColor("#1E88E5"),
                                               ScheduleAdapter.TransportType.TRAM, false));

    // Another new route NOT in original refs - dynamic
    items.add(new ScheduleAdapter.ScheduleItem("E-2", "Shopping Mall", "10 min", Color.parseColor("#8E24AA"),
                                               ScheduleAdapter.TransportType.BUS, true));

    // Early arrival (dynamic)
    items.add(new ScheduleAdapter.ScheduleItem("N41", "Airport", "18 min", "20 min",
                                               ScheduleAdapter.ArrivalStatus.EARLY, Color.parseColor("#43A047"),
                                               ScheduleAdapter.TransportType.BUS, true));

    items.add(new ScheduleAdapter.ScheduleItem("128", "University", "20 min", Color.parseColor("#FB8C00"),
                                               ScheduleAdapter.TransportType.TRAM, false));

    return items;
  }
}
