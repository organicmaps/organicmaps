@objc(MWMGCReviewSaver)
protocol UGCReviewSaver {
  typealias onSaveHandler = (Bool) -> Void
  func saveUgc(placePageData: PlacePageData, model: UGCReviewModel, language: String, resultHandler: @escaping onSaveHandler)
}

@objc(MWMUGCAddReviewController)
final class UGCAddReviewController: MWMTableViewController {
  private weak var textCell: UGCAddReviewTextCell?
  private var reviewPosted = false

  private enum Sections {
    case ratings
    case text
  }

  private let placePageData: PlacePageData
  private let model: UGCReviewModel
  private let saver: UGCReviewSaver
  private var sections: [Sections] = []

  @objc init(placePageData: PlacePageData, rating: UgcSummaryRatingType, saver: UGCReviewSaver) {
    self.placePageData = placePageData
    self.saver = saver
    let ratings = placePageData.ratingCategories.map {
      UGCRatingStars(title: $0, value: CGFloat(rating.rawValue))
    }
    model = UGCReviewModel(ratings: ratings, text: "")
    super.init(nibName: toString(UGCAddReviewController.self), bundle: nil)
    title = placePageData.previewData.title
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    sections.append(.ratings)
    sections.append(.text)

    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(onDone))
    configTableView()
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    if isMovingFromParent && !reviewPosted {
      Statistics.logEvent(kStatUGCReviewCancel)
    }
  }

  private func configTableView() {
    tableView.registerNib(cellClass: UGCAddReviewRatingCell.self)
    tableView.registerNib(cellClass: UGCAddReviewTextCell.self)

    tableView.estimatedRowHeight = 48
    tableView.rowHeight = UITableView.automaticDimension
  }

  @objc private func onDone() {
    guard let text = textCell?.reviewText else {
      assertionFailure()
      return
    }
    
    reviewPosted = true
    model.text = text
    
    saver.saveUgc(placePageData: placePageData,
                  model: model,
                  language: textCell?.reviewLanguage ?? "en",
                  resultHandler: { (saveResult) in
      guard let nc = self.navigationController else { return }

      if !saveResult {
        nc.popViewController(animated: true)
        return
      }
      
      Statistics.logEvent(kStatUGCReviewSuccess)
      
      let onSuccess = { Toast.toast(withText: L("ugc_thanks_message_auth")).show() }
      let onError = { Toast.toast(withText: L("ugc_thanks_message_not_auth")).show() }
      let onComplete = { () -> Void in nc.popToRootViewController(animated: true) }
      
      if User.isAuthenticated() || !FrameworkHelper.isNetworkConnected() {
        if User.isAuthenticated() {
          onSuccess()
        } else {
          onError()
        }
        nc.popViewController(animated: true)
      } else {
        let authVC = AuthorizationViewController(barButtonItem: self.navigationItem.rightBarButtonItem!,
                                                 source: .afterSaveReview,
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
