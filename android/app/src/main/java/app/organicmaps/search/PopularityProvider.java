package app.organicmaps.search;

import androidx.annotation.NonNull;

public interface PopularityProvider
{
  @NonNull
  Popularity getPopularity();
}
