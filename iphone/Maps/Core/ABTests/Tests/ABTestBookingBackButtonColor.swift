@objc class ABTestBookingBackButtonColor: NSObject, IABTest {
  enum ABTestBookingBackButtonColorType: Int {
    case control
    case transparent
    case opaque
    case disabled
  }

  var enabled: Bool = true
  private let storageKey = "\(ABTestBookingBackButtonColor.Type.self)"
  private let distribution = { arc4random() % 3 }

  lazy var type: ABTestBookingBackButtonColorType = {
    guard enabled else {
      return .disabled
    }
    if let stored = UserDefaults.standard.value(forKey: storageKey) as? Int,
      let type = ABTestBookingBackButtonColorType(rawValue: stored) {
      Statistics.logEvent(ABTestInitialized, withParameters: [kStatTestGroup: stat(type)])
      return type
    } else {
      let rawValue = distribution()
      let type = ABTestBookingBackButtonColorType(rawValue: Int(rawValue)) ?? .disabled
      UserDefaults.standard.set(rawValue, forKey: storageKey)
      return type
    }
  }()

  private func stat(_ type: ABTestBookingBackButtonColorType) -> String {
    switch type {
    case .opaque:
      return "opaque"
    case .transparent:
      return "transparent"
    case .control:
      return "transparent_control"
    default:
      return ""
    }
  }

  override required init() {}
}
