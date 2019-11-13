class DeepLinkInfoBuilder {
  static func build(delegate: DeeplinkInfoViewControllerDelegate) -> UIViewController {
    let sb = UIStoryboard.instance(.welcome)
    let vc = sb.instantiateViewController(ofType: WelcomeViewController.self);

    let deeplinkUrl = DeepLinkHandler.shared.deeplinkURL?.url
    let router = DeepLinkInfoRouter(viewController: vc,
                                    delegate: delegate,
                                    deeplink: deeplinkUrl)
    let presenter = DeepLinkInfoPresenter(view: vc,
                                          router: router,
                                          deeplink: deeplinkUrl)
    vc.presenter = presenter

    return vc
  }
}
