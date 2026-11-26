final class RecentlyDeletedTableViewCell: UITableViewCell {
  struct ViewModel: Equatable, Hashable {
    let fileName: String
    let deletionDate: Date
  }

  override init(style _: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func configureWith(_ viewModel: ViewModel) {
    textLabel?.text = viewModel.fileName
    detailTextLabel?.text = DateTimeFormatter.dateString(from: viewModel.deletionDate,
                                                         dateStyle: .medium,
                                                         timeStyle: .medium)
    accessibilityTraits.insert(.button)
  }
}

extension RecentlyDeletedTableViewCell.ViewModel {
  init(_ category: RecentlyDeletedCategory) {
    fileName = category.title
    deletionDate = category.deletionDate
  }
}
