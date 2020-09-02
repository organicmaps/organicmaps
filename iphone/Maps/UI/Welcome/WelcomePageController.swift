@objc(MWMWelcomePageControllerProtocol)
protocol WelcomePageControllerProtocol {
  var view: UIView! { get set }

  func addChildViewController(_ childController: UIViewController)
  func closePageController(_ pageController: WelcomePageController)
}

@objc(MWMWelcomePageController)
final class WelcomePageController: UIPageViewController {

  fileprivate var controllers: [UIViewController] = []
  private var parentController: WelcomePageControllerProtocol!
  private var iPadBackgroundView: SolidTouchView?
  private var isAnimatingTransition = true
  fileprivate var currentController: UIViewController! {
    get {
      return viewControllers?.first
    }
    set {
      guard let controller = newValue, let parentView = parentController.view else { return }
      let animated = !isAnimatingTransition
      parentView.isUserInteractionEnabled = isAnimatingTransition
      setViewControllers([controller], direction: .forward, animated: animated) { [weak self] _ in
        guard let s = self else { return }
        s.isAnimatingTransition = false
        parentView.isUserInteractionEnabled = true
      }
      isAnimatingTransition = animated
    }
  }

  static func shouldShowWelcome() -> Bool {
    return WelcomeStorage.shouldShowTerms ||
      Alohalytics.isFirstSession() ||
      (WelcomeStorage.shouldShowWhatsNew && !DeepLinkHandler.shared.isLaunchedByDeeplink)
  }

  @objc static func controller(parent: WelcomePageControllerProtocol) -> WelcomePageController? {
    guard WelcomePageController.shouldShowWelcome() else {
      return nil
    }

    let vc = WelcomePageController(transitionStyle: .scroll ,
                                   navigationOrientation: .horizontal,
                                   options: convertToOptionalUIPageViewControllerOptionsKeyDictionary([:]))
    vc.parentController = parent

    var controllersToShow: [UIViewController] = []

    if WelcomeStorage.shouldShowTerms {
      controllersToShow.append(TermsOfUseBuilder.build(delegate: vc))
      controllersToShow.append(contentsOf: FirstLaunchBuilder.build(delegate: vc))
    } else {
      if Alohalytics.isFirstSession() {
        WelcomeStorage.shouldShowTerms = true
        controllersToShow.append(TermsOfUseBuilder.build(delegate: vc))
        controllersToShow.append(contentsOf: FirstLaunchBuilder.build(delegate: vc))
      } else {
        NSLog("deeplinking: whats new check")
        if (WelcomeStorage.shouldShowWhatsNew && !DeepLinkHandler.shared.isLaunchedByDeeplink) {
          controllersToShow.append(contentsOf: WhatsNewBuilder.build(delegate: vc))
        }
      }
    }
    
    WelcomeStorage.shouldShowWhatsNew = false
    vc.controllers = controllersToShow
    return vc
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    view.styleName = "Background"
    iPadSpecific {
      let parentView = parentController.view!
      iPadBackgroundView = SolidTouchView(frame: parentView.bounds)
      iPadBackgroundView!.styleName = "FadeBackground"
      iPadBackgroundView!.autoresizingMask = [.flexibleWidth, .flexibleHeight]
      parentView.addSubview(iPadBackgroundView!)
      view.layer.cornerRadius = 5
      view.clipsToBounds = true
    }
    currentController = controllers.first
  }

  func nextPage() {
    currentController = pageViewController(self, viewControllerAfter: currentController)
  }

  func close() {
    iPadBackgroundView?.removeFromSuperview()
    view.removeFromSuperview()
    removeFromParent()
    parentController.closePageController(self)
    FrameworkHelper.processFirstLaunch(LocationManager.lastLocation() != nil)
  }

  @objc func show() {
    parentController.addChildViewController(self)
    parentController.view.addSubview(view)
    updateFrame()
  }

  private func updateFrame() {
    let parentView = parentController.view!
    let size = WelcomeViewController.presentationSize
    view.frame = alternative(iPhone: CGRect(origin: CGPoint(), size: parentView.size),
                             iPad: CGRect(x: parentView.center.x - size.width/2,
                                          y: parentView.center.y - size.height/2,
                                          width: size.width,
                                          height: size.height))
  }

  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)
    coordinator.animate(alongsideTransition: { [weak self] _ in self?.updateFrame() }, completion: nil)
  }
}

extension WelcomePageController: UIPageViewControllerDataSource {

  func pageViewController(_: UIPageViewController, viewControllerBefore viewController: UIViewController) -> UIViewController? {
    guard viewController != controllers.first else { return nil }
    let index = controllers.index(before: controllers.firstIndex(of: viewController)!)
    return controllers[index]
  }

  func pageViewController(_: UIPageViewController, viewControllerAfter viewController: UIViewController) -> UIViewController? {
    guard viewController != controllers.last else { return nil }
    let index = controllers.index(after: controllers.firstIndex(of: viewController)!)
    return controllers[index]
  }

  func presentationCount(for _: UIPageViewController) -> Int {
    return controllers.count
  }

  func presentationIndex(for _: UIPageViewController) -> Int {
    guard let vc = currentController else { return 0 }
    return controllers.firstIndex(of: vc)!
  }
}

extension WelcomePageController: WelcomeViewDelegate {
  func welcomeDidPressNext(_ viewContoller: UIViewController) {
    guard let index = controllers.firstIndex(of: viewContoller) else {
      close()
      return
    }
    if index + 1 < controllers.count {
      nextPage()
    } else {
      if DeepLinkHandler.shared.needExtraWelcomeScreen {
        let vc = DeepLinkInfoBuilder.build(delegate: self)
        controllers.append(vc)
        nextPage()
      } else {
        close()
        DeepLinkHandler.shared.handleDeeplink()
      }
    }
  }

  func welcomeDidPressClose(_ viewContoller: UIViewController) {
    close()
  }
}

extension WelcomePageController: DeeplinkInfoViewControllerDelegate {
  func deeplinkInfoViewControllerDidFinish(_ viewController: UIViewController, deeplink: URL?) {
    close()
    guard let dl = deeplink else { return }
    DeepLinkHandler.shared.handleDeeplink(dl)
  }
}

// Helper function inserted by Swift 4.2 migrator.
fileprivate func convertToOptionalUIPageViewControllerOptionsKeyDictionary(_ input: [String: Any]?) -> [UIPageViewController.OptionsKey: Any]? {
  guard let input = input else { return nil }
  return Dictionary(uniqueKeysWithValues: input.map { key, value in (UIPageViewController.OptionsKey(rawValue: key), value)})
}
