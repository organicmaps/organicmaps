extension BookmarksListCell {
  struct Configuration {
    enum LeadingItem {
      case none
      case image(UIImage, tintColor: UIColor?, action: ((UIView) -> Void)?)
    }

    enum AccessoryItem {
      case none
      case detailButton
      case image(UIImage, tintColor: UIColor?, action: ((UIView) -> Void)?)
    }

    let title: String
    let subtitle: String?
    let leadingItem: LeadingItem
    let accessoryItem: AccessoryItem
    let selectionStyle: UITableViewCell.SelectionStyle
  }
}

extension BookmarksListCell.Configuration {
  static var `default`: Self {
    BookmarksListCell.Configuration(title: "",
                                    subtitle: "",
                                    leadingItem: .none,
                                    accessoryItem: .none,
                                    selectionStyle: .default)
  }

  static func bookmark(_ item: IBookmarksListItemViewModel) -> Self {
    BookmarksListCell.Configuration(title: item.name,
                                    subtitle: item.subtitle,
                                    leadingItem: .image(item.image,
                                                        tintColor: nil,
                                                        action: item.colorDidTapAction),
                                    accessoryItem: .detailButton,
                                    selectionStyle: .default)
  }

  static func category(_ category: BookmarkGroup,
                       leadingAction: @escaping (UIView) -> Void,
                       accessoryAction: @escaping (UIView) -> Void) -> Self {
    BookmarksListCell.Configuration(title: category.title,
                                    subtitle: category.placesCountTitle(),
                                    leadingItem: .image(category.isVisible ? UIImage.icEyeOn : UIImage.icEyeOff,
                                                        tintColor: category.isVisible ? .linkBlue : .blackHintText,
                                                        action: leadingAction),
                                    accessoryItem: .image(UIImage.ic24PxMore,
                                                          tintColor: .blackHintText,
                                                          action: accessoryAction),
                                    selectionStyle: .default)
  }

  static func action(_ action: BMCAction) -> Self {
    BookmarksListCell.Configuration(title: action.title,
                                    subtitle: nil,
                                    leadingItem: .image(action.image,
                                                        tintColor: .linkBlue,
                                                        action: nil),
                                    accessoryItem: .none,
                                    selectionStyle: .default)
  }

  static func loading() -> Self {
    BookmarksListCell.Configuration(title: L("load_kmz_title"),
                                    subtitle: nil,
                                    leadingItem: .none,
                                    accessoryItem: .none,
                                    selectionStyle: .none)
  }
}
