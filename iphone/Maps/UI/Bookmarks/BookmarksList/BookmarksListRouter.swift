final class BookmarksListRouter {
  private weak var viewController: UIViewController?
  private weak var coordinator: BookmarksCoordinator?

  init(viewController: UIViewController, coordinator: BookmarksCoordinator?) {
    self.viewController = viewController
    self.coordinator = coordinator
  }
}

extension BookmarksListRouter: IBookmarksListRouter {
  func listSettings(_ bookmarkGroup: BookmarkGroup, delegate: CategorySettingsViewControllerDelegate?) {
    let listSettingsController = CategorySettingsViewController(bookmarkGroup: bookmarkGroup)
    listSettingsController.delegate = delegate
    coordinator?.push(listSettingsController)
  }

  func viewOnMap(_ bookmarkGroup: BookmarkGroup) {
    coordinator?.hide(categoryId: bookmarkGroup.categoryId)
  }

  func showDescription(_ bookmarkGroup: BookmarkGroup) {
    let description = BookmarksListRouter.prepareHtmlDescription(bookmarkGroup)
    let descriptionViewController = WebViewController(html: description, baseUrl: nil, title: bookmarkGroup.title)!
    descriptionViewController.openInSafari = true
    coordinator?.push(descriptionViewController)
  }

  private static func prepareHtmlDescription(_ bookmarkGroup: BookmarkGroup) -> String {
    var description = bookmarkGroup.detailedAnnotation
    if bookmarkGroup.isHtmlDescription {
      if !description.contains("<body>") {
        description = "<body>" + description
      }
    } else {
      description = description.replacingOccurrences(of: "\n", with: "<br>")
      let header = """
      <head>
        <style>
          body {
            line-height: 1.4;
            margin-right: 16px;
            margin-left: 16px;
          }
        </style>
      </head>
      <body>
      """
      description = header + description
    }
    if !description.contains("</body>") {
      description += "</body>"
    }
    return description
  }

  func showSubgroup(_ subgroupId: MWMMarkGroupID) {
    let bookmarksListViewController = BookmarksListBuilder.build(markGroupId: subgroupId,
                                                                 bookmarksCoordinator: coordinator)
    coordinator?.push(bookmarksListViewController)
  }

  func selectGroup(currentGroupId groupId: MWMMarkGroupID,
                   delegate: SelectBookmarkGroupViewControllerDelegate?) {
    let groupViewController = SelectBookmarkGroupViewController(groupId: groupId)
    groupViewController.delegate = delegate
    let navigationController = UINavigationController(rootViewController: groupViewController)
    viewController?.present(navigationController, animated: true, completion: nil)
  }

  func editBookmark(bookmarkId: MWMMarkID, completion: @escaping (Bool) -> Void) {
    let editBookmarkController = UIStoryboard.instance(.main).instantiateViewController(withIdentifier: "MWMEditBookmarkController") as! EditBookmarkViewController
    editBookmarkController.configure(with: bookmarkId, editCompletion: completion)
    coordinator?.push(editBookmarkController)
  }

  func editTrack(trackId: MWMTrackID, completion: @escaping (Bool) -> Void) {
    let editTrackController = EditTrackViewController(trackId: trackId, editCompletion: completion)
    coordinator?.push(editTrackController)
  }

  func goBack() {
    if let viewController {
      coordinator?.pop(viewController)
    }
  }
}
