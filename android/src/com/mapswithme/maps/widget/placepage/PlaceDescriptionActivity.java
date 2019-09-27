package com.mapswithme.maps.widget.placepage;

import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.util.statistics.Statistics;

public class PlaceDescriptionActivity extends BaseToolbarActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return PlaceDescriptionFragment.class;
  }

  public static void start(@NonNull Context context, @NonNull String description,
                           @NonNull String source)
  {
    Intent intent = new Intent(context, PlaceDescriptionActivity.class)
        .putExtra(PlaceDescriptionFragment.EXTRA_DESCRIPTION, description);
    context.startActivity(intent);
    Statistics.ParameterBuilder builder = new Statistics.ParameterBuilder()
        .add(Statistics.EventParam.SOURCE, source);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.PLACEPAGE_DESCRIPTION_MORE, builder.get(),
                                   Statistics.STATISTICS_CHANNEL_REALTIME);
  }
}
