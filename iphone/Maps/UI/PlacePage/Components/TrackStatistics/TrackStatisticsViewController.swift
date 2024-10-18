class TrackStatisticsViewController: MWMTableViewController {

  private var viewModel: TrackStatisticsViewModel
  private var viewHeightConstraint: NSLayoutConstraint?

  init(viewModel: TrackStatisticsViewModel) {
    self.viewModel = viewModel
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
  }

  override func viewWillLayoutSubviews() {
    super.viewWillLayoutSubviews()
    viewHeightConstraint?.constant = tableView.contentSize.height
  }

  private func setupView() {
    viewHeightConstraint = view.heightAnchor.constraint(equalToConstant: .zero)
    viewHeightConstraint?.isActive = true
    tableView.register(cell: TrackStatisticsTableViewCell.self)
    tableView.allowsSelection = false
  }

  override func numberOfSections(in tableView: UITableView) -> Int {
    viewModel.data.count
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    viewModel.data[section].cells.count
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: TrackStatisticsTableViewCell.self, indexPath: indexPath)
    let cellData = viewModel.data[indexPath.section].cells[indexPath.row]
    cell.configure(text: cellData.text, detailText: cellData.detailText)
    return cell
  }
}
