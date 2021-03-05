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
  }

  func onNext() {
    if let action = config.action {
      action()
    }
    router.onNext()
  }

  func onClose() {
    router.onClose()
  }
}
