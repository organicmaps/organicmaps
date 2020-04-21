@objc class BottomTabBarBuilder: NSObject {
  @objc static func build(mapViewController: MapViewController, controlsManager: MWMMapViewControlsManager) -> BottomTabBarViewController {
    let viewController = BottomTabBarViewController(nibName: nil, bundle: nil)
    let interactor = BottomTabBarInteractor(viewController: viewController,
                                            mapViewController: mapViewController,
                                            controlsManager: controlsManager)
    let presenter = BottomTabBarPresenter(view: viewController, interactor: interactor)
    
    interactor.presenter = presenter
    viewController.presenter = presenter
    
    return viewController
  }
}
