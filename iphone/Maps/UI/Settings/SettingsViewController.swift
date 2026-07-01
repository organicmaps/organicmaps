enum SettingsViewControllerAction<Item: Hashable> {
  case didLoad
  case willAppear
  case didAppear
  case willDisappear
  case didSelect(Item)
  case didTapAccessory(Item)
  case didChangeSwitch(Item, isOn: Bool)
  case didChangeText(Item, text: String)
  case didEndEditingText(Item, text: String)
  case didChangeSlider(Item, value: Float)
  case didCompleteBookmarkBackupSharing(Bool)
}

protocol SettingsViewControllerInteractor<Section, Item>: AnyObject {
  associatedtype Section: Hashable
  associatedtype Item: Hashable

  func handle(_ action: SettingsViewControllerAction<Item>)
}

// TODO: Remove this ObjC-wrapper class when the OSMAuthorization view controllers will be rewritten on Swift.
@objc(SettingsViewController)
class BaseSettingsViewController: MWMTableViewController {}

final class SettingsViewController<Section: Hashable, Item: Hashable>: BaseSettingsViewController,
  SettingsTableViewSwitchCellDelegate, SettingsTextFieldCellDelegate, SettingsSliderCellDelegate {
  private var interactor: (any SettingsViewControllerInteractor<Section, Item>)?

  private var dataSource: SettingsTableViewDataSource<Section, Item>!
  private var itemViewModels: [Item: SettingsItemViewModel<Item>] = [:]

  init() {
    super.init(style: .grouped)
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
  }

  func configure<Interactor: SettingsViewControllerInteractor>(interactor: Interactor)
    where Interactor.Section == Section, Interactor.Item == Item {
    self.interactor = interactor
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    configureTableView()
    interactor?.handle(.didLoad)
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    interactor?.handle(.willAppear)
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    interactor?.handle(.didAppear)
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    view.endEditing(true)
    interactor?.handle(.willDisappear)
  }

  private func configureTableView() {
    tableView.register(cell: SettingsTableViewLinkCell.self)
    tableView.register(cell: SettingsTableViewSelectableCell.self)
    tableView.register(cell: SettingsTableViewSwitchCell.self)
    tableView.register(cell: SettingsTableViewiCloudSwitchCell.self)
    tableView.register(cell: SettingsTextFieldCell.self)
    tableView.register(cell: SettingsMessageCell.self)
    tableView.register(cell: SettingsSliderCell.self)
    tableView.rowHeight = UITableView.automaticDimension
    tableView.estimatedRowHeight = 44

    dataSource = makeDataSource()
    dataSource.defaultRowAnimation = .fade
    tableView.dataSource = dataSource
  }

  private func makeDataSource() -> SettingsTableViewDataSource<Section, Item> {
    SettingsTableViewDataSource(tableView: tableView) { [weak self] tableView, indexPath, item in
      self?.cell(tableView, indexPath: indexPath, item: item) ?? UITableViewCell()
    }
  }

  private func cell(_ tableView: UITableView, indexPath: IndexPath, item: Item) -> UITableViewCell {
    guard let itemViewModel = itemViewModels[item] else { return UITableViewCell() }
    switch itemViewModel.kind {
    case .link:
      let cell = tableView.dequeueReusableCell(cell: SettingsTableViewLinkCell.self, indexPath: indexPath)
      cell.config(title: itemViewModel.title ?? "", info: itemViewModel.detail)
      return cell
    case .selectable(let isSelected):
      let cell = tableView.dequeueReusableCell(cell: SettingsTableViewSelectableCell.self, indexPath: indexPath)
      cell.config(title: itemViewModel.title ?? "", isSelected: isSelected)
      return cell
    case .switcher(let isOn, let isEnabled):
      let cell = tableView.dequeueReusableCell(cell: SettingsTableViewSwitchCell.self, indexPath: indexPath)
      cell.isEnabled = isEnabled
      cell.config(delegate: self, title: itemViewModel.title ?? "", subtitile: itemViewModel.detail, isOn: isOn)
      return cell
    case .iCloudSwitcher(let isOn, let isAvailable, let errorDescription):
      let cell = tableView.dequeueReusableCell(cell: SettingsTableViewiCloudSwitchCell.self, indexPath: indexPath)
      cell.isEnabled = true
      cell.config(delegate: self,
                  title: itemViewModel.title ?? "",
                  subtitile: errorDescription,
                  isAvailable: isAvailable,
                  isOn: isOn)
      return cell
    case .textField(let text, let placeholder, let isEnabled, let isValid):
      let cell = tableView.dequeueReusableCell(cell: SettingsTextFieldCell.self, indexPath: indexPath)
      cell.configure(delegate: self, text: text, placeholder: placeholder, isEnabled: isEnabled, isValid: isValid)
      return cell
    case .message(let text):
      let cell = tableView.dequeueReusableCell(cell: SettingsMessageCell.self, indexPath: indexPath)
      cell.configure(text: text)
      return cell
    case .slider(let value, let minimumValue, let maximumValue, let valueTitle, let isEnabled):
      let cell = tableView.dequeueReusableCell(cell: SettingsSliderCell.self, indexPath: indexPath)
      cell.configure(delegate: self,
                     value: value,
                     minimumValue: minimumValue,
                     maximumValue: maximumValue,
                     valueTitle: valueTitle,
                     isEnabled: isEnabled)
      return cell
    }
  }

  private func item(at indexPath: IndexPath) -> Item? {
    dataSource.itemIdentifier(for: indexPath)
  }

  private func apply(_ viewModel: SettingsViewModel<Section, Item>) {
    title = viewModel.title

    let previousItems = itemViewModels
    let nextItems = Dictionary(uniqueKeysWithValues: viewModel.sections.flatMap(\.items).map { ($0.item, $0) })
    let itemsToReconfigure = itemsToReconfigure(previousItems: previousItems,
                                                nextItems: nextItems,
                                                forcedItems: viewModel.reconfiguredItems)
    itemViewModels = nextItems
    dataSource.sectionViewModels = Dictionary(uniqueKeysWithValues: viewModel.sections.map { ($0.section, $0) })

    var snapshot = NSDiffableDataSourceSnapshot<Section, Item>()
    for section in viewModel.sections {
      snapshot.appendSections([section.section])
      snapshot.appendItems(section.items.map(\.item), toSection: section.section)
    }
    snapshot.reconfigureItems(itemsToReconfigure.filter { snapshot.indexOfItem($0) != nil })
    dataSource.apply(snapshot, animatingDifferences: viewModel.animatingDifferences)
  }

  private func itemsToReconfigure(previousItems: [Item: SettingsItemViewModel<Item>],
                                  nextItems: [Item: SettingsItemViewModel<Item>],
                                  forcedItems: [Item]) -> [Item] {
    let changedItems = previousItems.isEmpty ? [] : nextItems.compactMap { item, viewModel in
      previousItems[item] == viewModel ? nil : item
    }
    return uniqueItems(changedItems + forcedItems)
  }

  private func uniqueItems(_ items: [Item]) -> [Item] {
    var seen = Set<Item>()
    return items.filter { seen.insert($0).inserted }
  }

  func display(_ viewModel: SettingsViewModel<Section, Item>) {
    apply(viewModel)
  }

  func display(_ alert: UIAlertController) {
    present(alert, animated: true)
  }

  func switchCell(_ cell: SettingsTableViewSwitchCell, didChangeValue value: Bool) {
    guard let indexPath = tableView.indexPath(for: cell),
          let item = item(at: indexPath) else { return }
    interactor?.handle(.didChangeSwitch(item, isOn: value))
  }

  func textFieldCell(_ cell: SettingsTextFieldCell, didEndEditingText text: String) {
    guard let indexPath = tableView.indexPath(for: cell),
          let item = item(at: indexPath) else { return }
    interactor?.handle(.didEndEditingText(item, text: text))
  }

  func textFieldCell(_ cell: SettingsTextFieldCell, didChangeText text: String) {
    guard let indexPath = tableView.indexPath(for: cell),
          let item = item(at: indexPath) else { return }
    interactor?.handle(.didChangeText(item, text: text))
  }

  func sliderCell(_ cell: SettingsSliderCell, didChangeValue value: Float) {
    guard let indexPath = tableView.indexPath(for: cell),
          let item = item(at: indexPath) else { return }
    interactor?.handle(.didChangeSlider(item, value: value))
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    guard let item = item(at: indexPath) else { return }
    interactor?.handle(.didSelect(item))
  }

  override func tableView(_: UITableView, accessoryButtonTappedForRowWith indexPath: IndexPath) {
    guard let item = item(at: indexPath) else { return }
    interactor?.handle(.didTapAccessory(item))
  }
}

extension SettingsViewController where Section == RootSettingsSection, Item == RootSettings {
  func display(_ screen: SettingsScreen) {
    navigationController?.pushViewController(SettingsBuilder.build(screen), animated: true)
  }

  func displayBookmarkBackupShare(message: String, url: URL) {
    let shareController = ActivityViewController.share(for: url,
                                                       message: message,
                                                       displayName: nil) { [weak self] _, completed, _, _ in
      guard let self else { return }
      interactor?.handle(.didCompleteBookmarkBackupSharing(completed))
    }
    let anchorView = dataSource.indexPath(for: .iCloud).flatMap { tableView.cellForRow(at: $0) } ?? view
    shareController.present(inParentViewController: self, anchorView: anchorView)
  }

  func displayHighlight(_ item: Item) {
    guard let indexPath = dataSource.indexPath(for: item),
          let cell = tableView.cellForRow(at: indexPath) else { return }
    tableView.scrollToRow(at: indexPath, at: .middle, animated: true)
    cell.highlight()
  }
}

private final class SettingsTableViewDataSource<Section: Hashable, Item: Hashable>: UITableViewDiffableDataSource<Section, Item> {
  var sectionViewModels: [Section: SettingsSectionViewModel<Section, Item>] = [:]

  override func tableView(_: UITableView, titleForHeaderInSection section: Int) -> String? {
    guard section < snapshot().sectionIdentifiers.count else { return nil }
    return sectionViewModels[snapshot().sectionIdentifiers[section]]?.header
  }

  override func tableView(_: UITableView, titleForFooterInSection section: Int) -> String? {
    guard section < snapshot().sectionIdentifiers.count else { return nil }
    return sectionViewModels[snapshot().sectionIdentifiers[section]]?.footer
  }
}
