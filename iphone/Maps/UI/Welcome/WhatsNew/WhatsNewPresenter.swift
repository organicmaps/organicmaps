protocol IWhatsNewPresenter: IWelcomePresenter {

}

class WhatsNewPresenter {
  struct WhatsNewConfig: IWelcomeConfig{
    var image: UIImage?
    var title: String
    var text: String
    var buttonNextTitle: String
    var isCloseButtonHidden: Bool
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
    Statistics.logEvent(kStatWhatsNew, withParameters: [kStatAction: kStatOpen,
                                                        kStatVersion: appVersion])
  }

  func onNext() {
    router.onNext()
    Statistics.logEvent(kStatWhatsNew, withParameters: [kStatAction: kStatNext,
                                                        kStatVersion: appVersion])
  }

  func onClose() {
    router.onClose()
    Statistics.logEvent(kStatWhatsNew, withParameters: [kStatAction: kStatClose,
                                                        kStatVersion: appVersion])
  }
}
