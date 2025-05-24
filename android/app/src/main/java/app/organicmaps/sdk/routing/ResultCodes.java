package app.organicmaps.sdk.routing;

public interface ResultCodes
{
  // Codes correspond to native routing::RouterResultCode in routing/routing_callbacks.hpp
  int NO_ERROR = 0;
  int CANCELLED = 1;
  int NO_POSITION = 2;
  int INCONSISTENT_MWM_ROUTE = 3;
  int ROUTING_FILE_NOT_EXIST = 4;
  int START_POINT_NOT_FOUND = 5;
  int END_POINT_NOT_FOUND = 6;
  int DIFFERENT_MWM = 7;
  int ROUTE_NOT_FOUND = 8;
  int NEED_MORE_MAPS = 9;
  int INTERNAL_ERROR = 10;
  int FILE_TOO_OLD = 11;
  int INTERMEDIATE_POINT_NOT_FOUND = 12;
  int TRANSIT_ROUTE_NOT_FOUND_NO_NETWORK = 13;
  int TRANSIT_ROUTE_NOT_FOUND_TOO_LONG_PEDESTRIAN = 14;
  int ROUTE_NOT_FOUND_REDRESS_ROUTE_ERROR = 15;
  int HAS_WARNINGS = 16;
}
