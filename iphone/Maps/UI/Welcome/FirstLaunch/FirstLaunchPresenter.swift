protocol IFirstLaunchPresenter: IWelcomePresenter {
}

class FirstLaunchPresenter {
  enum Permission {
    case location
    case notifications
    case nothing
  }

  struct FirstLaunchConfig: IWelcomeConfig{
    var image: UIImage?
    var title: String
    var text: String
    var buttonNextTitle: String
    var isCloseButtonHidden: Bool
    let requestPermission: Permission
  }

  private weak var viewController: IWelcomeView?
  private let router: WelcomeRouter
  private let config: FirstLaunchConfig

  init(viewController: IWelcomeView,
       router: WelcomeRouter,
       config: FirstLaunchConfig) {
    self.viewController = viewController
    self.router = router
    self.config = config
  }
}

extension FirstLaunchPresenter: IFirstLaunchPresenter {
  func configure() {
    viewController?.configure(config: config)
  }

  func onAppear() {
    switch config.requestPermission {
    case .location:
      LocationManager.start()
    case .notifications:
      MWMPushNotifications.setup()
    case .nothing:
      break
    }
  }

  func onNext() {
    router.onNext()
  }

  func onClose() {
    router.onClose()
  }
}
