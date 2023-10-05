protocol BottomTabBarPresenterProtocol: AnyObject {
  func configure()
  func onSearchButtonPressed()
  func onHelpButtonPressed()
  func onBookmarksButtonPressed()
  func onMenuButtonPressed()
}

class BottomTabBarPresenter: NSObject {
  private let interactor: BottomTabBarInteractorProtocol
  
  init(interactor: BottomTabBarInteractorProtocol) {
    self.interactor = interactor
  }
}

extension BottomTabBarPresenter: BottomTabBarPresenterProtocol {
  func configure() {
  }

  func onSearchButtonPressed() {
    interactor.openSearch()
  }
  
  func onHelpButtonPressed() {
    interactor.openHelp()
  }
  
  func onBookmarksButtonPressed() {
    interactor.openBookmarks()
  }
  
  func onMenuButtonPressed() {
    interactor.openMenu()
  }
}

