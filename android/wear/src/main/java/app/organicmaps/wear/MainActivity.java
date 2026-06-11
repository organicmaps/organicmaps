package app.organicmaps.wear;

import android.app.Activity;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;

import app.organicmaps.wear.protocol.WearNavigationState;

public final class MainActivity extends Activity
{
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    WearNavigationState state = WearNavigationState.normal();

    LinearLayout layout = new LinearLayout(this);
    layout.setGravity(Gravity.CENTER);
    layout.setOrientation(LinearLayout.VERTICAL);
    int padding = getResources().getDimensionPixelSize(R.dimen.screen_padding);
    layout.setPadding(padding, padding, padding, padding);

    TextView title = new TextView(this);
    title.setGravity(Gravity.CENTER);
    title.setText(R.string.app_name);
    title.setTextSize(18);

    TextView subtitle = new TextView(this);
    subtitle.setGravity(Gravity.CENTER);
    subtitle.setText(R.string.wear_no_navigation_message);
    subtitle.setTextSize(14);

    layout.addView(title);
    layout.addView(subtitle);

    setContentView(layout);
  }
}
