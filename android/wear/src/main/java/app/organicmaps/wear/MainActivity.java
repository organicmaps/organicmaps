package app.organicmaps.wear;
import android.app.Activity;
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
import com.google.android.gms.wearable.Wearable;

public final class MainActivity extends Activity implements DataClient.OnDataChangedListener
{
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
    render(WearNavigationState.normal());
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    // Phase 1 keeps Wear sync foreground-only: read the latest state on start and listen while visible.
    DataClient dataClient = Wearable.getDataClient(this);
    dataClient.addListener(this);
    dataClient.getDataItems().addOnSuccessListener(this::updateFromDataItems);
  }

  @Override
  protected void onStop()
  {
    Wearable.getDataClient(this).removeListener(this);
    super.onStop();
  }

  @Override
  public void onDataChanged(DataEventBuffer dataEvents)
  {
    try
    {
      for (DataEvent event : dataEvents)
      {
        if (event.getType() != DataEvent.TYPE_CHANGED)
          continue;

        updateFromDataItem(event.getDataItem());
      }
    }
    finally
    {
      dataEvents.release();
    }
  }

  private void updateFromDataItems(DataItemBuffer dataItems)
  {
    try
    {
      for (DataItem item : dataItems)
        updateFromDataItem(item);
    }
    finally
    {
      dataItems.release();
    }
  }

  private void updateFromDataItem(DataItem item)
  {
    if (!WearNavigationData.PATH_NAVIGATION_STATE.equals(item.getUri().getPath()))
      return;

    WearNavigationState state = WearNavigationStateCodec.decode(item.getData());
    if (state != null)
      render(state);
  }

  private void render(WearNavigationState state)
  {
    mSubtitle.setText(getMessageResId(state));
  }

  private int getMessageResId(WearNavigationState state)
  {
    if (state.getMode() == WearNavigationMode.NAVIGATION)
      return R.string.wear_navigation_active_message;
    return R.string.wear_no_navigation_message;
  }
}
