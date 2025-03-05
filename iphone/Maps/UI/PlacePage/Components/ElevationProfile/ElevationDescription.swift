enum ElevationDescription {
  case ascent
  case descent
  case maxElevation
  case minElevation
}

extension ElevationDescription {
  var title: String {
    switch self {
    case .ascent: return L("elevation_profile_ascent")
    case .descent: return L("elevation_profile_descent")
    case .maxElevation: return L("elevation_profile_max_elevation")
    case .minElevation: return L("elevation_profile_min_elevation")
    }
  }

  var imageName: String {
    switch self {
    case .ascent: return "ic_em_ascent_24"
    case .descent: return "ic_em_descent_24"
    case .maxElevation: return "ic_em_max_attitude_24"
    case .minElevation: return "ic_em_min_attitude_24"
    }
  }
}
