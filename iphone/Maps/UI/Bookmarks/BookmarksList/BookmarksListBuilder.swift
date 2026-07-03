enum BookmarksListBuilder {
  static func build(markGroupId: MWMMarkGroupID,
                    bookmarksCoordinator: BookmarksCoordinator?) -> BookmarksListViewController {
    let viewController = BookmarksListViewController()
    let router = BookmarksListRouter(viewController: viewController,
                                     coordinator: bookmarksCoordinator)
    let interactor = BookmarksListInteractor(markGroupId: markGroupId)
    let presenter = BookmarksListPresenter(view: viewController,
                                           router: router,
                                           interactor: interactor)
    viewController.presenter = presenter
    return viewController
  }
}
