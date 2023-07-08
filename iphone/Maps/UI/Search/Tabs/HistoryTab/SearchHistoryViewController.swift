protocol SearchHistoryViewControllerDelegate: AnyObject {
  func searchHistoryViewController(_ viewController: SearchHistoryViewController,
                                   didSelect query: String)
}

final class SearchHistoryViewController: MWMViewController {
  private weak var delegate: SearchHistoryViewControllerDelegate?
  private var lastQueries: [String]
  private let frameworkHelper: MWMSearchFrameworkHelper
  private static let clearCellIdentifier = "SearchHistoryViewController_clearCellIdentifier"
  
  @IBOutlet private var tableView: UITableView!
  @IBOutlet private weak var noResultsViewContainer: UIView!
  
  init(frameworkHelper: MWMSearchFrameworkHelper, delegate: SearchHistoryViewControllerDelegate?) {
    self.delegate = delegate
    self.lastQueries = frameworkHelper.lastSearchQueries()
    self.frameworkHelper = frameworkHelper
    super.init(nibName: nil, bundle: nil)
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    if frameworkHelper.isSearchHistoryEmpty() {
      showNoResultsView()
    } else {
      tableView.registerNib(cellClass: SearchHistoryQueryCell.self)
      let nib = UINib(nibName: "SearchHistoryClearCell", bundle: nil)
      tableView.register(nib, forCellReuseIdentifier: SearchHistoryViewController.clearCellIdentifier)
    }
    tableView.keyboardDismissMode = .onDrag
  }
  
  func showNoResultsView() {
    guard let noResultsView = MWMSearchNoResults.view(with: nil,
                                                      title: L("search_history_title"),
                                                      text: L("search_history_text")) else {
                                                        assertionFailure()
                                                        return
    }
    noResultsViewContainer.addSubview(noResultsView)
    tableView.isHidden = true
  }
  
  func clearSearchHistory() {
    frameworkHelper.clearSearchHistory()
    lastQueries = []
  }
}

extension SearchHistoryViewController: UITableViewDataSource {
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return frameworkHelper.isSearchHistoryEmpty() ? 0 : lastQueries.count + 1 
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
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     animations: {
                      tableView.alpha = 0.0
      }) { _ in
        self.showNoResultsView()
      }
    } else {
      let query = lastQueries[indexPath.row]
      delegate?.searchHistoryViewController(self, didSelect: query)
    }
  }
}
