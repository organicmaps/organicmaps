enum BMCSection {
  case categories
  case actions
  case notifications
}

protocol BMCModel {}

enum BMCAction: BMCModel {
  case create
  case exportAll
}

extension BMCAction {
  var title: String {
    switch self {
    case .create:
      return L("bookmarks_create_new_group")
    case .exportAll:
      return L("bookmarks_export")
    }
  }

  var image: UIImage {
    switch self {
    case .create:
      return UIImage(named: "ic24PxAddCopy")!
    case .exportAll:
      return UIImage(named: "ic24PxShare")!
    }
  }
}

enum BMCNotification: BMCModel {
  case load
}
