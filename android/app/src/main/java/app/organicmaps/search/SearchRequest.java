package app.organicmaps.search;

import androidx.annotation.Nullable;

/**
 * Immutable description of a search-sheet open. Created at an entry point (search toolbar, deep
 * link, intent, process-death restore) and consumed once by the search fragment when it runs the
 * initial query. The locale and isCategory flag only apply to this initial query; manual edits in
 * the search bar fall back to defaults. Map-only (viewport) deep links bypass this path entirely
 * and call the search engine directly.
 */
public final class SearchRequest
{
  /** Initial query; may be null/empty to just open an empty search (e.g. route-point picking). */
  @Nullable
  public final String query;
  /** Input locale for the query, or null to use the keyboard locale. */
  @Nullable
  public final String locale;
  /** True when the query was triggered by a category chip — preserves categorial engine ranking. */
  public final boolean isCategory;

  public SearchRequest(@Nullable String query, @Nullable String locale)
  {
    this(query, locale, false);
  }

  public SearchRequest(@Nullable String query, @Nullable String locale, boolean isCategory)
  {
    this.query = query;
    this.locale = locale;
    this.isCategory = isCategory;
  }
}
