// Solution for issue #475
// Generated automatically by Baobot

const OrganicMaps = require('organicmaps'); // Assuming organicmaps is a library for Organic Maps

// Function to handle opening links in the browser
function openLinksInOrganicMaps() {
    const links = document.querySelectorAll('a[href^="https://maps.google.com"]'); // Google Maps
    const osmLinks = document.querySelectorAll('a[href^="https://www.openstreetmap.org"]'); // OSM
    const geoLinks = document.querySelectorAll('a[href^="geo:"]'); // Geo URLs

    links.forEach(link => {
        link.addEventListener('click', (event) => {
            event.preventDefault();
            OrganicMaps.openInApp(link.href);
        });
    });

    osmLinks.forEach(link => {
        link.addEventListener('click', (event) => {
            event.preventDefault();
            OrganicMaps.openInApp(link.href);
        });
    });

    geoLinks.forEach(link => {
        link.addEventListener('click', (event) => {
            event.preventDefault();
            OrganicMaps.openInApp(link.href);
        });
    });
}

// Function to trigger the link handling when the document is ready
function initLinkHandling() {
    document.addEventListener('DOMContentLoaded', openLinksInOrganicMaps);
}

// Call the init function to start
initLinkHandling();


Please note that this script assumes the existence of a library `organicmaps` that provides the `openInApp` method, which is used to open the links within the Organic Maps application. If such a library does not exist, you would need to implement the `openInApp` method or use a different approach to handle the links.