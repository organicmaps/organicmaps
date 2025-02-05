protocol SearchHistoryViewControllerDelegate: SearchOnMapScrollViewDelegate {
  func searchHistoryViewController(_ viewController: SearchHistoryViewController,
                                   didSelect query: String)
}

final class SearchHistoryViewController: MWMViewController {
  private weak var delegate: SearchHistoryViewControllerDelegate?
  private var lastQueries: [String] = []
  private let frameworkHelper: MWMSearchFrameworkHelper
  private static let clearCellIdentifier = "SearchHistoryViewController_clearCellIdentifier"
  private let emptyHistoryView = PlaceholderView(title: L("search_history_title"),
                                                 subtitle: L("search_history_text"))

  @IBOutlet var tableView: UITableView!

  init(frameworkHelper: MWMSearchFrameworkHelper, delegate: SearchHistoryViewControllerDelegate?) {
    self.delegate = delegate
    self.frameworkHelper = frameworkHelper
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupTableView()
    setupNoResultsView()
  }

  private func setupTableView() {
    tableView.setStyle(.background)
    tableView.registerNib(cellClass: SearchHistoryQueryCell.self)
    let nib = UINib(nibName: "SearchHistoryClearCell", bundle: nil)
    tableView.register(nib, forCellReuseIdentifier: SearchHistoryViewController.clearCellIdentifier)
    tableView.keyboardDismissMode = .onDrag
  }

  private func setupNoResultsView() {
    view.addSubview(emptyHistoryView)
    emptyHistoryView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      emptyHistoryView.topAnchor.constraint(equalTo: view.topAnchor),
      emptyHistoryView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      emptyHistoryView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      emptyHistoryView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    reload(animated: false)
  }

  func reload(animated: Bool = true) {
    guard isViewLoaded else { return }
    lastQueries = frameworkHelper.lastSearchQueries()
    showEmptyHistoryView(lastQueries.isEmpty ? true : false)
    tableView.reloadData()
  }

  private func showEmptyHistoryView(_ isVisible: Bool = true, animated: Bool = true) {
    UIView.transition(with: emptyHistoryView,
                      duration: animated ? kDefaultAnimationDuration : 0,
                      options: [.transitionCrossDissolve, .curveEaseInOut]) {
      self.emptyHistoryView.alpha = isVisible ? 1.0 : 0.0
      self.emptyHistoryView.isHidden = !isVisible
    }
  }
  
  private func clearSearchHistory() {
    frameworkHelper.clearSearchHistory()
    reload()
  }
}

extension SearchHistoryViewController: UITableViewDataSource {
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return lastQueries.isEmpty ? 0 : lastQueries.count + 1
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if indexPath.row == lastQueries.count {
      let cell = tableView.dequeueReusableCell(withIdentifier: SearchHistoryViewController.clearCellIdentifier,
                                               for: indexPath)
      return cell
    }
    
    let cell = tableView.dequeueReusableCell(cell: SearchHistoryQueryCell.self, indexPath: indexPath)
    cell.update(with: lastQueries[indexPath.row])
    return cell
  }
}

extension SearchHistoryViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    if indexPath.row == lastQueries.count {
      clearSearchHistory()
    } else {
      let query = lastQueries[indexPath.row]
      delegate?.searchHistoryViewController(self, didSelect: query)
    }
    tableView.deselectRow(at: indexPath, animated: true)
  }

  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    delegate?.scrollViewDidScroll(scrollView)
  }
}

extension SearchHistoryViewController: ModallyPresentedViewController {
  func translationYDidUpdate(_ translationY: CGFloat) {
    guard isViewLoaded else { return }
    tableView.contentInset.bottom = translationY
    emptyHistoryView.translationYDidUpdate(translationY)
  }
}
