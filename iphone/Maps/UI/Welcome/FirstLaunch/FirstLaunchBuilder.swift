class FirstLaunchBuilder {
  static var configs:[FirstLaunchPresenter.FirstLaunchConfig] {
    return [
      FirstLaunchPresenter.FirstLaunchConfig(image: UIImage(named: "img_onboarding_dreamnplan"),
                                             title: "new_onboarding_step2_header",
                                             text: "new_onboarding_step2_message",
                                             buttonNextTitle: "new_onboarding_button",
                                             statType: kStatOnboardingDream,
                                             isCloseButtonHidden: true,
                                             requestPermission: .nothing),
      FirstLaunchPresenter.FirstLaunchConfig(image: UIImage(named: "img_onboarding_offlinemaps"),
                                             title: "new_onboarding_step3_header",
                                             text: "new_onboarding_step3_message",
                                             buttonNextTitle: "new_onboarding_button",
                                             statType: kStatOnboardingExperience,
                                             isCloseButtonHidden: true,
                                             requestPermission: .notifications),
      FirstLaunchPresenter.FirstLaunchConfig(image: UIImage(named: "img_onboarding_sharebookmarks"),
                                             title: "new_onboarding_step4_header",
                                             text: "new_onboarding_step4_message",
                                             buttonNextTitle: "new_onboarding_button_2",
                                             statType: kStatOnboardingShare,
                                             isCloseButtonHidden: true,
                                             requestPermission: .location),
    ]
  }

  static func build(delegate: WelcomeViewDelegate) -> [UIViewController] {
    return FirstLaunchBuilder.configs.map { (config) -> UIViewController in
      let sb = UIStoryboard.instance(.welcome)
      let vc = sb.instantiateViewController(ofType: WelcomeViewController.self);

      let router = WelcomeRouter(viewController: vc,
                                     delegate: delegate)
      let presenter = FirstLaunchPresenter(viewController: vc,
                                              router: router,
                                              config: config)
      vc.presenter = presenter

      return vc
    }
  }
}
