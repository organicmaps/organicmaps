import UIKit

@objc class BookmarksCoordinator: NSObject {
  enum BookmarksState {
    case opened
    case closed
    case hidden(categoryId: MWMMarkGroupID)
  }

  private weak var navigationController: UINavigationController?
  private var bookmarksControllers: [UIViewController]?
  private var state: BookmarksState = .closed {
    didSet {
      updateForState(newState: state)
    }
  }

  @objc init(navigationController: UINavigationController) {
    self.navigationController = navigationController
  }

  @objc func open() {
    state = .opened
  }

  @objc func close() {
    state = .closed
  }

  @objc func push(_ viewController: UIViewController) {
    navigationController?.pushViewController(viewController, animated: true)
  }

  @objc func pop(_ viewController: UIViewController? = nil) {
    guard let navigationController else { return }
    if let viewController {
      let stack = navigationController.viewControllers
      guard let index = stack.firstIndex(where: { $0 === viewController }) else {
        return
      }
      guard index > 0 else {
        navigationController.popToRootViewController(animated: true)
        return
      }
      let newStack = Array(stack.prefix(index))
      navigationController.setViewControllers(newStack, animated: true)
    } else {
      navigationController.popViewController(animated: true)
    }
  }

  @objc func hide(categoryId: MWMMarkGroupID) {
    state = .hidden(categoryId: categoryId)
  }

  private func updateForState(newState _: BookmarksState) {
    guard let navigationController = navigationController else {
      fatalError()
    }

    switch state {
    case .opened:
      guard let bookmarksControllers = bookmarksControllers else {
        // Instead of BookmarksTabViewController
        let bookmarks = BMCViewController(coordinator: self)
        bookmarks.title = L("bookmarks_and_tracks")
        navigationController.pushViewController(bookmarks, animated: true)
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
      FrameworkHelper.deactivateMapSelection()
      self.bookmarksControllers = nil
    case .closed:
      navigationController.popToRootViewController(animated: true)
      bookmarksControllers = nil
    case .hidden:
      UIView.transition(with: self.navigationController!.view,
                        duration: kDefaultAnimationDuration,
                        options: [.curveEaseInOut, .transitionCrossDissolve],
                        animations: {
                          self.bookmarksControllers = navigationController.popToRootViewController(animated: false)
                        }, completion: nil)
    }
  }
}
