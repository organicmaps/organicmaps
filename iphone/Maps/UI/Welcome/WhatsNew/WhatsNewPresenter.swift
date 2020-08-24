protocol IWhatsNewPresenter: IWelcomePresenter {

}

class WhatsNewPresenter {
  struct WhatsNewConfig: IWelcomeConfig{
    var image: UIImage?
    var title: String
    var text: String
    var buttonNextTitle: String = "new_onboarding_button"
    var isCloseButtonHidden: Bool = true
    var action: (()->())? = nil
  }

  private weak var view: IWelcomeView?
  private let router: WelcomeRouter
  private let config: WhatsNewConfig
  private let appVersion = AppInfo.shared().bundleVersion ?? ""

  init(view: IWelcomeView, router: WelcomeRouter, config: WhatsNewConfig) {
    self.view = view
    self.router = router
    self.config = config
  }
}

extension WhatsNewPresenter: IWhatsNewPresenter {
  func configure() {
    view?.configure(config: config)
  }

  func onAppear() {
    Statistics.logEvent(kStatWhatsNewAction, withParameters: [kStatAction: kStatOpen,
                                                              kStatVersion: appVersion])
  }

  func onNext() {
    if let action = config.action {
      action()
    }
    router.onNext()
    Statistics.logEvent(kStatWhatsNewAction, withParameters: [kStatAction: kStatNext,
                                                              kStatVersion: appVersion])
  }

  func onClose() {
    router.onClose()
    Statistics.logEvent(kStatWhatsNewAction, withParameters: [kStatAction: kStatClose,
                                                              kStatVersion: appVersion])
  }
}
