protocol PlacePageEditBookmarkOrTrackViewControllerDelegate: AnyObject {
  func didUpdate(color: UIColor, category: MWMMarkGroupID, for data: PlacePageEditData)
  func didPressEdit(_ data: PlacePageEditData)
}

enum PlacePageEditData {
  case bookmark(PlacePageBookmarkData)
  case track(PlacePageTrackData)
}

final class PlacePageEditBookmarkAndTrackSectionInteractor: PlacePageExpandableDetailsSectionInteractor {

  let presenter: PlacePageExpandableDetailsSectionPresenter
  var data: PlacePageEditData? {
    didSet {
      guard data != nil else { return }
      reloadData()
    }
  }
  weak var delegate: PlacePageEditBookmarkOrTrackViewControllerDelegate?

  init(presenter: PlacePageExpandableDetailsSectionPresenter,
       data: PlacePageEditData?,
       delegate: PlacePageEditBookmarkOrTrackViewControllerDelegate) {
    self.presenter = presenter
    self.data = data
    self.delegate = delegate
  }

  func handle(_ event: PlacePageExpandableDetailsSectionRequest) {
    let responses = resolve(event)
    presenter.process(responses)
  }

  private func resolve(_ event: PlacePageExpandableDetailsSectionRequest) -> [PlacePageExpandableDetailsSectionResponse] {
    switch event {
    case .viewDidLoad:
      return []
    case .didTapIcon:
      showColorPicker()
      return []
    case .didTapTitle:
      showGroupPicker()
      return []
    case .didLongPressTitle:
      return []
    case .didTapAccessory:
      if let data {
        delegate?.didPressEdit(data)
      }
      return []
    case .didTapExpandableText:
      return [.expandText]
    }
  }

  private func reloadData() {
    guard let data else { return }

    let iconColor: UIColor
    let category: String?
    let description: String?
    let isHtmlDescription: Bool

    switch data {
    case .bookmark(let bookmarkData):
      iconColor = bookmarkData.color.color
      category = bookmarkData.bookmarkCategory
      description = bookmarkData.bookmarkDescription
      isHtmlDescription = bookmarkData.isHtmlDescription
    case .track(let trackData):
      iconColor = trackData.color ?? UIColor.buttonRed()
      category = trackData.trackCategory
      description = trackData.trackDescription
      isHtmlDescription = false
    }

    let editColorImage = circleImageForColor(iconColor, frameSize: 28, diameter: 22, iconName: "ic_bm_none")

    presenter.process([
      .updateTitle(category ?? ""),
      .updateIcon(editColorImage),
      .updateAccessory(.ic24PxEdit),
      .updateExpandableText(description, isHTML: isHtmlDescription)
    ])
  }

  private func showColorPicker() {
    guard let data, let view = presenter.view else { return }
    switch data {
    case .bookmark(let bookmarkData):
      ColorPicker.shared.present(from: view, pickerType: .bookmarkColorPicker(bookmarkData.color)) { [weak self] color in
        self?.update(color: color)
      }
    case .track(let trackData):
      ColorPicker.shared.present(from: view, pickerType: .defaultColorPicker(trackData.color ?? .buttonRed())) { [weak self] color in
        self?.update(color: color)
      }
    }
  }

  private func showGroupPicker() {
    guard let data else { return }
    let groupId: MWMMarkGroupID
    let groupName: String?
    switch data {
    case .bookmark(let bookmarkData):
      groupId = bookmarkData.bookmarkGroupId
      groupName = bookmarkData.bookmarkCategory
    case .track(let trackData):
      groupId = trackData.groupId
      groupName = trackData.trackCategory
    }
    let groupViewController = SelectBookmarkGroupViewController(groupName: groupName ?? "", groupId: groupId)
    let navigationController = UINavigationController(rootViewController: groupViewController)
    groupViewController.delegate = self
    presenter.view?.present(navigationController, animated: true, completion: nil)
  }

  private func update(color: UIColor? = nil, category: MWMMarkGroupID? = nil) {
    guard let data else { return }
    switch data {
    case .bookmark(let bookmarkData):
      delegate?.didUpdate(color: color ?? bookmarkData.color.color, category: category ?? bookmarkData.bookmarkGroupId, for: data)
    case .track(let trackData):
      delegate?.didUpdate(color: color ?? trackData.color!, category: category ?? trackData.groupId, for: data)
    }
  }
}

// MARK: - SelectBookmarkGroupViewControllerDelegate
extension PlacePageEditBookmarkAndTrackSectionInteractor: SelectBookmarkGroupViewControllerDelegate {
  func bookmarkGroupViewController(_ viewController: SelectBookmarkGroupViewController,
                                   didSelect groupTitle: String,
                                   groupId: MWMMarkGroupID) {
    viewController.navigationController?.dismiss(animated: true)
    update(category: groupId)
  }
}
