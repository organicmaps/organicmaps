extension LocationManager {
  @objc static func speedSymbolFor(_ speed: Double) -> String {
    switch max(speed, 0) {
    case 0 ..< 1: return "🐢"
    case 1 ..< 2: return "🚶"
    case 2 ..< 5: return "🏃"
    case 5 ..< 10: return "🚲"
    case 10 ..< 36: return "🚗"
    case 36 ..< 120: return "🚄"
    case 120 ..< 278: return "🛩"
    default: return "🚀"
    }
  }
}
