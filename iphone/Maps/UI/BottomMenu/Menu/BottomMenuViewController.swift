protocol BottomMenuViewProtocol: AnyObject {
  var presenter: BottomMenuPresenterProtocol? { get set }
}

final class BottomMenuViewController: MWMViewController {
  private enum Constants {
    static let estimatedRowHeight: CGFloat = 80
    static let sheetCornerRadius: CGFloat = 20
  }

  var presenter: BottomMenuPresenterProtocol?

  @IBOutlet var tableView: UITableView!

  private var lastDetentHeight: CGFloat?

  override func viewDidLoad() {
    super.viewDidLoad()

    tableView.sectionFooterHeight = 0
    tableView.rowHeight = UITableView.automaticDimension
    tableView.estimatedRowHeight = Constants.estimatedRowHeight

    NotificationCenter.default.addObserver(self,
                                           selector: #selector(contentSizeCategoryDidChange),
                                           name: UIContentSizeCategory.didChangeNotification,
                                           object: nil)

    tableView.dataSource = presenter
    tableView.delegate = presenter
    tableView.registerNib(cell: BottomMenuItemCell.self)
    tableView.registerNib(cell: BottomMenuLayersCell.self)
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    if let cellToHighlight = presenter?.cellToHighlightIndexPath() {
      tableView.cellForRow(at: cellToHighlight)?.highlight()
    }
    layoutAndUpdateSheet()
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

  private func updateSheetSize() {
    let maximumHeight = UIScreen.main.bounds.height * 0.7
    guard let targetHeight = BottomMenuSheetLayout.height(contentHeight: tableView.contentSize.height,
                                                          maximumHeight: maximumHeight) else { return }
    guard lastDetentHeight != targetHeight else { return }
    lastDetentHeight = targetHeight
    guard let sheet = sheetPresentationController else { return }
    sheet.delegate = self

    if #available(iOS 16.0, *) {
      sheet.detents = [.custom(identifier: nil) { _ in targetHeight }]
    } else {
      sheet.detents = [.large()]
    }
    sheet.prefersGrabberVisible = true
    sheet.preferredCornerRadius = Constants.sheetCornerRadius
    if #available(iOS 26.1, *) {
      sheet.backgroundEffect = UIBlurEffect(style: .systemUltraThinMaterial)
    }
  }

  override var modalPresentationStyle: UIModalPresentationStyle {
    get { .pageSheet }
    set {}
  }
}

extension BottomMenuViewController: BottomMenuViewProtocol, UISheetPresentationControllerDelegate {
  func presentationControllerDidDismiss(_: UIPresentationController) {
    // The user swiped the native sheet away; keep the controls manager in sync
    // with the dismissed menu.
    presenter?.onClosePressed()
  }
}
