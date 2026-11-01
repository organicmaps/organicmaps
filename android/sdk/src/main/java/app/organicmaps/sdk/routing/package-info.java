/**
 * Route planning, navigation coordination, and native guidance data.
 * {@link app.organicmaps.sdk.routing.RoutingController} owns the process-wide route and accepts one UI container.
 * Route builds are asynchronous and can report missing region maps. Hosts own permissions, location updates,
 * foreground-service policy, and lifecycle-aware UI presentation.
 */
package app.organicmaps.sdk.routing;
