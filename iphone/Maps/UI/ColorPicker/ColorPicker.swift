final class ColorPicker: NSObject {
  static let shared = ColorPicker()

  private var pickerViewController: UIViewController?
  private var onUpdateColorHandler: ((UIColor) -> Void)?
  private var nativePickedColor: UIColor?

  /// Presents the default bookmark/track color grid.
  /// `currentColor` preselects a matching color in the palette; pass nil when there is no
  /// current color (e.g. when selecting a color for the whole group).
  /// With an `anchor` the grid is always shown as a compact popover anchored to the tapped view.
  func present(from rootViewController: UIViewController,
               anchor: UIView?,
               currentColor: UIColor?,
               completionHandler: ((UIColor) -> Void)?) {
    let picker = ColorGridViewController(currentColor: currentColor,
                                         predefinedColors: BookmarksManager.predefinedColors().compactMap {
                                           PredefinedColor(rawValue: $0.intValue)
                                         }) { [weak self] color in
      self?.commitSelection(color)
    } onSelectCustomColor: { [weak self] viewController in
      self?.presentNativeColorPicker(from: viewController, currentColor: currentColor)
    }
    picker.onDismiss = { [weak self] in
      self?.resetPresentationState()
    }
    picker.modalPresentationStyle = .popover
    if let popover = picker.popoverPresentationController {
      let sourceView: UIView = anchor ?? rootViewController.view
      popover.sourceView = sourceView
      popover.sourceRect = anchor?.bounds ?? CGRect(x: sourceView.bounds.midX,
                                                    y: sourceView.bounds.midY,
                                                    width: 1,
                                                    height: 1)
      popover.permittedArrowDirections = .any
      popover.delegate = picker
    }

    pickerViewController = picker
    onUpdateColorHandler = completionHandler
    updatePopoverUserInterfaceStyle(rootViewController.traitCollection.userInterfaceStyle)
    subscribeToTraitChanges()
    rootViewController.present(picker, animated: true)
  }

  private func commitSelection(_ color: UIColor) {
    let onUpdateColorHandler = onUpdateColorHandler
    self.onUpdateColorHandler = nil
    onUpdateColorHandler?(color)
    pickerViewController?.dismiss(animated: true)
    resetPresentationState()
  }

  private func presentNativeColorPicker(from rootViewController: UIViewController, currentColor: UIColor?) {
    let picker = UIColorPickerViewController()
    picker.supportsAlpha = false
    if let currentColor {
      picker.selectedColor = currentColor
    }
    picker.delegate = self
    picker.presentationController?.delegate = self
    nativePickedColor = nil
    rootViewController.present(picker, animated: true)
  }

  private func finishNativeColorPicker() {
    guard let nativePickedColor else { return }

    self.nativePickedColor = nil
    commitSelection(nativePickedColor)
  }

  private func subscribeToTraitChanges() {
    NotificationCenter.default.addObserver(self,
                                           selector: #selector(userInterfaceStyleDidChange(_:)),
                                           name: .userInterfaceStyleDidChange,
                                           object: nil)
  }

  private func unsubscribeFromTraitChanges() {
    NotificationCenter.default.removeObserver(self,
                                              name: .userInterfaceStyleDidChange,
                                              object: nil)
  }

  @objc private func userInterfaceStyleDidChange(_ notification: Notification) {
    guard let rawValue = notification.userInfo?[kUserInterfaceStyleKey] as? Int,
          let userInterfaceStyle = UIUserInterfaceStyle(rawValue: rawValue) else {
      return
    }
    updatePopoverUserInterfaceStyle(userInterfaceStyle)
  }

  private func updatePopoverUserInterfaceStyle(_ userInterfaceStyle: UIUserInterfaceStyle) {
    pickerViewController?.overrideUserInterfaceStyle = userInterfaceStyle
  }

  private func resetPresentationState() {
    unsubscribeFromTraitChanges()
    pickerViewController = nil
    onUpdateColorHandler = nil
  }
}

extension ColorGridViewController: UIPopoverPresentationControllerDelegate {
  func adaptivePresentationStyle(for _: UIPresentationController) -> UIModalPresentationStyle {
    .none
  }

  func presentationControllerDidDismiss(_: UIPresentationController) {
    onDismiss?()
  }
}

extension ColorPicker: UIColorPickerViewControllerDelegate {
  func colorPickerViewController(_: UIColorPickerViewController, didSelect color: UIColor, continuously _: Bool) {
    nativePickedColor = color.sRGBColor
  }

  func colorPickerViewControllerDidFinish(_: UIColorPickerViewController) {
    finishNativeColorPicker()
  }
}

extension ColorPicker: UIAdaptivePresentationControllerDelegate {
  func presentationControllerDidDismiss(_ presentationController: UIPresentationController) {
    guard presentationController.presentedViewController is UIColorPickerViewController else {
      return
    }
    // Swipe-down dismissal does not call colorPickerViewControllerDidFinish(_:).
    finishNativeColorPicker()
  }
}
