typedef NS_OPTIONS(NSInteger, MWMAvailableAreaAffectDirections) {
  MWMAvailableAreaAffectDirectionsNone = 0,
  MWMAvailableAreaAffectDirectionsTop = 1 << 0,
  MWMAvailableAreaAffectDirectionsBottom = 1 << 1,
  MWMAvailableAreaAffectDirectionsLeft = 1 << 2,
  MWMAvailableAreaAffectDirectionsRight = 1 << 3
};
