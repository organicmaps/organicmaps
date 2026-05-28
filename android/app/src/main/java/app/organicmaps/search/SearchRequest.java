package app.organicmaps.search;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Immutable description of a search to start. Created at an entry point (search toolbar, deep link,
 * intent) and consumed once by the search fragment when it runs the query. The locale and mode only
 * apply to this initial query; manual edits in the search bar fall back to defaults.
 */
public final class SearchRequest
{
  public enum Mode
  {
    /** Open the search sheet with the results list (and the results on the map). */
    SHEET,
    /** Show results on the map only, without opening the sheet (deep-link "search on map"). */
    MAP_ONLY
  }

  /** Initial query; may be null/empty to just open an empty search (e.g. route-point picking). */
  @Nullable
  public final String query;
  /** Input locale for the query, or null to use the keyboard locale. */
  @Nullable
  public final String locale;
  @NonNull
  public final Mode mode;

  public SearchRequest(@Nullable String query, @Nullable String locale, @NonNull Mode mode)
  {
    this.query = query;
    this.locale = locale;
    this.mode = mode;
  }
}
