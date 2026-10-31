/**
 * Asynchronous offline POI, bookmark, and downloadable-region search.
 * Requests carry host-supplied timestamps; filter old callbacks and replace result snapshots rather than appending
 * them. Preserve native result indexes when filtering a display list. Detailed POI search requires region maps.
 */
package app.organicmaps.sdk.search;
