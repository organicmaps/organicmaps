@objc(MWMUGCAddReviewControllerDelegate)
protocol UGCAddReviewControllerDelegate {
  typealias onSaveHandler = (Bool) -> Void
  func saveUgc(model: UGCAddReviewController.Model, resultHandler: @escaping onSaveHandler)
}

@objc(MWMUGCAddReviewController)
final class UGCAddReviewController: MWMTableViewController {
  typealias Model = UGCReviewModel

  weak var textCell: UGCAddReviewTextCell?
  var reviewPosted = false

  enum Sections {
    case ratings
    case text
  }

  @objc static func instance(model: Model, delegate: UGCAddReviewControllerDelegate) -> UGCAddReviewController {
    let vc = UGCAddReviewController(nibName: toString(self), bundle: nil)
    vc.model = model
    vc.delegate = delegate
    return vc
  }

  private var model: Model! {
    didSet {
      sections = []
      assert(!model.ratings.isEmpty)
      sections.append(.ratings)
      sections.append(.text)
    }
  }

  private var sections: [Sections] = []
  private var delegate: UGCAddReviewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()
    configNavBar()
    configTableView()
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    if isMovingFromParentViewController && !reviewPosted {
      Statistics.logEvent(kStatUGCReviewCancel)
    }
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
      assertionFailure()
      return
    }
    
    reviewPosted = true
    model.text = text
    
    delegate!.saveUgc(model: model, resultHandler: { (saveResult) in
      guard let nc = self.navigationController else { return }

      if !saveResult {
        nc.popViewController(animated: true)
        return
      }
      
      Statistics.logEvent(kStatUGCReviewSuccess)
      
      let onSuccess = { Toast.toast(withText: L("ugc_thanks_message_auth")).show() }
      let onError = { Toast.toast(withText: L("ugc_thanks_message_not_auth")).show() }
      let onComplete = { () -> Void in nc.popToRootViewController(animated: true) }
      
      if MWMAuthorizationViewModel.isAuthenticated() || MWMPlatform.networkConnectionType() == .none {
        if MWMAuthorizationViewModel.isAuthenticated() {
          onSuccess()
        } else {
          onError()
        }
        nc.popViewController(animated: true)
      } else {
        Statistics.logEvent(kStatUGCReviewAuthShown, withParameters: [kStatFrom: kStatAfterSave])
        let authVC = AuthorizationViewController(barButtonItem: self.navigationItem.rightBarButtonItem!,
                                                 sourceComponent: .UGC,
                                                 successHandler: {_ in onSuccess()},
                                                 errorHandler: {_ in onError()},
                                                 completionHandler: {_ in onComplete()})
        self.present(authVC, animated: true, completion: nil)
      }
    })
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
      textCell = cell
      return cell
    }
  }
}
