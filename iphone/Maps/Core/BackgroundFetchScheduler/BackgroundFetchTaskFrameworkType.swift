@objc enum BackgroundFetchTaskFrameworkType: Int {
  case none
  case full

  func create() {
    switch self {
    case .none: return
    case .full: MWMFrameworkHelper.createFramework()
    }
  }
}

extension BackgroundFetchTaskFrameworkType: Equatable {
  static func ==(lhs: BackgroundFetchTaskFrameworkType, rhs: BackgroundFetchTaskFrameworkType) -> Bool {
    return lhs.rawValue == rhs.rawValue
  }
}

extension BackgroundFetchTaskFrameworkType: Comparable {
  static func <(lhs: BackgroundFetchTaskFrameworkType, rhs: BackgroundFetchTaskFrameworkType) -> Bool {
    return lhs.rawValue < rhs.rawValue
  }
}
