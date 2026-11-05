/**
 * Offline-map catalogue, region hierarchy, and download queue.
 * Catalogue IDs identify nodes independently of localized names. Subscribe before enqueueing and unsubscribe UI
 * callbacks when their host stops; the download queue is independent of those callbacks. Sizes are byte counts.
 */
package app.organicmaps.sdk.downloader;
