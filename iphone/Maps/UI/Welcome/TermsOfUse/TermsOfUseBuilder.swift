class TermsOfUseBuilder {
  static func build(delegate: WelcomeViewDelegate) -> UIViewController {
    let sb = UIStoryboard.instance(.welcome)
    let vc = sb.instantiateViewController(ofType: TermsOfUseViewController.self);

    let router = WelcomeRouter(viewController: vc, delegate: delegate)
    let presenter = TermsOfUsePresenter(view: vc,
                                        router: router)
    vc.presenter = presenter

    return vc
  }
}
