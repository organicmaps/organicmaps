typedef NS_ENUM(NSUInteger, MWMRouterResultCode) {
  MWMRouterResultCodeNoError = 0,
  MWMRouterResultCodeCancelled = 1,
  MWMRouterResultCodeNoCurrentPosition = 2,
  MWMRouterResultCodeInconsistentMWMandRoute = 3,
  MWMRouterResultCodeRouteFileNotExist = 4,
  MWMRouterResultCodeStartPointNotFound = 5,
  MWMRouterResultCodeEndPointNotFound = 6,
  MWMRouterResultCodePointsInDifferentMWM = 7,
  MWMRouterResultCodeRouteNotFound = 8,
  MWMRouterResultCodeNeedMoreMaps = 9,
  MWMRouterResultCodeInternalError = 10,
  MWMRouterResultCodeFileTooOld = 11,
  MWMRouterResultCodeIntermediatePointNotFound = 12,
  MWMRouterResultCodeTransitRouteNotFoundNoNetwork = 13,
  MWMRouterResultCodeTransitRouteNotFoundTooLongPedestrian = 14,
  MWMRouterResultCodeRouteNotFoundRedressRouteError = 15,
  MWMRouterResultCodeHasWarnings = 16
} NS_SWIFT_NAME(RouterResultCode);
