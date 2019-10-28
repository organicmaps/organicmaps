class WhatsNewBuilder {

  static var configs:[WhatsNewPresenter.WhatsNewConfig] {
    return [
      WhatsNewPresenter.WhatsNewConfig(image: UIImage(named: "img_whats_new_mapboost"),
                                             title: "whatsnew_map_updates_title",
                                             text: "whatsnew_map_updates_message",
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
