@objc enum BackgroundFetchTaskFrameworkType: Int {
  case none
  case full

  func create() {
    switch self {
    case .none: return
    case .full: FrameworkHelper.createFramework()
    }
  }
}

extension BackgroundFetchTaskFrameworkType: Equatable {
  static func == (lhs: BackgroundFetchTaskFrameworkType, rhs: BackgroundFetchTaskFrameworkType) -> Bool {
    lhs.rawValue == rhs.rawValue
  }
}

extension BackgroundFetchTaskFrameworkType: Comparable {
  static func < (lhs: BackgroundFetchTaskFrameworkType, rhs: BackgroundFetchTaskFrameworkType) -> Bool {
    lhs.rawValue < rhs.rawValue
  }
}
