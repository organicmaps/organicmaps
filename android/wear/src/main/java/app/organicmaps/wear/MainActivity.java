package app.organicmaps.wear;

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;
import app.organicmaps.wear.protocol.WearNavigationData;
import app.organicmaps.wear.protocol.WearNavigationMode;
import app.organicmaps.wear.protocol.WearNavigationState;
import app.organicmaps.wear.protocol.WearNavigationStateCodec;
import com.google.android.gms.wearable.DataClient;
import com.google.android.gms.wearable.DataEvent;
import com.google.android.gms.wearable.DataEventBuffer;
import com.google.android.gms.wearable.DataItem;
import com.google.android.gms.wearable.DataItemBuffer;
import com.google.android.gms.wearable.PutDataRequest;
import com.google.android.gms.wearable.Wearable;

public final class MainActivity extends Activity implements DataClient.OnDataChangedListener
{
  private static final Uri NAVIGATION_STATE_URI = new Uri.Builder()
                                                      .scheme(PutDataRequest.WEAR_URI_SCHEME)
                                                      .authority("*")
                                                      .path(WearNavigationData.PATH_NAVIGATION_STATE)
                                                      .build();

  private TextView mSubtitle;

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
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    // Phase 1 keeps Wear sync foreground-only: read the latest published state and listen for
    // changes while the screen is shown. The Wear Data Layer persists the last published item, so a
    // fresh read reflects the current navigation state even with no live event.
    DataClient dataClient = Wearable.getDataClient(this);
    dataClient.addListener(this, NAVIGATION_STATE_URI, DataClient.FILTER_LITERAL);
    dataClient.getDataItems(NAVIGATION_STATE_URI, DataClient.FILTER_LITERAL)
        .addOnSuccessListener(this::renderFromDataItems);
  }

  @Override
  protected void onPause()
  {
    Wearable.getDataClient(this).removeListener(this);
    super.onPause();
  }

  @Override
  public void onDataChanged(DataEventBuffer dataEvents)
  {
    try
    {
      for (DataEvent event : dataEvents)
        if (event.getType() == DataEvent.TYPE_CHANGED)
          renderFromDataItem(event.getDataItem());
    }
    finally
    {
      dataEvents.release();
    }
  }

  private void renderFromDataItems(DataItemBuffer dataItems)
  {
    boolean rendered = false;
    try
    {
      for (DataItem item : dataItems)
        rendered |= renderFromDataItem(item);
    }
    finally
    {
      dataItems.release();
    }

    if (!rendered)
      render(WearNavigationState.normal());
  }

  private boolean renderFromDataItem(DataItem item)
  {
    if (!WearNavigationData.PATH_NAVIGATION_STATE.equals(item.getUri().getPath()))
      return false;

    WearNavigationState state = WearNavigationStateCodec.decode(item.getData());
    if (state != null)
    {
      render(state);
      return true;
    }
    return false;
  }

  private void render(WearNavigationState state)
  {
    boolean navigating = state.getMode() == WearNavigationMode.NAVIGATION;
    mSubtitle.setText(navigating ? R.string.wear_navigation_active_message : R.string.wear_no_navigation_message);
  }
}
