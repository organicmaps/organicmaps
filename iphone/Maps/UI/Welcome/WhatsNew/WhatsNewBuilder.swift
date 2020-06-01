class WhatsNewBuilder {
  static var configs:[WhatsNewPresenter.WhatsNewConfig] {
    return [
      WhatsNewPresenter.WhatsNewConfig(image: UIImage(named: "img_wnew_newguides"),
                                       title: "whatsnew_guides_on_map_title",
                                       text: "whatsnew_guides_on_map_message",
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
