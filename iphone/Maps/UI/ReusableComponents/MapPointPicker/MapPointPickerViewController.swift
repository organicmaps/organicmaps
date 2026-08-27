@objcMembers
final class MapPointPickerViewController: UIViewController {
  private enum Constants {
    static let navigationBarHeight: CGFloat = 44
    static let buttonHorizontalInset: CGFloat = 16
    static let hintInsets = UIEdgeInsets(top: 12, left: 20, bottom: 12, right: 20)
  }

  var doneHandler: ((CGPoint) -> Void)?
  var cancelHandler: (() -> Void)?

  private let pickerTitle: String
  private let hint: String
  private let enableBounds: Bool
  private let initialPosition: NSValue?

  private let headerView = UIView()
  private let navigationBarView = UIView()
  private let titleLabel = UILabel()
  private let hintLabel = UILabel()
  private let cancelButton = UIButton(type: .system)
  private let doneButton = UIButton(type: .system)
  private var isChoosePositionModeActive = false

  init(title: String, hint: String, enableBounds: Bool, initialPosition: NSValue?) {
    pickerTitle = title
    self.hint = hint
    self.enableBounds = enableBounds
    self.initialPosition = initialPosition
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func loadView() {
    view = TouchTransparentView()
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupViews()
    layoutViews()
  }

  deinit {
    stopChoosePositionMode()
  }

  func present(in parentViewController: UIViewController) {
    parentViewController.addChild(self)
    view.frame = parentViewController.view.bounds
    view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    parentViewController.view.addSubview(view)
    didMove(toParent: parentViewController)

    startChoosePositionMode()
    view.layoutIfNeeded()
    headerView.transform = CGAffineTransform(translationX: 0, y: -headerView.bounds.height)
    UIView.animate(withDuration: kDefaultAnimationDuration) {
      self.headerView.transform = .identity
    }
    parentViewController.setNeedsStatusBarAppearanceUpdate()
  }

  private func setupViews() {
    headerView.setStyleAndApply(.menuBackground)
    navigationBarView.setStyleAndApply(.primaryBackground)

    titleLabel.text = pickerTitle
    titleLabel.textAlignment = .center
    titleLabel.adjustsFontForContentSizeCategory = true
    titleLabel.setStyleAndApply(.navigationBarItem)

    hintLabel.text = hint
    hintLabel.textAlignment = .center
    hintLabel.numberOfLines = 0
    hintLabel.adjustsFontForContentSizeCategory = true
    hintLabel.setFontStyleAndApply(.regular12, color: .blackSecondary)

    configure(button: cancelButton, title: L("cancel"), action: #selector(cancelButtonDidTap))
    configure(button: doneButton, title: L("done"), action: #selector(doneButtonDidTap))
  }

  private func configure(button: UIButton, title: String, action: Selector) {
    button.setTitle(title, for: .normal)
    button.setStyleAndApply(.navigationBarItem)
    button.titleLabel?.adjustsFontForContentSizeCategory = true
    button.addTarget(self, action: action, for: .touchUpInside)
    button.accessibilityLabel = title
  }

  private func layoutViews() {
    view.addSubview(headerView)
    headerView.addSubview(navigationBarView)
    headerView.addSubview(hintLabel)
    navigationBarView.addSubview(cancelButton)
    navigationBarView.addSubview(titleLabel)
    navigationBarView.addSubview(doneButton)

    for item in [headerView, navigationBarView, hintLabel, cancelButton, titleLabel, doneButton] {
      item.translatesAutoresizingMaskIntoConstraints = false
    }

    NSLayoutConstraint.activate([
      headerView.topAnchor.constraint(equalTo: view.topAnchor),
      headerView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      headerView.trailingAnchor.constraint(equalTo: view.trailingAnchor),

      navigationBarView.topAnchor.constraint(equalTo: headerView.topAnchor),
      navigationBarView.leadingAnchor.constraint(equalTo: headerView.leadingAnchor),
      navigationBarView.trailingAnchor.constraint(equalTo: headerView.trailingAnchor),
      navigationBarView.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor,
                                                constant: Constants.navigationBarHeight),

      cancelButton.leadingAnchor.constraint(equalTo: navigationBarView.leadingAnchor,
                                            constant: Constants.buttonHorizontalInset),
      cancelButton.bottomAnchor.constraint(equalTo: navigationBarView.bottomAnchor),
      cancelButton.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.navigationBarHeight),

      doneButton.trailingAnchor.constraint(equalTo: navigationBarView.trailingAnchor,
                                           constant: -Constants.buttonHorizontalInset),
      doneButton.bottomAnchor.constraint(equalTo: navigationBarView.bottomAnchor),
      doneButton.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.navigationBarHeight),

      titleLabel.centerXAnchor.constraint(equalTo: navigationBarView.centerXAnchor),
      titleLabel.centerYAnchor.constraint(equalTo: cancelButton.centerYAnchor),
      titleLabel.leadingAnchor.constraint(greaterThanOrEqualTo: cancelButton.trailingAnchor, constant: 8),
      titleLabel.trailingAnchor.constraint(lessThanOrEqualTo: doneButton.leadingAnchor, constant: -8),

      hintLabel.topAnchor.constraint(equalTo: navigationBarView.bottomAnchor, constant: Constants.hintInsets.top),
      hintLabel.leadingAnchor.constraint(equalTo: headerView.leadingAnchor, constant: Constants.hintInsets.left),
      hintLabel.trailingAnchor.constraint(equalTo: headerView.trailingAnchor, constant: -Constants.hintInsets.right),
      hintLabel.bottomAnchor.constraint(equalTo: headerView.bottomAnchor, constant: -Constants.hintInsets.bottom),
    ])
  }

  private func startChoosePositionMode() {
    guard !isChoosePositionModeActive else { return }
    FrameworkHelper.startChoosePositionMode(withEnableBounds: enableBounds, initialPosition: initialPosition)
    isChoosePositionModeActive = true
  }

  private func stopChoosePositionMode() {
    guard isChoosePositionModeActive, !FrameworkHelper.isFrameworkDestroyed() else { return }
    FrameworkHelper.stopChoosePositionMode()
    isChoosePositionModeActive = false
  }

  private func dismiss(completion: @escaping () -> Void) {
    stopChoosePositionMode()
    let parentViewController = parent
    willMove(toParent: nil)
    UIView.animate(withDuration: kDefaultAnimationDuration,
                   animations: {
                     self.headerView.transform = CGAffineTransform(translationX: 0, y: -self.headerView.bounds.height)
                   },
                   completion: { _ in
                     self.view.removeFromSuperview()
                     self.removeFromParent()
                     parentViewController?.setNeedsStatusBarAppearanceUpdate()
                     completion()
                   })
  }

  @objc private func doneButtonDidTap() {
    let point = FrameworkHelper.viewportCenter()
    dismiss { [doneHandler] in doneHandler?(point) }
  }

  @objc private func cancelButtonDidTap() {
    dismiss { [cancelHandler] in cancelHandler?() }
  }
}
