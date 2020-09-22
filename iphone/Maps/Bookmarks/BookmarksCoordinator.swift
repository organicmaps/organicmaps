import UIKit

@objc class BookmarksCoordinator: NSObject {
  enum BookmarksState {
    case opened
    case closed
    case hidden(categoryId: MWMMarkGroupID)
  }

  private weak var navigationController: UINavigationController?
  private weak var controlsManager: MWMMapViewControlsManager?
  private weak var navigationManager: MWMNavigationDashboardManager?
  private var bookmarksControllers: [UIViewController]?
  private var state: BookmarksState = .closed {
    didSet {
      updateForState(newState: state)
    }
  }

  @objc init(navigationController: UINavigationController,
             controlsManager: MWMMapViewControlsManager,
             navigationManager: MWMNavigationDashboardManager) {
    self.navigationController = navigationController
    self.controlsManager = controlsManager
    self.navigationManager = navigationManager
  }

  @objc func open() {
    state = .opened
  }

  @objc func close() {
    state = .closed
  }

  @objc func hide(categoryId: MWMMarkGroupID) {
    state = .hidden(categoryId: categoryId)
  }

  private func updateForState(newState: BookmarksState) {
    guard let navigationController = navigationController else {
      fatalError()
    }

    switch state {
    case .opened:
      guard let bookmarksControllers = bookmarksControllers else {
        navigationController.pushViewController(BookmarksTabViewController(coordinator: self),
                                                animated: true)
        return
      }
      var controllers = navigationController.viewControllers
      controllers.append(contentsOf: bookmarksControllers)
      UIView.transition(with: self.navigationController!.view,
                        duration: kDefaultAnimationDuration,
                        options: [.curveEaseInOut, .transitionCrossDissolve],
                        animations: {
                          navigationController.setViewControllers(controllers, animated: false)
      }, completion: nil)
      FrameworkHelper.deactivateMapSelection(notifyUI: true)
      self.bookmarksControllers = nil
      controlsManager?.hideGuidesNavigationBar()
    case .closed:
      navigationController.popToRootViewController(animated: true)
      bookmarksControllers = nil
      controlsManager?.hideGuidesNavigationBar()
    case let .hidden(categoryId):
      UIView.transition(with: self.navigationController!.view,
                        duration: kDefaultAnimationDuration,
                        options: [.curveEaseInOut, .transitionCrossDissolve],
                        animations: {
                          self.bookmarksControllers = navigationController.popToRootViewController(animated: false)
      }, completion: nil)
      let isNavigation = navigationManager?.state != .hidden
      if isNavigation == false {
        controlsManager?.showGuidesNavigationBar(categoryId)
      }
    }
  }
}
