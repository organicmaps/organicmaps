@objc class BottomMenuBuilder: NSObject {
  @objc static func buildMenu(mapViewController: MapViewController,
                              controlsManager: MWMMapViewControlsManager,
                              delegate: BottomMenuDelegate) -> UIViewController {
    return BottomMenuBuilder.build(mapViewController: mapViewController,
                                   controlsManager: controlsManager,
                                   delegate: delegate,
                                   sections: [.layers, .items],
                                   source: kStatMenu)
  }

  @objc static func buildLayers(mapViewController: MapViewController,
                                controlsManager: MWMMapViewControlsManager,
                                delegate: BottomMenuDelegate) -> UIViewController {
    return BottomMenuBuilder.build(mapViewController: mapViewController,
                                   controlsManager: controlsManager,
                                   delegate: delegate,
                                   sections: [.layers],
                                   source: kStatMap)
  }

  private static func build(mapViewController: MapViewController,
                            controlsManager: MWMMapViewControlsManager,
                            delegate: BottomMenuDelegate,
                            sections: [BottomMenuPresenter.Sections],
                            source: String) -> UIViewController {
    let viewController = BottomMenuViewController(nibName: nil, bundle: nil)
    let interactor = BottomMenuInteractor(viewController: viewController,
                                          mapViewController: mapViewController,
                                          controlsManager: controlsManager,
                                          delegate: delegate)
    let presenter = BottomMenuPresenter(view: viewController, interactor: interactor, sections: sections, source: source)
    
    interactor.presenter = presenter
    viewController.presenter = presenter
    
    return viewController
  }
}
