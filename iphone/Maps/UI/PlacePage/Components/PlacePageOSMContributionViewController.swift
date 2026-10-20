protocol PlacePageOSMContributionViewControllerDelegate: AnyObject {
  func didPressAddPlace()
  func didPressEditPlace()
  func didPressOSMInfo()
}

final class PlacePageOSMContributionViewController: UIViewController {
  private enum Constants {
    static let horizontalPadding: CGFloat = 16
    static let buttonsHeight: CGFloat = 44
    static let buttonsTopSpacing: CGFloat = 12
    static let buttonsBottomPadding: CGFloat = 16
    static let buttonsSpacing: CGFloat = 12
  }

  private let osmHeaderView = InfoItemView()
  private let addPlaceButton = UIButton(type: .system)
  private let editPlaceButton = UIButton(type: .system)
  private let buttonsStackView = UIStackView()

  var buttonsData: PlacePageOSMContributionData {
    didSet {
      updateButtonsVisibility()
    }
  }

  private weak var delegate: PlacePageOSMContributionViewControllerDelegate?

  init(data: PlacePageOSMContributionData, delegate: PlacePageOSMContributionViewControllerDelegate?) {
    buttonsData = data
    self.delegate = delegate
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
    layoutView()
    updateButtonsVisibility()
  }

  private func setupView() {
    view.setStyle(.background)

    osmHeaderView.setIcon(image: UIImage(resource: .osmLogo))
    osmHeaderView.setTitle(L("contribute_to_osm"))
    osmHeaderView.setAccessory(image: UIImage(resource: .icQuestionmark),
                               tapHandler: { [weak self] in
                                 self?.onOSMInfo()
                               })

    configureButton(addPlaceButton, enabledTitle: L("contribute_to_osm_add_place"), action: #selector(onAddPlace))
    configureButton(editPlaceButton, enabledTitle: L("edit_place"), action: #selector(onEditPlace))

    buttonsStackView.axis = .horizontal
    buttonsStackView.spacing = Constants.buttonsSpacing
    buttonsStackView.distribution = .fillEqually
  }

  private func configureButton(_ button: UIButton,
                               enabledTitle: String,
                               action: Selector) {
    button.setTitle(enabledTitle, for: .normal)
    button.addTarget(self, action: action, for: .touchUpInside)
    button.setStyle(.flatNormalGrayButtonBig)
    button.contentEdgeInsets = UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 8)
    button.titleLabel?.configureSingleLineAutoScaling()
  }

  private func layoutView() {
    view.addSubview(osmHeaderView)
    view.addSubview(buttonsStackView)
    buttonsStackView.addArrangedSubview(addPlaceButton)
    buttonsStackView.addArrangedSubview(editPlaceButton)

    osmHeaderView.translatesAutoresizingMaskIntoConstraints = false
    buttonsStackView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      osmHeaderView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      osmHeaderView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      osmHeaderView.topAnchor.constraint(equalTo: view.topAnchor),

      buttonsStackView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: Constants.horizontalPadding),
      buttonsStackView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -Constants.horizontalPadding),
      buttonsStackView.topAnchor.constraint(equalTo: osmHeaderView.bottomAnchor, constant: Constants.buttonsTopSpacing),
      buttonsStackView.bottomAnchor.constraint(equalTo: view.bottomAnchor, constant: -Constants.buttonsBottomPadding),
      buttonsStackView.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.buttonsHeight),
    ])
  }

  private func updateButtonsVisibility() {
    addPlaceButton.isHidden = !buttonsData.showAddPlace
    editPlaceButton.isHidden = !buttonsData.showEditPlace
    addPlaceButton.isEnabled = buttonsData.canEditPlace
    editPlaceButton.isEnabled = buttonsData.canEditPlace
  }

  // MARK: - Actions

  @objc private func onAddPlace() {
    delegate?.didPressAddPlace()
  }

  @objc private func onEditPlace() {
    delegate?.didPressEditPlace()
  }

  @objc private func onOSMInfo() {
    delegate?.didPressOSMInfo()
  }
}
