final class TransportTransitStepsView: SolidTouchView {
  private enum Constants {
    static let contentInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
    static let chevronSpacing: CGFloat = 8
    static let chevronWidth: CGFloat = 12
  }

  private var stepsCollectionView: TransportTransitStepsCollectionView!
  private var stepsCollectionViewHeight: NSLayoutConstraint!
  /// The VoiceOver control for opening details; the line badges stay individually focusable.
  private let chevronButton = UIButton(type: .system)
  private var chevronWidth: NSLayoutConstraint!
  private var chevronSpacing: NSLayoutConstraint!
  private lazy var detailsTapRecognizer = UITapGestureRecognizer(target: self, action: #selector(didTap))

  /// Called when the strip is tapped while the route's details can be opened.
  var onTap: (() -> Void)?

  var onContentHeightChanged: (() -> Void)?

  init() {
    super.init(frame: .zero)
    setupViews()
    layout()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupViews() {
    let layout = TransportTransitFlowLayout()
    stepsCollectionView = TransportTransitStepsCollectionView(layout: layout)
    stepsCollectionView.backgroundColor = .clear
    stepsCollectionView.isScrollEnabled = false
    stepsCollectionView.onContentHeightChanged = { [weak self] height in
      guard let self else { return }
      self.stepsCollectionViewHeight.constant = height
      self.onContentHeightChanged?()
    }

    chevronButton.setImage(UIImage(systemName: "chevron.forward"), for: .normal)
    chevronButton.tintColor = .blackSecondaryText
    chevronButton.accessibilityLabel = L("transit_details_title")
    chevronButton.addTarget(self, action: #selector(didTap), for: .touchUpInside)

    // The whole strip opens details too; UIKit keeps this recognizer from also firing for a button tap.
    addGestureRecognizer(detailsTapRecognizer)
  }

  private func layout() {
    addSubview(stepsCollectionView)
    addSubview(chevronButton)
    stepsCollectionView.translatesAutoresizingMaskIntoConstraints = false
    chevronButton.translatesAutoresizingMaskIntoConstraints = false

    stepsCollectionViewHeight = stepsCollectionView.heightAnchor.constraint(equalToConstant: 0)
    chevronWidth = chevronButton.widthAnchor.constraint(equalToConstant: 0)
    chevronSpacing = stepsCollectionView.trailingAnchor.constraint(equalTo: chevronButton.leadingAnchor)

    NSLayoutConstraint.activate([
      stepsCollectionView.topAnchor.constraint(equalTo: topAnchor, constant: Constants.contentInsets.top),
      stepsCollectionView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.contentInsets.left),
      chevronSpacing,
      // A hidden arranged view collapses to zero height, including its insets.
      stepsCollectionView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: Constants.contentInsets.bottom)
        .withPriority(UILayoutPriority(999)),
      stepsCollectionViewHeight,
      chevronButton.trailingAnchor.constraint(equalTo: trailingAnchor, constant: Constants.contentInsets.right),
      chevronButton.centerYAnchor.constraint(equalTo: stepsCollectionView.centerYAnchor),
      chevronWidth,
    ])
  }

  /// - Parameter canOpenDetails: decided by the dashboard from the route these steps were calculated for.
  func setNavigationInfo(_ navigationInfo: MWMNavigationDashboardEntity?, canOpenDetails: Bool = false) {
    setSteps(navigationInfo?.transitSteps ?? [], canOpenDetails: canOpenDetails)
  }

  func setSteps(_ steps: [MWMRouterTransitStepInfo], canOpenDetails: Bool) {
    stepsCollectionView.steps = steps
    isHidden = steps.isEmpty
    detailsTapRecognizer.isEnabled = canOpenDetails
    chevronButton.isHidden = !canOpenDetails
    chevronWidth.constant = canOpenDetails ? Constants.chevronWidth : 0
    chevronSpacing.constant = canOpenDetails ? -Constants.chevronSpacing : 0
  }

  @objc
  private func didTap() {
    onTap?()
  }
}
