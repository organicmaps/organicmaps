@objc(MWMUGCAddReviewController)
final class UGCAddReviewController: MWMTableViewController {
  typealias Model = UGCReviewModel

  weak var textCell: UGCAddReviewTextCell?

  enum Sections {
    case ratings
    case text
  }

  @objc static func instance(model: Model, onSave: @escaping (Model) -> Void) -> UGCAddReviewController {
    let vc = UGCAddReviewController(nibName: toString(self), bundle: nil)
    vc.model = model
    vc.onSave = onSave
    return vc
  }

  private var model: Model! {
    didSet {
      sections = []
      assert(!model.ratings.isEmpty);
      sections.append(.ratings)
      sections.append(.text)
    }
  }

  private var onSave: ((Model) -> Void)!
  private var sections: [Sections] = []

  override func viewDidLoad() {
    super.viewDidLoad()
    configNavBar()
    configTableView()
  }

  private func configNavBar() {
    title = model.title
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(onDone))
  }

  private func configTableView() {
    tableView.register(cellClass: UGCAddReviewRatingCell.self)
    tableView.register(cellClass: UGCAddReviewTextCell.self)

    tableView.estimatedRowHeight = 48
    tableView.rowHeight = UITableViewAutomaticDimension
  }

  @objc private func onDone() {
    guard let text = textCell?.reviewText else {
      assert(false);
      return
    }
    model.text = text
    onSave(model)
    guard let nc = navigationController else { return }
    if MWMAuthorizationViewModel.isAuthenticated() {
      nc.popViewController(animated: true)
    } else {
      let authVC = AuthorizationViewController(barButtonItem: navigationItem.rightBarButtonItem!,
                                               completion: { nc.popViewController(animated: true) })
      nc.show(authVC, sender: self)
    }
  }

  override func numberOfSections(in _: UITableView) -> Int {
    return sections.count
  }

  override func tableView(_: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch sections[section] {
    case .ratings: return model.ratings.count
    case .text: return 1
    }
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    switch sections[indexPath.section] {
    case .ratings:
      let cell = tableView.dequeueReusableCell(withCellClass: UGCAddReviewRatingCell.self, indexPath: indexPath) as! UGCAddReviewRatingCell
      cell.model = model.ratings[indexPath.row]
      return cell
    case .text:
      let cell = tableView.dequeueReusableCell(withCellClass: UGCAddReviewTextCell.self, indexPath: indexPath) as! UGCAddReviewTextCell
      cell.reviewText = model.text
      textCell = cell;
      return cell
    }
  }
}
