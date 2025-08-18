final class TrackRecordingButtonViewController: MWMViewController {

  private enum Constants {
    static let buttonDiameter = CGFloat(48)
    static let topOffset = CGFloat(6)
    static let trailingOffset = CGFloat(10)
    static let blinkingDuration = 1.0
    static let color: (lighter: UIColor, darker: UIColor) = (.red, .red.darker(percent: 0.3))
  }

  private let trackRecordingManager: TrackRecordingManager = .shared
  private let button = BottomTabBarButton()
  private var blinkingTimer: Timer?
  private var topConstraint = NSLayoutConstraint()
  private var trailingConstraint = NSLayoutConstraint()
  private var state: TrackRecordingButtonState = .hidden

  private static var availableArea: CGRect = .zero
  private static var topConstraintValue: CGFloat {
    availableArea.origin.y + Constants.topOffset
  }
  private static var trailingConstraintValue: CGFloat {
    -(UIScreen.main.bounds.maxX - availableArea.maxX + Constants.trailingOffset)
  }

  @objc
  init() {
    super.init(nibName: nil, bundle: nil)
    let ownerViewController = MapViewController.shared()
    ownerViewController?.addChild(self)
    ownerViewController?.controlsView.addSubview(view)
    self.setupView()
    self.layout()
    self.startTimer()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setState(self.state, completion: nil)
  }

  // MARK: - Public methods

  @objc
  func setState(_ state: TrackRecordingButtonState, completion: (() -> Void)?) {
    self.state = state
    switch state {
    case .visible:
      setHidden(false, completion: nil)
    case .hidden:
      setHidden(true, completion: completion)
    case .closed:
      close(completion: completion)
    @unknown default:
      fatalError()
    }
  }

  // MARK: - Private methods

  private func setupView() {
    view.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(button)

    button.setStyleAndApply(.trackRecordingWidgetButton)
    button.tintColor = Constants.color.darker
    button.translatesAutoresizingMaskIntoConstraints = false
    button.setImage(UIImage(resource: .icMenuBookmarkTrackRecording), for: .normal)
    button.addTarget(self, action: #selector(didTap), for: .touchUpInside)
    button.isHidden = true
  }

  private func layout() {
    guard let superview = view.superview else { return }
    topConstraint = view.topAnchor.constraint(equalTo: superview.topAnchor, constant: Self.topConstraintValue)
    trailingConstraint = view.trailingAnchor.constraint(equalTo: superview.trailingAnchor, constant: Self.trailingConstraintValue)
    NSLayoutConstraint.activate([
      topConstraint,
      trailingConstraint,
      view.widthAnchor.constraint(equalToConstant: Constants.buttonDiameter),
      view.heightAnchor.constraint(equalToConstant: Constants.buttonDiameter),

      button.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      button.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      button.topAnchor.constraint(equalTo: view.topAnchor),
      button.bottomAnchor.constraint(equalTo: view.bottomAnchor),
    ])
  }

  private func updateLayout() {
    guard let superview = view.superview else { return }
    superview.animateConstraints {
      self.topConstraint.constant = Self.topConstraintValue
      self.trailingConstraint.constant = Self.trailingConstraintValue
    }
  }

  private func startTimer() {
    guard blinkingTimer == nil else { return }
    var lighter = false
    let timer = Timer.scheduledTimer(withTimeInterval: Constants.blinkingDuration, repeats: true) { [weak self] _ in
      guard let self = self else { return }
      UIView.animate(withDuration: Constants.blinkingDuration, animations: {
        self.button.tintColor = lighter ? Constants.color.lighter : Constants.color.darker
        lighter.toggle()
      })
    }
    blinkingTimer = timer
    RunLoop.current.add(timer, forMode: .common)
  }

  private func stopTimer() {
    blinkingTimer?.invalidate()
    blinkingTimer = nil
  }

  private func setHidden(_ hidden: Bool, completion: (() -> Void)?) {
    UIView.transition(with: self.view,
                      duration: kDefaultAnimationDuration,
                      options: .transitionCrossDissolve,
                      animations: {
      self.button.isHidden = hidden
    }) { _ in
      completion?()
    }
  }

  private func close(completion: (() -> Void)?) {
    stopTimer()
    setHidden(true) { [weak self] in
      guard let self else { return }
      self.removeFromParent()
      self.view.removeFromSuperview()
      completion?()
    }
  }

  static func updateAvailableArea(_ frame: CGRect) {
    availableArea = frame
    guard let button = MapViewController.shared()?.controlsManager.trackRecordingButton else { return }
    DispatchQueue.main.async {
      button.updateLayout()
    }
  }

  // MARK: - Actions

  @objc
  private func didTap(_ sender: Any) {
    MapViewController.shared()?.showTrackRecordingPlacePage()
  }
}
