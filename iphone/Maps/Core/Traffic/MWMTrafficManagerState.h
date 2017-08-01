typedef NS_ENUM(NSUInteger, MWMTrafficManagerState) {
  MWMTrafficManagerStateDisabled,
  MWMTrafficManagerStateEnabled,
  MWMTrafficManagerStateWaitingData,
  MWMTrafficManagerStateOutdated,
  MWMTrafficManagerStateNoData,
  MWMTrafficManagerStateNetworkError,
  MWMTrafficManagerStateExpiredData,
  MWMTrafficManagerStateExpiredApp
};
