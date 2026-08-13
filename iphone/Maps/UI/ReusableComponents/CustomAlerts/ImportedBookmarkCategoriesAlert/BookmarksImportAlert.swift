import UIKit

@objc(MWMBookmarksImportAlert)
final class BookmarksImportAlert: MWMAlert {
  private enum Constants {
    static let title = L("load_kmz_title")
    static let success = L("load_kmz_successful")
    static let failure = L("load_kmz_failed")
    static let close = L("ok")

    static let rowHeight: CGFloat = 52
    static let sectionHeaderHeight: CGFloat = 40
    static let sectionHeaderTopInset: CGFloat = 8
    static let horizontalContentInset: CGFloat = 18
    static let contentTopInset: CGFloat = 20
    static let contentBottomInset: CGFloat = 12
    static let contentSpacing: CGFloat = 12
    static let containerWidth: CGFloat = 340
    static let minimumHorizontalAlertInset: CGFloat = 16
    static let minimumVerticalAlertInset: CGFloat = 24
    static let maximumAlertHeightRatio: CGFloat = 0.8
    static let closeButtonMinimumHeight: CGFloat = 52
    static let layoutTolerance: CGFloat = 0.5
  }

  private enum Row {
    case category(id: NSNumber, name: String)
    case failure(fileName: String)

    var title: String {
      switch self {
      case .category(_, let name): name
      case .failure(let fileName): fileName
      }
    }

    var textColor: TextColorStyleSheet {
      switch self {
      case .category: .blackPrimary
      case .failure: .buttonRed
      }
    }

    var isSelectable: Bool {
      if case .category = self {
        return true
      }
      return false
    }
  }

  private struct Section {
    let title: String
    let rows: [Row]
  }

  private let sections: [Section]
  private let selectCategory: ((NSNumber) -> Void)?
  private var tableHeightConstraint: NSLayoutConstraint?

  private lazy var tableView: UITableView = {
    let tableView = UITableView(frame: .zero, style: .plain)
    tableView.translatesAutoresizingMaskIntoConstraints = false
    tableView.setStyle(.clearBackground)
    tableView.backgroundColor = .clear
    tableView.backgroundView = nil
    tableView.dataSource = self
    tableView.delegate = self
    tableView.rowHeight = UITableView.automaticDimension
    tableView.estimatedRowHeight = Constants.rowHeight
    tableView.sectionHeaderHeight = UITableView.automaticDimension
    tableView.estimatedSectionHeaderHeight = Constants.sectionHeaderHeight
    tableView.sectionHeaderTopPadding = 0
    tableView.isScrollEnabled = false
    tableView.alwaysBounceVertical = false
    tableView.separatorStyle = .none
    tableView.register(cell: UITableViewCell.self)
    return tableView
  }()

  @objc(alertWithCategoryIds:categoryNames:failedFileNames:selectCategory:)
  static func alert(categoryIds: [NSNumber],
                    categoryNames: [String],
                    failedFileNames: [String],
                    selectCategory: ((NSNumber) -> Void)?) -> BookmarksImportAlert {
    assert(categoryIds.count == categoryNames.count)

    var sections = [Section]()
    if !failedFileNames.isEmpty {
      sections.append(Section(title: Constants.failure,
                              rows: failedFileNames.map { Row.failure(fileName: $0) }))
    }
    let categories = zip(categoryIds, categoryNames).map { Row.category(id: $0, name: $1) }
    if !categories.isEmpty {
      sections.append(Section(title: Constants.success, rows: categories))
    }

    return BookmarksImportAlert(sections: sections, selectCategory: selectCategory)
  }

  private init(sections: [Section], selectCategory: ((NSNumber) -> Void)?) {
    self.sections = sections
    self.selectCategory = selectCategory
    super.init(frame: .zero)
    autoresizingMask = [.flexibleWidth, .flexibleHeight]
    setupView()
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    tableView.layoutIfNeeded()
    let contentHeight = ceil(tableView.contentSize.height)
    if let tableHeightConstraint,
       abs(tableHeightConstraint.constant - contentHeight) > Constants.layoutTolerance {
      tableHeightConstraint.constant = contentHeight
    }

    let isScrollEnabled = contentHeight > tableView.bounds.height + Constants.layoutTolerance
    tableView.isScrollEnabled = isScrollEnabled
    if !isScrollEnabled, tableView.contentOffset != .zero {
      tableView.setContentOffset(.zero, animated: false)
    }
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    let container = UIView()
    container.translatesAutoresizingMaskIntoConstraints = false
    container.setStyleAndApply(.alertView)

    let titleLabel = UILabel()
    titleLabel.translatesAutoresizingMaskIntoConstraints = false
    titleLabel.numberOfLines = 0
    titleLabel.textAlignment = .center
    titleLabel.text = Constants.title
    titleLabel.setFontStyleAndApply(.medium18, color: .blackPrimary)
    titleLabel.setContentCompressionResistancePriority(.required, for: .vertical)

    let contentStack = UIStackView(arrangedSubviews: [titleLabel, tableView])
    contentStack.translatesAutoresizingMaskIntoConstraints = false
    contentStack.axis = .vertical
    contentStack.spacing = Constants.contentSpacing
    contentStack.isLayoutMarginsRelativeArrangement = true
    contentStack.layoutMargins = UIEdgeInsets(top: Constants.contentTopInset,
                                              left: Constants.horizontalContentInset,
                                              bottom: Constants.contentBottomInset,
                                              right: Constants.horizontalContentInset)
    contentStack.addSeparator(.bottom)

    let closeButton = UIButton(type: .system)
    closeButton.translatesAutoresizingMaskIntoConstraints = false
    closeButton.setTitle(Constants.close, for: .normal)
    closeButton.setStyleAndApply(.flatNormalTransButtonBig)
    closeButton.addTarget(self, action: #selector(closeButtonDidTap), for: .touchUpInside)

    let stack = UIStackView(arrangedSubviews: [contentStack, closeButton])
    stack.translatesAutoresizingMaskIntoConstraints = false
    stack.axis = .vertical
    container.addSubview(stack)
    addSubview(container)

    let tableHeight = CGFloat(sections.reduce(0) { $0 + $1.rows.count }) * Constants.rowHeight
      + CGFloat(sections.count) * Constants.sectionHeaderHeight
    let tableHeightConstraint = tableView.heightAnchor.constraint(equalToConstant: tableHeight)
    tableHeightConstraint.priority = .defaultHigh
    self.tableHeightConstraint = tableHeightConstraint
    let containerWidthConstraint = container.widthAnchor.constraint(equalToConstant: Constants.containerWidth)
    containerWidthConstraint.priority = .defaultHigh

    NSLayoutConstraint.activate([
      stack.topAnchor.constraint(equalTo: container.topAnchor),
      stack.leadingAnchor.constraint(equalTo: container.leadingAnchor),
      stack.trailingAnchor.constraint(equalTo: container.trailingAnchor),
      stack.bottomAnchor.constraint(equalTo: container.bottomAnchor),
      closeButton.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.closeButtonMinimumHeight),
      tableHeightConstraint,

      container.centerXAnchor.constraint(equalTo: safeAreaLayoutGuide.centerXAnchor),
      container.centerYAnchor.constraint(equalTo: safeAreaLayoutGuide.centerYAnchor),
      containerWidthConstraint,
      container.widthAnchor.constraint(lessThanOrEqualTo: safeAreaLayoutGuide.widthAnchor,
                                       constant: -2 * Constants.minimumHorizontalAlertInset),
      container.heightAnchor.constraint(lessThanOrEqualTo: safeAreaLayoutGuide.heightAnchor,
                                        multiplier: Constants.maximumAlertHeightRatio),
      container.topAnchor.constraint(greaterThanOrEqualTo: safeAreaLayoutGuide.topAnchor,
                                     constant: Constants.minimumVerticalAlertInset),
      container.bottomAnchor.constraint(lessThanOrEqualTo: safeAreaLayoutGuide.bottomAnchor,
                                        constant: -Constants.minimumVerticalAlertInset),
    ])
  }

  @objc private func closeButtonDidTap() {
    close(nil)
  }
}

// MARK: - UITableViewDataSource, UITableViewDelegate

extension BookmarksImportAlert: UITableViewDataSource, UITableViewDelegate {
  func numberOfSections(in _: UITableView) -> Int {
    sections.count
  }

  func tableView(_: UITableView, numberOfRowsInSection section: Int) -> Int {
    sections[section].rows.count
  }

  func tableView(_: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    let label = UILabel()
    label.numberOfLines = 0
    label.text = sections[section].title
    label.setFontStyleAndApply(.regular14, color: .blackSecondary)
    label.accessibilityTraits = .header

    let container = UIView()
    container.backgroundColor = .clear
    container.addSubview(label)
    label.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      label.topAnchor.constraint(equalTo: container.topAnchor, constant: Constants.sectionHeaderTopInset),
      label.leadingAnchor.constraint(equalTo: container.leadingAnchor,
                                     constant: Constants.horizontalContentInset),
      label.trailingAnchor.constraint(equalTo: container.trailingAnchor,
                                      constant: -Constants.horizontalContentInset),
      label.bottomAnchor.constraint(equalTo: container.bottomAnchor),
    ])
    return container
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: UITableViewCell.self, indexPath: indexPath)
    let row = sections[indexPath.section].rows[indexPath.row]

    cell.setStyleAndApply(.noStyleTableViewCell)
    cell.backgroundColor = .clear
    cell.backgroundView = nil
    cell.contentView.backgroundColor = .clear
    cell.textLabel?.numberOfLines = 0
    cell.textLabel?.setFontStyleAndApply(.regular16, color: row.textColor)
    cell.textLabel?.text = row.title
    cell.accessoryType = row.isSelectable ? .disclosureIndicator : .none
    cell.selectionStyle = row.isSelectable ? .default : .none

    return cell
  }

  func tableView(_: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard case .category(let id, _) = sections[indexPath.section].rows[indexPath.row] else { return }
    close { [selectCategory] in
      selectCategory?(id)
    }
  }
}
