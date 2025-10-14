package app.organicmaps.car.util;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarIcon;
import androidx.car.app.suggestion.SuggestionManager;
import androidx.car.app.suggestion.model.Suggestion;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.search.SearchRecents;
import java.util.ArrayList;
import java.util.List;

public final class SuggestionsHelpers
{
  private final static int MAX_SUGGESTIONS_SIZE = 5;

  public static void updateSuggestions(@NonNull final CarContext context)
  {
    context.getCarService(SuggestionManager.class).updateSuggestions(createSuggestionsList(context));
  }

  // TODO: Currently utilizing search history entries; potential future addition to include "Home" and "Work" marks once
  // supported.
  @NonNull
  private static List<Suggestion> createSuggestionsList(@NonNull final CarContext context)
  {
    final CarIcon iconRecent =
        new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_search_recent)).build();
    final List<Suggestion> suggestions = new ArrayList<>();

    SearchRecents.refresh();
    final int recentsSize = Math.min(SearchRecents.getSize(), MAX_SUGGESTIONS_SIZE);
    for (int i = 0; i < recentsSize; ++i)
    {
      final Suggestion.Builder builder = new Suggestion.Builder();
      builder.setIcon(iconRecent);
      builder.setIdentifier(Integer.toString(i));
      builder.setTitle(SearchRecents.get(i));
      builder.setSubtitle("");
      builder.setAction(IntentUtils.createSearchIntent(context, SearchRecents.get(i)));
      suggestions.add(builder.build());
    }

    return suggestions;
  }

  private SuggestionsHelpers() {}
}
