final class RecentlyDeletedTableViewCell: UITableViewCell {

  struct ViewModel: Equatable, Hashable {
    let fileName: String
    let fileURL: URL
    let deletionDate: Date
  }

  private static let dateFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.dateStyle = .medium
    formatter.timeStyle = .medium
    return formatter
  }()

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func configureWith(_ viewModel: ViewModel) {
    textLabel?.text = viewModel.fileName
    detailTextLabel?.text = Self.dateFormatter.string(from: viewModel.deletionDate)
  }
}

extension RecentlyDeletedTableViewCell.ViewModel {
  init(_ category: RecentlyDeletedCategory) {
    self.fileName = category.title
    self.fileURL = category.fileURL
    self.deletionDate = category.deletionDate
  }
}
