protocol SearchCategoriesViewControllerDelegate: SearchOnMapScrollViewDelegate {
  func categoriesViewController(_ viewController: SearchCategoriesViewController,
                                didSelect category: String)
}

final class SearchCategoriesViewController: MWMTableViewController {
  private weak var delegate: SearchCategoriesViewControllerDelegate?
  private let categories: [String]

  init(frameworkHelper: MWMSearchFrameworkHelper.Type, delegate: SearchCategoriesViewControllerDelegate?) {
    self.delegate = delegate
    categories = frameworkHelper.searchCategories()
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    tableView.setStyle(.background)
    tableView.register(cell: SearchCategoryCell.self)
    tableView.keyboardDismissMode = .onDrag
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return categories.count
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: SearchCategoryCell.self, indexPath: indexPath)
    cell.configure(with: category(at: indexPath))
    return cell
  }
  
  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    let selectedCategory = category(at: indexPath)
    delegate?.categoriesViewController(self, didSelect: selectedCategory)
    tableView.deselectRow(at: indexPath, animated: true)
  }

  override func scrollViewDidScroll(_ scrollView: UIScrollView) {
    delegate?.scrollViewDidScroll(scrollView)
  }

  override func scrollViewWillEndDragging(_ scrollView: UIScrollView, withVelocity velocity: CGPoint, targetContentOffset: UnsafeMutablePointer<CGPoint>) {
    delegate?.scrollViewWillEndDragging(scrollView, withVelocity: velocity, targetContentOffset: targetContentOffset)
  }

  func category(at indexPath: IndexPath) -> String {
    let index = indexPath.row
    return categories[index]
  }
}

extension SearchCategoriesViewController: ModallyPresentedViewController {
  func presentationFrameDidChange(_ frame: CGRect) {
    guard isViewLoaded else { return }
    tableView.contentInset.bottom = frame.origin.y + view.safeAreaInsets.bottom
  }
}
