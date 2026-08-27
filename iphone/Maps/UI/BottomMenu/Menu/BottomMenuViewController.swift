protocol BottomMenuViewProtocol: AnyObject {
  var presenter: BottomMenuPresenterProtocol? { get set }
}

final class BottomMenuViewController: MWMViewController {
  private enum Constants {
    static let estimatedRowHeight: CGFloat = 80
  }

  var presenter: BottomMenuPresenterProtocol?
  private let transitioningManager = BottomMenuTransitioningManager()

  @IBOutlet var tableView: UITableView!
  @IBOutlet var heightConstraint: NSLayoutConstraint!
  @IBOutlet var bottomConstraint: NSLayoutConstraint!

  /// The presentation controller owns the dimming backdrop; this is the bridge
  /// used while the sheet is being dragged.
  private var presentation: BottomMenuPresentationController? {
    presentationController as? BottomMenuPresentationController
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    tableView.layer.setCornerRadius(.modalSheet, maskedCorners: [.layerMinXMinYCorner, .layerMaxXMinYCorner])
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
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    updateTableHeight()
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  @objc private func contentSizeCategoryDidChange() {
    tableView.reloadData()
    view.setNeedsLayout()
  }

  private func updateTableHeight() {
    tableView.setNeedsLayout()
    tableView.layoutIfNeeded()

    let contentHeight = tableView.contentSize.height
    let maximumHeight = view.bounds.height - view.safeAreaInsets.bottom
    guard let targetHeight = BottomMenuSheetLayout.height(contentHeight: contentHeight,
                                                          maximumHeight: maximumHeight) else { return }

    if heightConstraint.constant != targetHeight {
      heightConstraint.constant = targetHeight
    }
    tableView.isScrollEnabled = BottomMenuSheetLayout.isScrollable(contentHeight: contentHeight,
                                                                   sheetHeight: targetHeight)
  }

  @IBAction func onClosePressed(_: Any) {
    presenter?.onClosePressed()
  }

  @IBAction func onPan(_ sender: UIPanGestureRecognizer) {
    let yOffset = sender.translation(in: view.superview).y
    let yVelocity = sender.velocity(in: view.superview).y
    sender.setTranslation(CGPoint.zero, in: view.superview)
    bottomConstraint.constant = min(bottomConstraint.constant - yOffset, 0)

    let alpha = 1.0 - abs(bottomConstraint.constant / tableView.height)
    presentation?.setDimmingAlpha(alpha)

    let state = sender.state
    if state == .ended || state == .cancelled {
      if yVelocity > 0 || (yVelocity == 0 && alpha < 0.8) {
        presenter?.onClosePressed()
      } else {
        let duration = min(AppConstants.defaultAnimationDuration, TimeInterval(bottomConstraint.constant / yVelocity))
        view.layoutIfNeeded()
        UIView.animate(withDuration: duration) {
          self.presentation?.setDimmingAlpha(1)
          self.bottomConstraint.constant = 0
          self.view.layoutIfNeeded()
        }
      }
    }
  }

  override var transitioningDelegate: UIViewControllerTransitioningDelegate? {
    get { transitioningManager }
    set {}
  }

  override var modalPresentationStyle: UIModalPresentationStyle {
    get { .custom }
    set {}
  }
}

extension BottomMenuViewController: BottomMenuViewProtocol {}
