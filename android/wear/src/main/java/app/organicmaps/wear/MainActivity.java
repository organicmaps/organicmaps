package app.organicmaps.wear;

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import app.organicmaps.wear.protocol.WearNavigationData;
import app.organicmaps.wear.protocol.WearNavigationMode;
import app.organicmaps.wear.protocol.WearNavigationState;
import app.organicmaps.wear.protocol.WearNavigationStateCodec;
import com.google.android.gms.wearable.CapabilityClient;
import com.google.android.gms.wearable.DataClient;
import com.google.android.gms.wearable.DataEventBuffer;
import com.google.android.gms.wearable.DataItemBuffer;
import com.google.android.gms.wearable.Node;
import com.google.android.gms.wearable.PutDataRequest;
import com.google.android.gms.wearable.Wearable;

public final class MainActivity extends Activity implements DataClient.OnDataChangedListener
{
  private static final String TAG = MainActivity.class.getSimpleName();
  private static final Uri NAVIGATION_STATE_URI = navigationStateUri("*");

  private TextView mSubtitle;
  private DataClient mDataClient;
  private CapabilityClient mCapabilityClient;
  private boolean mResumed;
  private int mLifecycleGeneration;
  private int mRefreshGeneration;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    LinearLayout layout = new LinearLayout(this);
    layout.setGravity(Gravity.CENTER);
    layout.setOrientation(LinearLayout.VERTICAL);
    int padding = getResources().getDimensionPixelSize(R.dimen.screen_padding);
    layout.setPadding(padding, padding, padding, padding);

    TextView title = new TextView(this);
    title.setGravity(Gravity.CENTER);
    title.setText(R.string.app_name);
    title.setTextSize(18);

    mSubtitle = new TextView(this);
    mSubtitle.setGravity(Gravity.CENTER);
    mSubtitle.setTextSize(14);

    layout.addView(title);
    layout.addView(mSubtitle);

    setContentView(layout);
    mDataClient = Wearable.getDataClient(this);
    mCapabilityClient = Wearable.getCapabilityClient(this);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    // Listen only while visible to avoid background wakeups. Register before reading the persistent
    // item so a state change cannot be missed between the initial read and listener registration.
    mResumed = true;
    final int generation = ++mLifecycleGeneration;
    mDataClient.addListener(this, NAVIGATION_STATE_URI, DataClient.FILTER_LITERAL)
        .addOnSuccessListener(unused -> {
          if (isResumed(generation))
            refreshNavigationState();
          else if (!mResumed)
            mDataClient.removeListener(this);
        })
        .addOnFailureListener(e -> {
          Log.w(TAG, "Failed to listen for navigation state", e);
          if (isResumed(generation))
          {
            ++mRefreshGeneration;
            renderNormal();
          }
        });
  }

  @Override
  protected void onPause()
  {
    mResumed = false;
    ++mLifecycleGeneration;
    ++mRefreshGeneration;
    mDataClient.removeListener(this).addOnFailureListener(e -> Log.w(TAG, "Failed to stop navigation listener", e));
    super.onPause();
  }

  @Override
  public void onDataChanged(DataEventBuffer dataEvents)
  {
    if (mResumed && NavigationStateSource.requiresRefresh(dataEvents))
      refreshNavigationState();
  }

  private void refreshNavigationState()
  {
    final int generation = ++mRefreshGeneration;
    mCapabilityClient.getCapability(WearNavigationData.CAPABILITY_PHONE_APP, CapabilityClient.FILTER_REACHABLE)
        .addOnSuccessListener(capability -> {
          if (!isCurrent(generation))
            return;

          final Node companion = NavigationStateSource.selectCompanion(capability.getNodes());
          if (companion == null)
          {
            renderNormal();
            return;
          }
          readNavigationState(companion, generation);
        })
        .addOnFailureListener(e -> {
          Log.w(TAG, "Failed to find the companion phone", e);
          if (isCurrent(generation))
            renderNormal();
        });
  }

  private void readNavigationState(@NonNull Node companion, int generation)
  {
    final Uri uri = navigationStateUri(companion.getId());
    mDataClient.getDataItems(uri, DataClient.FILTER_LITERAL)
        .addOnSuccessListener(dataItems -> renderFromDataItems(dataItems, generation))
        .addOnFailureListener(e -> {
          Log.w(TAG, "Failed to read navigation state", e);
          if (isCurrent(generation))
            renderNormal();
        });
  }

  private void renderFromDataItems(@NonNull DataItemBuffer dataItems, int generation)
  {
    WearNavigationState state = null;
    try
    {
      // A node owns at most one DataItem at a path. Treat any unexpected result as no current state.
      if (dataItems.getCount() == 1)
        state = WearNavigationStateCodec.decode(dataItems.get(0).getData());
    }
    finally
    {
      dataItems.release();
    }

    if (isCurrent(generation))
      render(state != null ? state : WearNavigationState.normal());
  }

  private boolean isCurrent(int generation)
  {
    return mResumed && generation == mRefreshGeneration;
  }

  private boolean isResumed(int generation)
  {
    return mResumed && generation == mLifecycleGeneration;
  }

  private void renderNormal()
  {
    render(WearNavigationState.normal());
  }

  private void render(@NonNull WearNavigationState state)
  {
    boolean navigating = state.getMode() == WearNavigationMode.NAVIGATION;
    mSubtitle.setText(navigating ? R.string.wear_navigation_active_message : R.string.wear_no_navigation_message);
  }

  @NonNull
  private static Uri navigationStateUri(@NonNull String authority)
  {
    return new Uri.Builder()
        .scheme(PutDataRequest.WEAR_URI_SCHEME)
        .authority(authority)
        .path(WearNavigationData.PATH_NAVIGATION_STATE)
        .build();
  }
}
