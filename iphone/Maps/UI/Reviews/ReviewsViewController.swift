@objc(MWMReviewsViewController)
final class ReviewsViewController: MWMTableViewController {
  private let viewModel: MWMReviewsViewModelProtocol

  @objc init(viewModel: MWMReviewsViewModelProtocol) {
    self.viewModel = viewModel
    super.init(nibName: toString(type(of: self)), bundle: nil)
  }

  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    registerCells()
  }

  private func registerCells() {
    [UGCYourReviewCell.self, UGCReviewCell.self].forEach {
      tableView.register(cellClass: $0)
    }
  }

  override func tableView(_: UITableView, numberOfRowsInSection _: Int) -> Int {
    return viewModel.numberOfReviews()
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cellModel = viewModel.review(with: indexPath.row)
    switch cellModel {
    case let cellModel as UGCYourReview:
      let cell = tableView.dequeueReusableCell(withCellClass: UGCYourReviewCell.self, indexPath: indexPath) as! UGCYourReviewCell
      cell.config(yourReview: cellModel, onUpdate: tableView.refresh)
      return cell
    case let cellModel as UGCReview:
      let cell = tableView.dequeueReusableCell(withCellClass: UGCReviewCell.self, indexPath: indexPath) as! UGCReviewCell
      cell.config(review: cellModel, onUpdate: tableView.refresh)
      return cell
    default: assert(false)
    }
    return UITableViewCell()
  }
}
