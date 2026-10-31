extension MWMRouterTransitType {
  /// The line badge icon; nil for steps that are not transit rides.
  var icon: UIImage? {
    switch self {
    case .intermediatePoint, .pedestrian, .ruler: nil
    case .train: UIImage.ic20PxRoutePlanningTrain
    case .subway: UIImage.ic20PxRoutePlanningMetro
    case .lightRail: UIImage.ic20PxRoutePlanningLightrail
    case .monorail: UIImage.ic20PxRoutePlanningMonorail
    case .tram, .cableTram: UIImage.ic20PxRoutePlanningTram
    case .bus: UIImage.ic20PxRoutePlanningBus
    case .ferry, .waterService: UIImage.ic20PxRoutePlanningFerry
    case .aerialLift: UIImage.ic20PxRoutePlanningAerialLift
    case .funicular: UIImage.ic20PxRoutePlanningFunicular
    case .airService: UIImage.ic20PxRoutePlanningAirService
    }
  }

  /// The transport type read by VoiceOver before a line number; nil when there is no fitting name.
  var spokenName: String? {
    switch self {
    case .intermediatePoint, .pedestrian, .ruler, .airService: nil
    case .subway: L("button_layer_subway")
    case .train: L("transit_type_train")
    case .tram, .cableTram: L("transit_type_tram")
    case .bus: L("transit_type_bus")
    // The map feature type names already cover these transport types.
    case .lightRail: Self.typeName("type.railway.light_rail")
    case .monorail: Self.typeName("type.railway.monorail")
    case .funicular: Self.typeName("type.railway.funicular")
    case .aerialLift: Self.typeName("type.aerialway")
    case .ferry, .waterService: Self.typeName("type.route.ferry")
    }
  }

  private static func typeName(_ key: String) -> String {
    NSLocalizedString(key, tableName: "LocalizableTypes", comment: "")
  }
}
