final class PhotosViewController: MWMViewController {
  var referenceViewForPhotoWhenDismissingHandler: ((HotelPhotoUrl) -> UIView?)?

  private let pageViewController = UIPageViewController(transitionStyle: .scroll, navigationOrientation: .horizontal, options: convertToOptionalUIPageViewControllerOptionsKeyDictionary([convertFromUIPageViewControllerOptionsKey(UIPageViewController.OptionsKey.interPageSpacing): 16.0]))
  private(set) var photos: [HotelPhotoUrl]

  fileprivate let transitionAnimator = PhotosTransitionAnimator()
  fileprivate let interactiveAnimator = PhotosInteractionAnimator()
  fileprivate let overlayView = PhotosOverlayView(frame: CGRect.zero)
  private var overlayViewHidden = false

  private lazy var singleTapGestureRecognizer: UITapGestureRecognizer = {
    UITapGestureRecognizer(target: self, action: #selector(handleSingleTapGestureRecognizer(_:)))
  }()

  private lazy var panGestureRecognizer: UIPanGestureRecognizer = {
    UIPanGestureRecognizer(target: self, action: #selector(handlePanGestureRecognizer(_:)))
  }()

  fileprivate var interactiveDismissal = false

  private var currentPhotoViewController: PhotoViewController? {
    return pageViewController.viewControllers?.first as? PhotoViewController
  }

  fileprivate var currentPhoto: HotelPhotoUrl? {
    return currentPhotoViewController?.photo
  }

  init(photos: [HotelPhotoUrl], initialPhoto: HotelPhotoUrl? = nil, referenceView: UIView? = nil) {
    self.photos = photos
    super.init(nibName: nil, bundle: nil)
    initialSetupWithInitialPhoto(initialPhoto)
    transitionAnimator.startingView = referenceView
    transitionAnimator.endingView = currentPhotoViewController?.scalingView.imageView
  }

  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  fileprivate func initialSetupWithInitialPhoto(_ initialPhoto: HotelPhotoUrl? = nil) {
    overlayView.photosViewController = self
    setupPageViewController(initialPhoto: initialPhoto)

    modalPresentationStyle = .custom
    transitioningDelegate = self
    modalPresentationCapturesStatusBarAppearance = true

    overlayView.photosViewController = self
  }

  private func setupPageViewController(initialPhoto: HotelPhotoUrl? = nil) {
    pageViewController.view.backgroundColor = UIColor.clear
    pageViewController.delegate = self
    pageViewController.dataSource = self

    if let photo = initialPhoto {
      let photoViewController = initializePhotoViewController(photo: photo)
      pageViewController.setViewControllers([photoViewController],
                                            direction: .forward,
                                            animated: false,
                                            completion: nil)
    }
    overlayView.photo = initialPhoto
  }

  fileprivate func initializePhotoViewController(photo: HotelPhotoUrl) -> PhotoViewController {
    let photoViewController = PhotoViewController(photo: photo)
    singleTapGestureRecognizer.require(toFail: photoViewController.doubleTapGestureRecognizer)
    return photoViewController
  }

  // MARK: - View Life Cycle
  override func viewDidLoad() {
    super.viewDidLoad()
    view.tintColor = UIColor.white
    view.backgroundColor = UIColor.black
    pageViewController.view.backgroundColor = UIColor.clear

    pageViewController.view.addGestureRecognizer(singleTapGestureRecognizer)
    pageViewController.view.addGestureRecognizer(panGestureRecognizer)

    addChild(pageViewController)
    view.addSubview(pageViewController.view)
    pageViewController.view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    pageViewController.didMove(toParent: self)

    setupOverlayView()
  }

  private func setupOverlayView() {
    overlayView.photo = currentPhoto
    overlayView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    overlayView.frame = view.bounds
    view.addSubview(overlayView)
    setOverlayHidden(false, animated: false)
  }

  private func setOverlayHidden(_ hidden: Bool, animated: Bool) {
    overlayViewHidden = hidden
    overlayView.setHidden(hidden, animated: animated) { [weak self] in
      self?.setNeedsStatusBarAppearanceUpdate()
    }
  }

  override func dismiss(animated flag: Bool, completion: (() -> Void)? = nil) {
    guard presentedViewController == nil else {
      super.dismiss(animated: flag, completion: completion)
      return
    }
    transitionAnimator.startingView = currentPhotoViewController?.scalingView.imageView
    if let currentPhoto = currentPhoto {
      transitionAnimator.endingView = referenceViewForPhotoWhenDismissingHandler?(currentPhoto)
    } else {
      transitionAnimator.endingView = nil
    }
    let overlayWasHiddenBeforeTransition = overlayView.isHidden
    setOverlayHidden(true, animated: true)

    super.dismiss(animated: flag) { [weak self] in
      guard let s = self else { return }
      let isStillOnscreen = s.view.window != nil
      if isStillOnscreen && !overlayWasHiddenBeforeTransition {
        s.setOverlayHidden(false, animated: true)
      }
      completion?()
    }
  }

  // MARK: - Gesture Recognizers
  @objc
  private func handlePanGestureRecognizer(_ gestureRecognizer: UIPanGestureRecognizer) {
    if gestureRecognizer.state == .began {
      interactiveDismissal = true
      dismiss(animated: true, completion: nil)
    } else {
      interactiveDismissal = false
      interactiveAnimator.handlePanWithPanGestureRecognizer(gestureRecognizer, viewToPan: pageViewController.view, anchorPoint: CGPoint(x: view.bounds.midX, y: view.bounds.midY))
    }
  }

  @objc
  private func handleSingleTapGestureRecognizer(_: UITapGestureRecognizer) {
    setOverlayHidden(!overlayView.isHidden, animated: true)
  }

  // MARK: - Orientations
  override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    return .all
  }

  // MARK: - Status Bar
  override var prefersStatusBarHidden: Bool {
    return overlayViewHidden
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    return .lightContent
  }

  override var preferredStatusBarUpdateAnimation: UIStatusBarAnimation {
    return .fade
  }
}

extension PhotosViewController: UIViewControllerTransitioningDelegate {
  func animationController(forPresented _: UIViewController, presenting _: UIViewController, source _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    transitionAnimator.dismissing = false
    return transitionAnimator
  }

  func animationController(forDismissed _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    transitionAnimator.dismissing = true
    return transitionAnimator
  }

  func interactionControllerForDismissal(using _: UIViewControllerAnimatedTransitioning) -> UIViewControllerInteractiveTransitioning? {
    if interactiveDismissal {
      interactiveAnimator.animator = transitionAnimator
      interactiveAnimator.shouldAnimateUsingAnimator = transitionAnimator.endingView != nil
      interactiveAnimator.viewToHideWhenBeginningTransition = overlayView
      return interactiveAnimator
    }
    return nil
  }
}

extension PhotosViewController: UIPageViewControllerDataSource {
  func pageViewController(_: UIPageViewController, viewControllerBefore viewController: UIViewController) -> UIViewController? {
    guard let photoViewController = viewController as? PhotoViewController,
      let photoIndex = photos.firstIndex(where: { $0 === photoViewController.photo }),
      photoIndex - 1 >= 0 else {
      return nil
    }
    let newPhoto = photos[photoIndex - 1]
    return initializePhotoViewController(photo: newPhoto)
  }

  func pageViewController(_: UIPageViewController, viewControllerAfter viewController: UIViewController) -> UIViewController? {
    guard let photoViewController = viewController as? PhotoViewController,
      let photoIndex = photos.firstIndex(where: { $0 === photoViewController.photo }),
      photoIndex + 1 < photos.count else {
      return nil
    }
    let newPhoto = photos[photoIndex + 1]
    return initializePhotoViewController(photo: newPhoto)
  }
}

extension PhotosViewController: UIPageViewControllerDelegate {
  func pageViewController(_: UIPageViewController, didFinishAnimating _: Bool, previousViewControllers _: [UIViewController], transitionCompleted completed: Bool) {
    if completed {
      if let currentPhoto = currentPhoto {
        overlayView.photo = currentPhoto
      }
    }
  }
}

// Helper function inserted by Swift 4.2 migrator.
fileprivate func convertToOptionalUIPageViewControllerOptionsKeyDictionary(_ input: [String: Any]?) -> [UIPageViewController.OptionsKey: Any]? {
	guard let input = input else { return nil }
	return Dictionary(uniqueKeysWithValues: input.map { key, value in (UIPageViewController.OptionsKey(rawValue: key), value)})
}

// Helper function inserted by Swift 4.2 migrator.
fileprivate func convertFromUIPageViewControllerOptionsKey(_ input: UIPageViewController.OptionsKey) -> String {
	return input.rawValue
}
