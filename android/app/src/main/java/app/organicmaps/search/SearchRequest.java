package app.organicmaps.search;

import androidx.annotation.Nullable;

/**
 * Immutable description of a search-sheet open. Created at an entry point (search toolbar, deep
 * link, intent) and consumed once by the search fragment when it runs the initial query. The locale
 * only applies to this initial query; manual edits in the search bar fall back to defaults. Map-only
 * (viewport) deep links bypass this path entirely and call the search engine directly.
 */
public final class SearchRequest
{
  /** Initial query; may be null/empty to just open an empty search (e.g. route-point picking). */
  @Nullable
  public final String query;
  /** Input locale for the query, or null to use the keyboard locale. */
  @Nullable
  public final String locale;

  public SearchRequest(@Nullable String query, @Nullable String locale)
  {
    this.query = query;
    this.locale = locale;
  }
}
