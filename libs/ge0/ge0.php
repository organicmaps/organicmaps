<?php

// @TODO add tests

// Returns array(lat, lon, zoom) or empty array in the case of error
function DecodeGe0LatLonZoom($latLonZoom)
{
  $OM_MAX_POINT_BYTES = 10;
  $OM_MAX_COORD_BITS = $OM_MAX_POINT_BYTES * 3;

  $FAILED = array();

  $base64ReverseArray = GetBase64ReverseArray();

  $zoom = $base64ReverseArray[ord($latLonZoom[0])];
  if ($zoom > 63)
    return $FAILED;
  $zoom = $zoom / 4. + 4.;

  $latLonStr = substr($latLonZoom, 1);
  $latLonBytes = strlen($latLonStr);
  $lat = 0;
  $lon = 0;
  for($i = 0, $shift = $OM_MAX_COORD_BITS - 3; $i < $latLonBytes; $i++, $shift -= 3)
  {
    $a = $base64ReverseArray[ord($latLonStr[$i])];
    $lat1 =  ((($a >> 5) & 1) << 2 |
                 (($a >> 3) & 1) << 1 |
                 (($a >> 1) & 1));
    $lon1 =  ((($a >> 4) & 1) << 2 |
                 (($a >> 2) & 1) << 1 |
                        ($a & 1));
    $lat |= $lat1 << $shift;
    $lon |= $lon1 << $shift;
  }

  $middleOfSquare = 1 << (3 * ($OM_MAX_POINT_BYTES - $latLonBytes) - 1);
  $lat += $middleOfSquare;
  $lon += $middleOfSquare;

  $lat = round($lat / ((1 << $OM_MAX_COORD_BITS) - 1) * 180.0 - 90.0, 5);
  $lon = round($lon / (1 << $OM_MAX_COORD_BITS) * 360.0 - 180.0, 5);

  if ($lat <= -90.0 || $lat >= 90.0)
    return $FAILED;
  if ($lon <= -180.0 || $lon >= 180.0)
    return $FAILED;

  return array($lat, $lon, $zoom);
}

// Returns decoded name
function DecodeGe0Name($name)
{
  return str_replace('_', ' ', rawurldecode($name));
}

// Returns empty array in the case of error.
// In the good case, returns array(lat, lon, zoom) or array(lat, lon, zoom, name)
function DecodeGe0Url($url)
{
  $OM_ZOOM_POSITION = 6;
  $NAME_POSITON_IN_URL = 17;

  $FAILED = array();
  if (strlen($url) < 16 || strpos($url, "ge0://") != 0)
    return $FAILED;

  $base64ReverseArray = GetBase64ReverseArray();

  $latLonZoom = DecodeGe0LatLonZoom(substr($url, 6, 10));
  if (empty($latLonZoom))
    return $FAILED;

  if (strlen($url) < $NAME_POSITON_IN_URL)
    return $latLonZoom;

  $name = DecodeGe0Name(substr($url, $NAME_POSITON_IN_URL));
  array_push($latLonZoom, $name);
  return $latLonZoom;
}

// Internal helper function
function GetBase64ReverseArray()
{
  static $base64Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  $base64ReverseArray = array();
  // fill array with 255's
  array_pad($base64ReverseArray, 256, 255);

  for ($i = 0; $i < 64; $i++)
  {
    $c = $base64Alphabet[$i];
    $base64ReverseArray[ord($c)] = $i;
  }
  return $base64ReverseArray;
}


//////////////////////////////////////
// Tests are below
//print_r(DecodeGe0Url("ge0://B4srhdHVVt/Some_Name"));
//print_r(DecodeGe0Url("ge0://AwAAAAAAAA/%d0%9c%d0%b8%d0%bd%d1%81%d0%ba_%d1%83%d0%bb._%d0%9b%d0%b5%d0%bd%d0%b8%d0%bd%d0%b0_9"));

?>

