@objc(MWMTutorialType)
enum TutorialType: Int {
  case search = 0
  case discovery
  case bookmarks
  case subway
  case isolines
}

@objc(MWMTutorialViewControllerDelegate)
protocol TutorialViewControllerDelegate: AnyObject {
  func didPressTarget(_ viewController: TutorialViewController)
  func didPressCancel(_ viewController: TutorialViewController)
  func didPressOnScreen(_ viewController: TutorialViewController)
}

fileprivate struct TargetAction {
  let target: Any
  let action: Selector
}

@objc(MWMTutorialViewController)
@objcMembers
class TutorialViewController: UIViewController {
  var targetView: UIControl?
  var customAction: (() -> Void)?
  weak var delegate: TutorialViewControllerDelegate?
  private var targetViewActions: [TargetAction] = []

  var tutorialView: TutorialBlurView {
    return view as! TutorialBlurView
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    tutorialView.targetView = targetView
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    overrideTargetAction()
    tutorialView.animateAppearance(kDefaultAnimationDuration)
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    restoreTargetAction()
  }

  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)
    tutorialView.animateSizeChange(coordinator.transitionDuration)
  }

  override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) { }

  override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) { }

  override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
    delegate?.didPressOnScreen(self)
  }

  func fadeOut(withCompletion completion: (() -> Void)?) {
    tutorialView.animateFadeOut(kDefaultAnimationDuration) {
      completion?()
    }
  }

  @objc func onTap(_ sender: UIControl) {
    customAction?()
    delegate?.didPressTarget(self)
  }

  @IBAction func onCancel(_ sender: UIButton) {
    delegate?.didPressCancel(self)
  }

  private func overrideTargetAction() {
    if customAction != nil {
      let targets = targetView?.allTargets
      targets?.forEach({ target in
        let actions = targetView?.actions(forTarget: target, forControlEvent: .touchUpInside)
        actions?.forEach({ action in
          let actionSelector = NSSelectorFromString(action)
          targetViewActions.append(TargetAction(target: target, action: actionSelector))
          targetView?.removeTarget(target, action: actionSelector, for: .touchUpInside)
        })
      })
    }
    targetView?.addTarget(self, action: #selector(onTap(_:)), for: .touchUpInside)
  }

  private func restoreTargetAction() {
    targetView?.removeTarget(self, action: #selector(onTap(_:)), for: .touchUpInside)
    targetViewActions.forEach { targetAction in
      targetView?.addTarget(targetAction.target, action: targetAction.action, for: .touchUpInside)
    }
  }
}

extension TutorialViewController {
  @objc static func tutorial(_ type: TutorialType,
                             target: UIControl,
                             delegate: TutorialViewControllerDelegate) -> TutorialViewController {
    let result: TutorialViewController
    switch type {
    case .search:
      result = searchTutorialBlur()
    case .discovery:
      result = discoveryTutorialBlur()
    case .subway:
      result = subwayTutorialBlur()
    case .isolines:
      result = isolinesTutorialBlur()
    case .bookmarks:
      result = bookmarksTutorialBlur()
    }
    result.targetView = target
    result.delegate = delegate
    return result
  }

  private static func bookmarksTutorial() -> TutorialViewController {
    return TutorialViewController(nibName: "BookmarksTutorial", bundle: nil)
  }

  private static func bookmarksTutorialBlur() -> TutorialViewController {
    let result = TutorialViewController(nibName: "BookmarksTutorialBlur", bundle: nil)
    result.customAction = {
      MapViewController.shared().openCatalog(animated: true, utm: .tipsAndTricks)
    }
    return result
  }

  private static func searchTutorialBlur() -> TutorialViewController {
    let result = TutorialViewController(nibName: "SearchTutorialBlur", bundle: nil)
    result.customAction = {
      MapViewController.shared().searchText(L("hotel").appending(" "))
    }
    return result
  }

  private static func discoveryTutorialBlur() -> TutorialViewController {
    let result = TutorialViewController(nibName: "DiscoveryTutorialBlur", bundle: nil)
    return result
  }

  private static func subwayTutorialBlur() -> TutorialViewController {
    let result = TutorialViewController(nibName: "SubwayTutorialBlur", bundle: nil)
    result.customAction = {
      MapOverlayManager.setTransitEnabled(true)
    }
    return result
  }
  
  private static func isolinesTutorialBlur() -> TutorialViewController {
    let result = TutorialViewController(nibName: "IsolinesTutorialBlur", bundle: nil)
    result.customAction = {
      MapOverlayManager.setIsoLinesEnabled(true)
    }
    return result
  }
}
