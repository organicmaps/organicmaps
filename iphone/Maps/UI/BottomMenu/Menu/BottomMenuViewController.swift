final class BottomMenuViewController: MWMViewController {
  private enum Constants {
    static let estimatedRowHeight: CGFloat = 80
    static let compactSheetWidth: CGFloat = 350
  }

  var presenter: BottomMenuPresenterProtocol?

  private let tableView = UITableView(frame: .zero, style: .plain)

  private var lastDetentHeight: CGFloat?
  private var configuredSheet = false

  override func loadView() {
    super.loadView()
    view.setStyle(.background)
    view.addSubview(tableView)
    tableView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      tableView.topAnchor.constraint(equalTo: view.topAnchor),
      tableView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      tableView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      tableView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
    ])
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    tableView.alwaysBounceVertical = true
    tableView.sectionHeaderHeight = 28
    tableView.sectionFooterHeight = 0
    tableView.rowHeight = UITableView.automaticDimension
    tableView.estimatedRowHeight = Constants.estimatedRowHeight
    tableView.separatorStyle = .none

    NotificationCenter.default.addObserver(self,
                                           selector: #selector(contentSizeCategoryDidChange),
                                           name: UIContentSizeCategory.didChangeNotification,
                                           object: nil)

    tableView.dataSource = presenter
    tableView.delegate = presenter
    tableView.registerNib(cell: BottomMenuItemCell.self)
    tableView.registerNib(cell: BottomMenuLayersCell.self)
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    configureSheet()
    layoutAndUpdateSheet()
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    if let cellToHighlight = presenter?.cellToHighlightIndexPath() {
      tableView.cellForRow(at: cellToHighlight)?.highlight()
    }
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    updateSheetSize()
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  @objc private func contentSizeCategoryDidChange() {
    tableView.reloadData()
    layoutAndUpdateSheet()
  }

  private func layoutAndUpdateSheet() {
    tableView.layoutIfNeeded()
    updateSheetSize()
  }

  private func configureSheet() {
    guard !configuredSheet, let sheet = sheetPresentationController else { return }
    configuredSheet = true
    sheet.delegate = self
    sheet.prefersGrabberVisible = true
    sheet.preferredCornerRadius = CornerRadius.modalSheet.value
    sheet.prefersEdgeAttachedInCompactHeight = true
    sheet.widthFollowsPreferredContentSizeWhenEdgeAttached = true
    // Width applies only while the sheet is edge-attached, i.e. in compact height (iPhone landscape).
    preferredContentSize = CGSize(width: Constants.compactSheetWidth, height: 0)
    // backgroundEffect is only available on iOS 26.1+, hence the divergence
    // from the repo's other iOS 26.0 gates.
    if #available(iOS 26.1, *) {
      sheet.backgroundEffect = UIBlurEffect(style: .systemUltraThinMaterial)
    }
  }

  private func updateSheetSize() {
    guard let sheet = sheetPresentationController, let containerView = sheet.containerView else { return }
    let contentHeight = tableView.contentSize.height
    guard contentHeight > 0 else { return }
    let bottomInset = view.safeAreaInsets.bottom
    let targetHeight = min(contentHeight + bottomInset, containerView.bounds.height)
    guard targetHeight > 0, lastDetentHeight != targetHeight else { return }
    lastDetentHeight = targetHeight

    tableView.isScrollEnabled = contentHeight + bottomInset > targetHeight
    sheet.animateChanges {
      sheet.detents = detents(for: targetHeight)
    }
  }

  // custom(identifier:resolver:) is iOS 16+ and the deployment target is 15.0.
  private func detents(for height: CGFloat) -> [UISheetPresentationController.Detent] {
    if #available(iOS 16.0, *) {
      return [.custom(identifier: nil) { _ in height }]
    }
    return [.large()]
  }

  override var modalPresentationStyle: UIModalPresentationStyle {
    get { .pageSheet }
    set {}
  }
}

extension BottomMenuViewController: UISheetPresentationControllerDelegate {
  func presentationControllerDidDismiss(_: UIPresentationController) {
    // The user swiped the native sheet away; keep the controls manager in sync
    // with the dismissed menu.
    presenter?.onClosePressed()
  }
}
