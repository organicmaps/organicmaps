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

  @objc static func controller(parent: WelcomePageControllerProtocol) -> WelcomePageController? {
    if (!WelcomeViewController.shouldShowWelcome) { return nil }
    guard let welcomeControllers = WelcomeViewController.controllers(firstSession: Alohalytics.isFirstSession())
      else { return nil }
    
    let vc = WelcomePageController(transitionStyle: welcomeControllers.count > 1 ? .scroll : .pageCurl,
                                   navigationOrientation: .horizontal,
                                   options: [:])
    vc.parentController = parent
    vc.dataSource = vc
    welcomeControllers.forEach { (controller) in
      controller.delegate = vc
    }
    vc.controllers = welcomeControllers
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

  func nextPage() {
    currentController = pageViewController(self, viewControllerAfter: currentController)
    if let controller = currentController as? WelcomeViewController {
      Statistics.logEvent(kStatEventName(kStatWhatsNew, type(of: controller).key),
                          withParameters: [kStatAction: kStatNext])
    }
  }

  func close() {
    WelcomeViewController.shouldShowWelcome = false
    if let controller = currentController as? WelcomeViewController {
      Statistics.logEvent(kStatEventName(kStatWhatsNew, type(of: controller).key),
                        withParameters: [kStatAction: kStatClose])
    }
    iPadBackgroundView?.removeFromSuperview()
    view.removeFromSuperview()
    removeFromParentViewController()
    parentController.closePageController(self)
  }

  func show() {
    if let controller = currentController as? WelcomeViewController {
      Statistics.logEvent(kStatEventName(kStatWhatsNew, type(of: controller).key),
                          withParameters: [kStatAction: kStatOpen])
    }
    parentController.addChildViewController(self)
    parentController.view.addSubview(view)
    updateFrame()
  }

  private func updateFrame() {
    let parentView = parentController.view!
    view.frame = alternative(iPhone: CGRect(origin: CGPoint(), size: parentView.size),
                             iPad: CGRect(x: parentView.center.x - 260, y: parentView.center.y - 300, width: 520, height: 600))
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

extension WelcomePageController: WelcomeViewControllerDelegate {
  func viewSize() -> CGSize {
    return view.size
  }
  
  func welcomeViewControllerDidPressNext(_ viewContoller: WelcomeViewController) {
    guard let index = controllers.index(of: viewContoller) else {
      close()
      return
    }
    if index + 1 < controllers.count {
      nextPage()
    } else {
      close()
    }
  }
  
  func welcomeViewControllerDidPressClose(_ viewContoller: WelcomeViewController) {
    close()
  }
}
