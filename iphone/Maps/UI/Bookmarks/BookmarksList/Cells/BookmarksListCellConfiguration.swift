extension BookmarksListCell {
  struct Configuration {
    enum LeadingItem {
      case none
      case image(UIImage, tintColor: UIColor?, action: ((UIView) -> Void)?)
    }

    struct TrailingButton {
      let image: UIImage
      let tintColor: UIColor?
      let accessibilityLabel: String
      let action: (UIView) -> Void
    }

    enum TrailingButtons {
      case none
      /// A single button always takes the slot at the cell's trailing edge.
      case one(TrailingButton)
      case two(inner: TrailingButton, edge: TrailingButton)
    }

    let title: String
    let subtitle: String?
    let leadingItem: LeadingItem
    let trailingButtons: TrailingButtons
  }
}

extension BookmarksListCell.Configuration.TrailingButton {
  /// A replacement for `UITableViewCell.AccessoryType.detailButton`, which can neither share the row
  /// with another button nor keep its size when the content size category grows.
  static func info(action: @escaping (UIView) -> Void) -> Self {
    .init(image: .icInfo, tintColor: .linkBlue, accessibilityLabel: L("edit"), action: action)
  }

  static func visibility(isVisible: Bool, action: @escaping (UIView) -> Void) -> Self {
    .init(image: isVisible ? .icEyeOn : .icEyeOff,
          tintColor: isVisible ? .linkBlue : .blackHintText,
          accessibilityLabel: L(isVisible ? "hide_track" : "show_track"),
          action: action)
  }

  static func more(action: @escaping (UIView) -> Void) -> Self {
    .init(image: .ic24PxMore, tintColor: .blackHintText, accessibilityLabel: L("placepage_more_button"), action: action)
  }
}

extension BookmarksListCell.Configuration {
  static var `default`: Self {
    BookmarksListCell.Configuration(title: "",
                                    subtitle: nil,
                                    leadingItem: .none,
                                    trailingButtons: .none)
  }

  static func bookmark(_ item: IBookmarksListItemViewModel, infoAction: @escaping (UIView) -> Void) -> Self {
    BookmarksListCell.Configuration(title: item.name,
                                    subtitle: item.subtitle,
                                    leadingItem: .image(item.image,
                                                        tintColor: nil,
                                                        action: item.colorDidTapAction),
                                    trailingButtons: .one(.info(action: infoAction)))
  }

  static func track(_ item: ITrackViewModel, infoAction: @escaping (UIView) -> Void) -> Self {
    BookmarksListCell.Configuration(title: item.name,
                                    subtitle: item.subtitle,
                                    leadingItem: .image(item.image,
                                                        tintColor: nil,
                                                        action: item.colorDidTapAction),
                                    trailingButtons: .two(
                                      inner: .visibility(isVisible: item.isVisible,
                                                         action: { _ in item.visibilityDidTapAction() }),
                                      edge: .info(action: infoAction)
                                    ))
  }

  static func category(_ category: BookmarkGroup,
                       leadingAction: @escaping (UIView) -> Void,
                       accessoryAction: @escaping (UIView) -> Void) -> Self {
    BookmarksListCell.Configuration(title: category.title,
                                    subtitle: category.placesCountTitle(),
                                    leadingItem: .image(category.isVisible ? UIImage.icEyeOn : UIImage.icEyeOff,
                                                        tintColor: category.isVisible ? .linkBlue : .blackHintText,
                                                        action: leadingAction),
                                    trailingButtons: .one(.more(action: accessoryAction)))
  }

  static func action(_ action: BMCAction) -> Self {
    BookmarksListCell.Configuration(title: action.title,
                                    subtitle: nil,
                                    leadingItem: .image(action.image,
                                                        tintColor: .linkBlue,
                                                        action: nil),
                                    trailingButtons: .none)
  }
}
