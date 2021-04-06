protocol BottomTabBarPresenterProtocol: class {
  func configure()
  func onSearchButtonPressed()
  func onPoint2PointButtonPressed()
  func onDiscoveryButtonPressed()
  func onBookmarksButtonPressed()
  func onMenuButtonPressed()
}

class BottomTabBarPresenter: NSObject {
  private weak var view: BottomTabBarViewProtocol?
  private let interactor: BottomTabBarInteractorProtocol
  
  init(view: BottomTabBarViewProtocol, 
       interactor: BottomTabBarInteractorProtocol) {
    self.view = view
    self.interactor = interactor
  }
}

extension BottomTabBarPresenter: BottomTabBarPresenterProtocol {
  func configure() {
    view?.isLayersBadgeHidden = true
  }

  func onSearchButtonPressed() {
    interactor.openSearch()
  }
  
  func onPoint2PointButtonPressed() {
    interactor.openPoint2Point()
  }
  
  func onDiscoveryButtonPressed() {
    interactor.openDiscovery()
  }
  
  func onBookmarksButtonPressed() {
    interactor.openBookmarks()
  }
  
  func onMenuButtonPressed() {
    interactor.openMenu()
  }
}

