@objc class BottomMenuBuilder: NSObject {
  @objc static func build(mapViewController: MapViewController, controlsManager: MWMMapViewControlsManager, delegate: BottomMenuDelegate) -> UIViewController {
    let viewController = BottomMenuViewController(nibName: nil, bundle: nil)
    let interactor = BottomMenuInteractor(viewController: viewController,
                                          mapViewController: mapViewController,
                                          controlsManager: controlsManager,
                                          delegate: delegate)
    let presenter = BottomMenuPresenter(view: viewController, interactor: interactor)
    
    interactor.presenter = presenter
    viewController.presenter = presenter
    
    return viewController
  }
}
