struct PopoverListSelectorBuilder {
  let dataSource: [PopoverListSelectorViewController.RowViewModel]
  let style: PopoverListSelectorViewController.Style
  let sourceView: UIView
  let sourceRect: CGRect
  let userInterfaceStyle: UIUserInterfaceStyle

  func build() -> PopoverListSelectorViewController {
    let viewController = PopoverListSelectorViewController(dataSource, style: style)
    viewController.modalPresentationStyle = .popover
    viewController.overrideUserInterfaceStyle = userInterfaceStyle
    viewController.popoverPresentationController?.sourceView = sourceView
    viewController.popoverPresentationController?.sourceRect = sourceRect
    viewController.popoverPresentationController?.permittedArrowDirections = .any
    viewController.popoverPresentationController?.delegate = viewController
    return viewController
  }
}
