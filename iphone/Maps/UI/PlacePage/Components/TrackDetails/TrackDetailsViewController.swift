
class TrackDetailsBuilder {
  static func build(data: PlacePageData) -> TrackDetailsViewController {
    let viewModel = TrackDetailsViewModel(data: data)
    let viewController = TrackDetailsViewController(viewModel: viewModel)
    return viewController

  }
}


struct TrackDetailsViewModel {
  let distance: String
  let duration: String

  init(data: PlacePageData) {
    let track = BookmarksManager.shared().track(withId: data.previewData.)
    distance = data.
    duration = data.
  }
}

class TrackDetailsViewController: MWMTableViewController {

  var viewModel: TrackDetailsViewModel

  init(viewModel: TrackDetailsViewModel) {
    self.viewModel = viewModel
    super.init()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
  }

  private func setupView() {
    tableView.register(cell: UITableViewCell.self)
  }
}
