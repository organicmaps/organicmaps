final class PlacePageExpandableDetailsSectionViewController: UIViewController {
  private let stackView = UIStackView()
  private let headerInfoView = InfoItemView()
  private let textContainerFactory: any ExpandableTextContainerFactory.Type
  private var expandableLabel: ExpandableLabel?
  private var expandableLabelText: ExpandableText?

  private(set) var interactor: any PlacePageExpandableDetailsSectionInteractor

  // MARK: Init

  init(interactor: any PlacePageExpandableDetailsSectionInteractor,
       textContainerFactory: any ExpandableTextContainerFactory.Type) {
    self.interactor = interactor
    self.textContainerFactory = textContainerFactory
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: Lifecycle

  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
    layoutView()
    interactor.handle(.viewDidLoad)
  }

  deinit {
    removeExpandableLabelIfNeeded()
  }

  // MARK: Setup

  private func setupView() {
    view.setStyle(.background)
    stackView.axis = .vertical
    stackView.distribution = .fill
    stackView.spacing = 0
  }

  private func layoutView() {
    view.addSubview(stackView)
    stackView.addArrangedSubview(headerInfoView)

    stackView.translatesAutoresizingMaskIntoConstraints = false
    headerInfoView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      stackView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      stackView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      stackView.topAnchor.constraint(equalTo: view.topAnchor),
      stackView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
    ])
  }

  // MARK: - Public

  func render(_ viewModel: PlacePageExpandableDetailsSectionViewModel) {
    headerInfoView.setTitle(viewModel.title,
                            style: viewModel.style,
                            tapHandler: { [weak self] in
                              self?.interactor.handle(.didTapTitle)
                            },
                            longPressHandler: { [weak self] in
                              self?.interactor.handle(.didLongPressTitle)
                            })
    headerInfoView.setIcon(image: viewModel.icon,
                           tapHandler: { [weak self] in
                             guard let self else { return }
                             self.interactor.handle(.didTapIcon(anchor: self.headerInfoView.iconButton))
                           })
    headerInfoView.setAccessory(image: viewModel.accessory,
                                tapHandler: { [weak self] in
                                  self?.interactor.handle(.didTapAccessory)
                                })

    guard let expandableText = viewModel.expandableText else {
      removeExpandableLabelIfNeeded()
      return
    }

    if let expandableLabel, expandableLabelText?.isSameCase(as: expandableText) == true {
      expandableLabel.text = expandableText.string
    } else {
      removeExpandableLabelIfNeeded()
      let label = ExpandableLabel(expandableText: expandableText, textContainerFactory: textContainerFactory) { [weak self] in
        self?.interactor.handle(.didTapExpandableText)
      }
      expandableLabel = label
      expandableLabelText = expandableText
      stackView.addArrangedSubview(label)
    }

    if let expandableLabel {
      applyExpandedState(viewModel.expandedState, to: expandableLabel)
    }
  }

  private func applyExpandedState(_ expandedState: PlacePageExpandableDetailsSectionViewModel.ExpandedState,
                                  to expandableLabel: ExpandableLabel) {
    switch expandedState {
    case .collapsed:
      expandableLabel.isHidden = false
      expandableLabel.setExpanded(false)
    case .expanded:
      expandableLabel.isHidden = false
      expandableLabel.setExpanded(true)
    }
  }

  private func removeExpandableLabelIfNeeded() {
    guard let expandableLabel else { return }

    stackView.removeArrangedSubview(expandableLabel)
    expandableLabel.removeFromSuperview()
    self.expandableLabel = nil
    expandableLabelText = nil
  }
}
