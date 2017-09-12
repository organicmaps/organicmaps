import UIKit

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

  static func controller(parent: WelcomePageControllerProtocol) -> WelcomePageController? {
    let isFirstSession = Alohalytics.isFirstSession()
    let welcomeKey = isFirstSession ? FirstLaunchController.key : WhatsNewController.key
    guard UserDefaults.standard.bool(forKey: welcomeKey) == false else { return nil }

    let pagesCount = isFirstSession ? FirstLaunchController.pagesCount : WhatsNewController.pagesCount
    let id = pagesCount == 1 ? "WelcomePageCurlController" : "WelcomePageScrollController"
    let sb = UIStoryboard.instance(.welcome)
    let vc = sb.instantiateViewController(withIdentifier: id) as! WelcomePageController
    vc.config(parent)
    vc.show()
    return vc
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    view.backgroundColor = UIColor.white()
    iPadSpecific {
      let parentView = parentController.view!
      iPadBackgroundView = SolidTouchView(frame: parentView.bounds)
      iPadBackgroundView!.backgroundColor = UIColor.fadeBackground()
      iPadBackgroundView!.autoresizingMask = [.flexibleWidth, .flexibleHeight]
      parentView.addSubview(iPadBackgroundView!)
      view.layer.cornerRadius = 5
      view.clipsToBounds = true
    }
    currentController = controllers.first
  }

  private func config(_ parent: WelcomePageControllerProtocol) {
    parentController = parent
    let isFirstSession = Alohalytics.isFirstSession()
    let pagesCount = isFirstSession ? FirstLaunchController.pagesCount : WhatsNewController.pagesCount
    let welcomeClass: WelcomeProtocolBase.Type = isFirstSession ? FirstLaunchController.self : WhatsNewController.self
    (0 ..< pagesCount).forEach {
      let vc = welcomeClass.controller($0)
      (vc as! WelcomeProtocolBase).pageController = self
      controllers.append(vc)
    }
    dataSource = self
  }

  func nextPage() {
    let welcomeKey = Alohalytics.isFirstSession() ? FirstLaunchController.key : WhatsNewController.key
    Statistics.logEvent(kStatEventName(kStatWhatsNew, welcomeKey),
                        withParameters: [kStatAction: kStatNext])
    currentController = pageViewController(self, viewControllerAfter: currentController)
  }

  func close() {
    UserDefaults.standard.set(true, forKey: FirstLaunchController.key)
    UserDefaults.standard.set(true, forKey: WhatsNewController.key)
    let welcomeKey = Alohalytics.isFirstSession() ? FirstLaunchController.key : WhatsNewController.key
    Statistics.logEvent(kStatEventName(kStatWhatsNew, welcomeKey),
                        withParameters: [kStatAction: kStatClose])
    iPadBackgroundView?.removeFromSuperview()
    view.removeFromSuperview()
    removeFromParentViewController()
    parentController.closePageController(self)
  }

  func show() {
    let welcomeKey = Alohalytics.isFirstSession() ? FirstLaunchController.key : WhatsNewController.key
    Statistics.logEvent(kStatEventName(kStatWhatsNew, welcomeKey),
                        withParameters: [kStatAction: kStatOpen])
    parentController.addChildViewController(self)
    parentController.view.addSubview(view)
    updateFrame()
  }

  private func updateFrame() {
    let parentView = parentController.view!
    view.frame = alternative(iPhone: CGRect(origin: CGPoint(), size: parentView.size),
                             iPad: CGRect(x: parentView.center.x - 260, y: parentView.center.y - 300, width: 520, height: 600))
    (currentController as! WelcomeProtocolBase).updateSize()
  }

  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)
    coordinator.animate(alongsideTransition: { [weak self] _ in self?.updateFrame() }, completion: nil)
  }
}

extension WelcomePageController: UIPageViewControllerDataSource {

  func pageViewController(_: UIPageViewController, viewControllerBefore viewController: UIViewController) -> UIViewController? {
    guard viewController != controllers.first else { return nil }
    let index = controllers.index(before: controllers.index(of: viewController)!)
    return controllers[index]
  }

  func pageViewController(_: UIPageViewController, viewControllerAfter viewController: UIViewController) -> UIViewController? {
    guard viewController != controllers.last else { return nil }
    let index = controllers.index(after: controllers.index(of: viewController)!)
    return controllers[index]
  }

  func presentationCount(for _: UIPageViewController) -> Int {
    return controllers.count
  }

  func presentationIndex(for _: UIPageViewController) -> Int {
    guard let vc = currentController else { return 0 }
    return controllers.index(of: vc)!
  }
}
