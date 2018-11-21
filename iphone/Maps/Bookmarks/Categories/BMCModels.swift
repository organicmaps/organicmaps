enum BMCSection {
  case permissions
  case categories
  case actions
  case notifications
}

protocol BMCModel {}

enum BMCPermission: BMCModel {
  case signup
  case backup
  case restore(Date?)
}

@objc
protocol BMCCategoryObserver {
  func categoryUpdated()
}

class BMCCategory: BMCModel, Equatable {
  let identifier: MWMMarkGroupID
  var title: String {
    didSet {
      notifyObservers()
    }
  }

  var count: UInt64 {
    didSet {
      notifyObservers()
    }
  }

  var isVisible: Bool {
    didSet {
      notifyObservers()
    }
  }

  var accessStatus: MWMCategoryAccessStatus {
    didSet {
      notifyObservers()
    }
  }

  init(identifier: MWMMarkGroupID = 0,
       title: String = L("core_my_places"),
       count: UInt64 = 0,
       isVisible: Bool = true,
       accessStatus: MWMCategoryAccessStatus = .local) {
    self.identifier = identifier
    self.title = title
    self.count = count
    self.isVisible = isVisible
    self.accessStatus = accessStatus
  }

  private let observers = NSHashTable<BMCCategoryObserver>.weakObjects()

  func addObserver(_ observer: BMCCategoryObserver) {
    observers.add(observer)
  }

  func removeObserver(_ observer: BMCCategoryObserver) {
    observers.remove(observer)
  }

  private func notifyObservers() {
    observers.allObjects.forEach { $0.categoryUpdated() }
  }

  static func ==(lhs: BMCCategory, rhs: BMCCategory) -> Bool {
    return lhs.identifier == rhs.identifier && lhs.title == rhs.title && lhs.count == rhs.count && lhs.isVisible == rhs.isVisible
  }
}

enum BMCAction: BMCModel {
  case create
}

enum BMCNotification: BMCModel {
  case load
}
