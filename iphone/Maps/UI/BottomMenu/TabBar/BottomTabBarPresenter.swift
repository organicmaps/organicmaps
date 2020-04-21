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
    view?.isLayersBadgeHidden = !MapOverlayManager.guidesFirstLaunch()
  }

  func onSearchButtonPressed() {
    Statistics.logEvent(kStatToolbarClick, withParameters: [kStatButton: kStatSearch])
    interactor.openSearch()
  }
  
  func onPoint2PointButtonPressed() {
    Statistics.logEvent(kStatToolbarClick, withParameters: [kStatButton: kStatPointToPoint])
    interactor.openPoint2Point()
  }
  
  func onDiscoveryButtonPressed() {
    Statistics.logEvent(kStatToolbarClick, withParameters: [kStatButton: kStatDiscovery])
    interactor.openDiscovery()
  }
  
  func onBookmarksButtonPressed() {
    Statistics.logEvent(kStatToolbarClick, withParameters: [kStatButton: kStatDiscovery])
    interactor.openBookmarks()
  }
  
  func onMenuButtonPressed() {
    Statistics.logEvent(kStatToolbarClick, withParameters: [kStatButton : kStatMenu])
    interactor.openMenu()
  }
}

extension BottomTabBarPresenter: MapOverlayManagerObserver {
  func onGuidesStateUpdated() {
    view?.isLayersBadgeHidden = !MapOverlayManager.guidesFirstLaunch()
  }
}

