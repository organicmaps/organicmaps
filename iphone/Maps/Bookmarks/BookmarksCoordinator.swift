import UIKit

@objc class BookmarksCoordinator: NSObject {
  @objc enum BookmarksState: Int {
    case opened
    case closed
    case hidden
  }

  private weak var navigationController: UINavigationController?
  private weak var controlsManager: MWMMapViewControlsManager?
  private weak var navigationManager: MWMNavigationDashboardManager?
  private var bookmarksControllers: [UIViewController]?
  @objc var state: BookmarksState = .closed {
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
      controlsManager?.bookmarksBackButtonHidden = true
    case .closed:
      navigationController.popToRootViewController(animated: true)
      bookmarksControllers = nil
      controlsManager?.bookmarksBackButtonHidden = true
    case .hidden:
      UIView.transition(with: self.navigationController!.view,
                        duration: kDefaultAnimationDuration,
                        options: [.curveEaseInOut, .transitionCrossDissolve],
                        animations: {
                          self.bookmarksControllers = navigationController.popToRootViewController(animated: false)
      }, completion: nil)
      let isNavigation = navigationManager?.state != .hidden
      controlsManager?.bookmarksBackButtonHidden = false || isNavigation
    }
  }
}
