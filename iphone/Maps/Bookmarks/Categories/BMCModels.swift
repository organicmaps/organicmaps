enum BMCSection {
  case categories
  case actions
  case recentlyDeleted
  case notifications
}

protocol BMCModel {}

enum BMCAction: BMCModel {
  case create
  case exportAll
  case `import`
  case recentlyDeleted(Int)
}

extension BMCAction {
  var title: String {
    switch self {
    case .create:
      return L("bookmarks_create_new_group")
    case .exportAll:
      return L("bookmarks_export")
    case .import:
      return L("bookmarks_import")
    case .recentlyDeleted(let count):
      return L("bookmarks_recently_deleted") + " (\(count))"
    }
  }

  var image: UIImage {
    switch self {
    case .create:
      return UIImage(named: "ic24PxAddCopy")!
    case .exportAll:
      return UIImage(named: "ic24PxShare")!
    case .import:
      return UIImage(named: "ic24PxImport")!
    case .recentlyDeleted:
      return UIImage(named: "ic_route_manager_trash_open")!
    }
  }
}

enum BMCNotification: BMCModel {
  case load
}
