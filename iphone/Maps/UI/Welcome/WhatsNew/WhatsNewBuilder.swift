class WhatsNewBuilder {

  static var configs:[WhatsNewPresenter.WhatsNewConfig] {
    return [
      WhatsNewPresenter.WhatsNewConfig(image: UIImage(named: "img_onboarding_outdoor"),
                                       title: "whatsnew_outdoor_guides_title",
                                       text: "whatsnew_outdoor_guides_message",
                                       buttonNextTitle: "new_onboarding_button",
                                       isCloseButtonHidden: true),
      WhatsNewPresenter.WhatsNewConfig(image: UIImage(named: "img_onboarding_gallery"),
                                       title: "whatsnew_guides_galleries_title",
                                       text: "whatsnew_guides_galleries_message",
                                       buttonNextTitle: "done",
                                       isCloseButtonHidden: true)
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
