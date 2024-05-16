enum BMCSection {
  case categories
  case actions
  case notifications
}

protocol BMCModel {}

enum BMCAction: BMCModel {
  case create
  case exportAll
  case recentlyDeleted
}

extension BMCAction {
  var title: String {
    switch self {
    case .create:
      return L("bookmarks_create_new_group")
    case .exportAll:
      return L("bookmarks_export")
    case .recentlyDeleted:
      return L("bookmarks_recently_deleted")
    }
  }

  var image: UIImage {
    switch self {
    case .create:
      return UIImage(named: "ic24PxAddCopy")!
    case .exportAll:
      return UIImage(named: "ic24PxShare")!
    case .recentlyDeleted:
      return UIImage(named: "ic_route_manager_trash")!
    }
  }
}

enum BMCNotification: BMCModel {
  case load
}
