typescript
import { Linking } from 'react-native';

interface MapCoordinates {
  latitude: number;
  longitude: number;
  zoom?: number;
  name?: string;
}

/**
 * Parses a given map URL and extracts coordinates, zoom, and place name.
 * Supports Google Maps, OpenStreetMap, Apple Maps, 2GIS, and Geo URIs.
 * @param url The map URL string.
 * @returns MapCoordinates object or null if parsing fails or no coordinates are found.
 */
function parseMapUrl(url: string): MapCoordinates | null {
  try {
    const urlObj = new URL(url);
    let latitude: number | undefined;
    let longitude: number | undefined;
    let zoom: number | undefined;
    let name: string | undefined;

    // Normalize hostname to handle 'www.' and simplify checks
    const hostname = urlObj.hostname.replace(/^www\./, '');

    // --- Google Maps URLs ---
    if (hostname.includes('google.com/maps') || hostname === 'goo.gl' || hostname === 'maps.app.goo.gl') {
      // 1. Handle '/place/Name/@lat,lon,zoomz' format (most specific for coordinates and name)
      // Example: https://www.google.com/maps/place/Falafel+M.+Sahyoun/@33.8904447,35.5044618,16z
      const placeMatch = urlObj.pathname.match(/@(-?\d+\.?\d*),(-?\d+\.?\d*),(\d+)z/);
      if (placeMatch) {
        latitude = parseFloat(placeMatch[1]);
        longitude = parseFloat(placeMatch[2]);
        zoom = parseInt(placeMatch[3], 10);
        const nameMatch = urlObj.pathname.match(/\/place\/([^/]+)/);
        if (nameMatch && nameMatch[1]) {
          // Decode URL component and replace '+' with space for names
          name = decodeURIComponent(nameMatch[1].replace(/\+/g, ' '));
        }
      }

      // 2. Handle 'q=lat,lon' or 'q=Place Name@lat,lon'
      // Example: https://maps.google.com/maps?z=16&q=Mezza9%401.3067198,103.83282
      // Example: https://maps.google.com/maps?q=55.751809,37.6130029
      // Only parse if coordinates haven't been found by a more precise method like /place/
      if (latitude === undefined && longitude === undefined) {
        const qParam = urlObj.searchParams.get('q');
        if (qParam) {
          const parts = qParam.split('@');
          const lastPart = parts[parts.length - 1];
          // Check if the last part looks like coordinates
          if (lastPart.includes(',') && /^-?\d+\.?\d*,-?\d+\.?\d*$/.test(lastPart)) {
            const coordParts = lastPart.split(',').map(Number);
            if (coordParts.length === 2 && !isNaN(coordParts[0]) && !isNaN(coordParts[1])) {
              latitude = coordParts[0];
              longitude = coordParts[1];
              if (parts.length > 1) { // If there's a name part before coordinates
                name = parts.slice(0, -1).join('@').replace(/\+/g, ' ');
              }
            }
          } else if (!name) { // If 'q' param contains only a name and no name was found previously
            name = qParam.replace(/\+/g, ' ');
          }
        }
      }

      // 3. Handle 'z' parameter for zoom level in search queries (if not already set)
      if (zoom === undefined) {
        const zParam = urlObj.searchParams.get('z');
        if (zParam) {
          zoom = parseInt(zParam, 10);
        }
      }

      // Note: Complex Google Maps URLs like those with /data=!4m2!3m1!1s...
      // are often based on Place IDs and not directly parseable for lat/lon without external APIs.
      // These will naturally fall through and be opened as the original URL if no coordinates are found.
    }
    // --- OpenStreetMap URLs ---
    else if (hostname.includes('openstreetmap.org') || hostname === 'osm.org') {
      // 1. Handle '#map=zoom/lat/lon' hash format
      // Example: https://www.openstreetmap.org/#map=16/33.89041/35.50664
      const hashMatch = urlObj.hash.match(/#map=(\d+)\/(-?\d+\.?\d*)\/(-?\d+\.?\d*)/);
      if (hashMatch) {
        zoom = parseInt(hashMatch[1], 10);
        latitude = parseFloat(hashMatch[2]);
        longitude = parseFloat(hashMatch[3]);
      }

      // 2. Extract name from 'query' search parameter if available
      // Example: https://www.openstreetmap.org/search?query=Falafel%20Sahyoun#map=19/33.89041/35.50665
      if (!name) { // Only set name if not already found (e.g., from a different part of the URL)
        const queryParam = urlObj.searchParams.get('query');
        if (queryParam) {
          name = decodeURIComponent(queryParam.replace(/\+/g, ' '));
        }
      }
      // Note: 'osm.org/go/' links are usually redirects. If the URL object doesn't resolve the redirect
      // to a parsable format (e.g., #map=...), it will fall back to opening the original link.
    }
    // --- Apple Maps URLs ---
    else if (hostname.includes('apple.com/maps')) {
      // Example: https://maps.apple.com/place?address=...&ll=53.991561,-111.289525&q=Example%20Link
      const llParam = urlObj.searchParams.get('ll');
      if (llParam) {
        const coordParts = llParam.split(',').map(Number);
        if (coordParts.length === 2 && !isNaN(coordParts[0]) && !isNaN(coordParts[1])) {
          latitude = coordParts[0];
          longitude = coordParts[1];
        }
      }
      if (!name) {
        const qParam = urlObj.searchParams.get('q');
        if (qParam) {
          name = decodeURIComponent(qParam.replace(/\+/g, ' '));
        }
      }
      // Apple Maps usually doesn't have a direct 'zoom' parameter in the query for place links.
    }
    // --- 2GIS URLs ---
    else if (hostname.includes('2gis.ru') || hostname.includes('go.2gis.com')) {
      // 1. Handle coordinates and zoom from pathname (e.g., /center/lon,lat/zoom/value)
      // Example: https://2gis.ru/moscow/firm/4504127908589159/center/37.6186,55.7601/zoom/15.9764
      const centerMatch = urlObj.pathname.match(/\/center\/(-?\d+\.?\d*),(-?\d+\.?\d*)/);
      if (centerMatch) {
        longitude = parseFloat(centerMatch[1]); // 2GIS often uses lon,lat order in /center/
        latitude = parseFloat(centerMatch[2]);
      }
      const zoomMatch = urlObj.pathname.match(/\/zoom\/(\d+\.?\d*)/);
      if (zoomMatch) {
        zoom = parseInt(zoomMatch[1], 10);
      }

      // 2. Handle coordinates and zoom from 'm' query parameter
      // Example: https://2gis.ru/moscow/firm/4504127908589159?m=37.618632%2C55.760069%2F15.232
      // Only parse if coordinates haven't been found from pathname
      if (latitude === undefined && longitude === undefined) {
        const mParam = urlObj.searchParams.get('m');
        if (mParam) {
          const mParts = mParam.split('/');
          if (mParts.length >= 1) { // Can be just coords or coords/zoom
            const coordParts = mParts[0].split(',').map(Number);
            if (coordParts.length === 2 && !isNaN(coordParts[0]) && !isNaN(coordParts[1])) {
              longitude = coordParts[0]; // 2GIS often uses lon,lat order in 'm' param
              latitude = coordParts[1];
            }
            if (mParts.length >= 2) {
              zoom = parseInt(mParts[1], 10);
            }
          }
        }
      }
      // Name extraction from 2GIS URLs (e.g., firm names) is not straightforward
      // without more specific URL structures or 2GIS API access.
    }
    // --- Geo URIs (RFC 5870) ---
    else if (urlObj.protocol === 'geo:') {
      // Handle 'geo:latitude,longitude[;param=value][?z=zoom]' format
      const geoPath = urlObj.pathname;
      const geoParts = geoPath.split(';')[0].split(','); // Consider only the first part before ';', then split by ','
      if (geoParts.length >= 2) {
        latitude = parseFloat(geoParts[0]);
        longitude = parseFloat(geoParts[1]);
        const zParam = urlObj.searchParams.get('z'); // Check for 'z' in query string
        if (zParam) {
          zoom = parseInt(zParam, 10);
        }
      }
    }

    if (latitude !== undefined && longitude !== undefined) {
      return { latitude, longitude, zoom, name };
    }
    return null;

  } catch (error) {
    // Error parsing URL (e.g., invalid URL format).
    // Suppress errors for production code as per instruction, and fall back to opening the original URL.
    return null;
  }
}

/**
 * Attempts to open a map URL in Organic Maps. If Organic Maps is not installed
 * or the deep link fails, it falls back to opening the original URL, allowing
 * the OS to handle it with other available map applications.
 * @param originalUrl The original map URL to open.
 */
export async function openMapLink(originalUrl: string): Promise<void> {
  const parsedCoords = parseMapUrl(originalUrl);

  // If coordinates are successfully parsed, try to construct and open an Organic Maps URL
  if (parsedCoords && parsedCoords.latitude !== undefined && parsedCoords.longitude !== undefined) {
    let organicMapsUrl = `orgmaps://map?ll=${parsedCoords.latitude},${parsedCoords.longitude}`;
    if (parsedCoords.zoom) {
      organicMapsUrl += `&z=${parsedCoords.zoom}`;
    }
    if (parsedCoords.name) {
      organicMapsUrl += `&q=${encodeURIComponent(parsedCoords.name)}`;
    }

    try {
      const canOpen = await Linking.canOpenURL(organicMapsUrl);
      if (canOpen) {
        await Linking.openURL(organicMapsUrl);
        return; // Successfully opened in Organic Maps
      }
    } catch (error) {
      // Fallback if canOpenURL or openURL fails for organicMapsUrl (e.g., app not installed)
      // The error is intentionally suppressed as we're falling back.
    }
  }

  // Fallback: If Organic Maps failed or no coordinates were parsed, open the original URL
  try {
    const canOpenOriginal = await Linking.canOpenURL(originalUrl);
    if (canOpenOriginal) {
      await Linking.openURL(originalUrl);
    } else {
      // If even the original URL cannot be opened, it indicates a deeper system issue
      // or no apps are registered to handle this URL type.
      // As per "bounty hunter production code" context, suppressing further action is acceptable.
    }
  } catch (error) {
    // Error opening original URL, suppress for bounty hunter production code.
  }
}

/**
 * Helper function to directly construct an Organic Maps deep link URL
 * from a structured MapCoordinates object.
 * @param coords The coordinates and optional name/zoom.
 * @returns The constructed Organic Maps URL string.
 * @throws Error if latitude or longitude are missing.
 */
export function getOrganicMapsUrl(coords: MapCoordinates): string {
  if (coords.latitude === undefined || coords.longitude === undefined) {
    throw new Error('Latitude and longitude are required for constructing an Organic Maps URL.');
  }
  let url = `orgmaps://map?ll=${coords.latitude},${coords.longitude}`;
  if (coords.zoom) {
    url += `&z=${coords.zoom}`;
  }
  if (coords.name) {
    url += `&q=${encodeURIComponent(coords.name)}`;
  }
  return url;
}