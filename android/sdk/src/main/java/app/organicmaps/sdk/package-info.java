/**
 * Embedded offline-map framework and rendering entry points.
 * Create one {@link app.organicmaps.sdk.OrganicMaps} per process and wait for its asynchronous map-loading callback
 * before using feature APIs. Bind each activity's {@link app.organicmaps.sdk.MapView} to a
 * {@link app.organicmaps.sdk.MapController} and its lifecycle. Native state is shared across activity recreation.
 */
package app.organicmaps.sdk;
