class WhatsNewBuilder {
  static var catalogUrl = URL(string: "https://routes.maps.me/v3/mobilefront/?utm_source=maps.me&utm_medium=whatsnew&utm_campaign=100_minor&utm_content=download_guides_button")
  static var configs:[WhatsNewPresenter.WhatsNewConfig] {
    return [
      WhatsNewPresenter.WhatsNewConfig(image: UIImage(named: "img_onboarding_elevprofile"),
                                       title: "whatsnew_elevation_profile_title",
                                       text: "whatsnew_elevation_profile_message",
                                       buttonNextTitle: "new_onboarding_button"),
      WhatsNewPresenter.WhatsNewConfig(image: UIImage(named: "img_onboarding_newcatalog"),
                                       title: "whatsnew_catalog_new_title",
                                       text: "whatsnew_catalog_new_message",
                                       buttonNextTitle: "download_guides_button",
                                       isCloseButtonHidden: false,
                                       action: { MapViewController.shared()?.openCatalogAbsoluteUrl(catalogUrl, animated: true, utm: .none) }),
      WhatsNewPresenter.WhatsNewConfig(image: UIImage(named: "img_onboarding_newcatalog"),
                                       title: "whatsnew_catalog_new_title",
                                       text: "whatsnew_catalog_new_message",
                                       buttonNextTitle: "done")
    ]
  }

  static func build(delegate: WelcomeViewDelegate) -> [UIViewController] {
    return WhatsNewBuilder.configs.map { (config) -> UIViewController in
      let sb = UIStoryboard.instance(.welcome)
      let vc = sb.instantiateViewController(ofType: WelcomeViewController.self);

      let router = WelcomeRouter(viewController: vc, delegate: delegate)
      let presenter = WhatsNewPresenter(view: vc,
                                        router: router,
                                        config: config)
      vc.presenter = presenter

      return vc
    }
	}
}
