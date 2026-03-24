final class PlacePageExpandableDetailsSectionViewController: UIViewController {
  private let stackView = UIStackView()
  private let headerInfoView = InfoItemView()
  private let expandableLabel = ExpandableLabel()

  private(set) var interactor: any PlacePageExpandableDetailsSectionInteractor

  // MARK: Init

  init(interactor: any PlacePageExpandableDetailsSectionInteractor) {
    self.interactor = interactor
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

  // MARK: Setup

  private func setupView() {
    view.setStyle(.background)
    stackView.axis = .vertical
    stackView.distribution = .fill
    stackView.spacing = 0

    expandableLabel.isHidden = true
    expandableLabel.didTap = { [weak self] in
      self?.interactor.handle(.didTapExpandableText)
    }
  }

  private func layoutView() {
    view.addSubview(stackView)
    stackView.addArrangedSubview(headerInfoView)
    stackView.addArrangedSubview(expandableLabel)

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
                             self?.interactor.handle(.didTapIcon)
                           })
    headerInfoView.setAccessory(image: viewModel.accessory,
                                tapHandler: { [weak self] in
                                  self?.interactor.handle(.didTapAccessory)
                                })
    expandableLabel.expandableText = viewModel.expandableText

    switch viewModel.expandedState {
    case .collapsed:
      expandableLabel.isHidden = false
      expandableLabel.setExpanded(false)
    case .expanded:
      expandableLabel.isHidden = false
      expandableLabel.setExpanded(true)
    case .hidden:
      expandableLabel.isHidden = true
      expandableLabel.setExpanded(false)
    }
  }
}
