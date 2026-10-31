package app.organicmaps.consumer;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import android.os.Bundle;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.fragment.app.FragmentActivity;
import app.organicmaps.sdk.MapController;
import app.organicmaps.sdk.MapRenderingListener;
import app.organicmaps.sdk.MapView;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.widgets.lanes.LanesView;
import app.organicmaps.sdk.widgets.speedlimit.SpeedLimitView;

public final class MainActivity extends FragmentActivity
{
  private TextView mStatus;
  private LinearLayout mContent;

  @Override
  public void onCreate(Bundle state)
  {
    super.onCreate(state);
    mStatus = new TextView(this);
    mStatus.setId(android.R.id.text1);
    mStatus.setText("Loading SDK");
    mContent = new LinearLayout(this);
    mContent.setOrientation(LinearLayout.VERTICAL);
    mContent.addView(mStatus, new LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT));
    setContentView(mContent);
    ConsumerApplication app = (ConsumerApplication) getApplication();
    app.getReady().observe(this, ready -> {
      if (Boolean.TRUE.equals(ready))
        showMap(app);
    });
  }

  private void showMap(ConsumerApplication app)
  {
    // Exercise JNI-populated data as well as the native renderer in the minified app.
    CountryItem.fill(MapManager.nativeGetRoot());
    MapView map = new MapView(this);
    MapController controller = new MapController(map, app.getSdk().getLocationHelper(), new MapRenderingListener() {
      @Override
      public void onRenderingInitializationFinished()
      {
        mStatus.setText("SDK ready");
      }
    }, () -> mStatus.setText("Unsupported graphics"), false);
    getLifecycle().addObserver(controller);
    mContent.addView(map, new LinearLayout.LayoutParams(MATCH_PARENT, 0, 1));
    // These types also verify that optional published modules have usable dependency metadata.
    int widgetHeight = (int) (32 * getResources().getDisplayMetrics().density);
    mContent.addView(new LanesView(this, null), new LinearLayout.LayoutParams(MATCH_PARENT, widgetHeight));
    mContent.addView(new SpeedLimitView(this), new LinearLayout.LayoutParams(MATCH_PARENT, widgetHeight));
  }
}
