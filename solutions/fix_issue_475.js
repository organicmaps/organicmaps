// Solution for issue #475
// Generated automatically by Baobot

const OrganicMaps = require('organicmaps'); // Assuming a hypothetical OrganicMaps module

// Function to open links in the Organic Maps viewer
async function openLinksInViewer(leafletMap) {
    const urlPatterns = [
        'https://www.google.com/maps',
        'https://www.openstreetmap.org',
        'https://geojson.io'
    ];

    leafletMap.eachLayer((layer) => {
        if (layer instanceof L.GeoJSON) {
            layer.on('click', async () => {
                for (const urlPattern of urlPatterns) {
                    const url = new URL(urlPattern);
                    url.searchParams.append('url', layer.feature.url);
                    layer.openPopupWithUrl(url.href);
                }
            });
        }
    });
}

// Example usage
(async () => {
    const organicMap = await OrganicMaps.init();
    const viewer = organicMap.getViewer();
    await openLinksInViewer(viewer.leafletMap);
})();


This script assumes the existence of a `OrganicMaps` module that provides a `Viewer` and `leafletMap` interface. The `openLinksInViewer` function detects GeoJSON layers and opens their associated URLs in Google Maps, OpenStreetMap, and GeoJSON.IO when clicked. Adjustments may be needed based on the actual API provided by the `OrganicMaps` module.