final class ColorPicker: NSObject {
  static let shared = ColorPicker()

  private var pickerViewController: ColorGridViewController?
  // Strong reference: on macOS the "Show Colors..." button closes the picker and hands the
  // selection over to the system color panel, which keeps reporting through this picker's
  // delegate only while the picker is alive (FB8981193).
  private var nativeColorPickerViewController: UIColorPickerViewController?
  private var onUpdateColorHandler: ((UIColor) -> Void)?
  private var isIOSAppOnMac: Bool { ProcessInfo.processInfo.isiOSAppOnMac }
  private var nativePickedColor: UIColor?

  /// Presents the default bookmark/track color grid.
  /// `currentColor` preselects a matching color in the palette; pass nil when there is no
  /// current color (e.g. when selecting a color for the whole group).
  /// With an `anchor` the grid is always shown as a compact popover anchored to the tapped view.
  func present(from rootViewController: UIViewController,
               anchor: UIView?,
               currentColor: UIColor?,
               completionHandler: ((UIColor) -> Void)?) {
    nativeColorPickerViewController = nil
    let picker = ColorGridViewController(currentColor: currentColor,
                                         predefinedColors: BookmarksManager.predefinedColors().compactMap {
                                           PredefinedColor(rawValue: $0.intValue)
                                         }) { [weak self] color in
      self?.selectColor(color)
    } onSelectCustomColor: { [weak self] viewController in
      self?.presentNativeColorPicker(from: viewController, currentColor: currentColor)
    }
    picker.onDismiss = { [weak self] in
      self?.endColorPicker()
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
    // Animated presentation can cause visual artifacts on macOS; dismissal can remain animated.
    rootViewController.present(picker, animated: !isIOSAppOnMac)
  }

  private func selectColor(_ color: UIColor) {
    onUpdateColorHandler?(color)
    endColorPicker()
  }

  private func presentNativeColorPicker(from rootViewController: UIViewController, currentColor: UIColor?) {
    let picker = DismissReportingColorPickerViewController()
    picker.supportsAlpha = false
    if let currentColor {
      picker.selectedColor = currentColor
    }
    picker.delegate = self
    picker.onDismiss = { [weak self] in
      guard let self else { return }
      let didCommitSelection = self.commitNativeSelection()

      // On macOS the system color panel ("Show Colors...") keeps reporting through the
      // dismissed picker's delegate. On iOS a cancelled picker can be released immediately.
      if didCommitSelection {
        self.endColorPicker(keepingNativePicker: isIOSAppOnMac)
      } else if !isIOSAppOnMac {
        self.nativeColorPickerViewController = nil
      }
    }
    nativeColorPickerViewController = picker
    nativePickedColor = nil
    rootViewController.present(picker, animated: true)
  }

  @discardableResult
  private func commitNativeSelection() -> Bool {
    guard let nativePickedColor else { return false }

    self.nativePickedColor = nil
    onUpdateColorHandler?(nativePickedColor)
    return true
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

  private func closeColorGrid() {
    unsubscribeFromTraitChanges()
    let picker = pickerViewController
    pickerViewController = nil
    picker?.onDismiss = nil
    picker?.dismiss(animated: true)
  }

  private func endColorPicker(keepingNativePicker: Bool = false) {
    closeColorGrid()
    if !keepingNativePicker {
      nativeColorPickerViewController = nil
      onUpdateColorHandler = nil
    }
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
  func colorPickerViewController(_ viewController: UIColorPickerViewController, didSelect color: UIColor, continuously: Bool) {
    nativePickedColor = color.sRGBColor
    // On macOS colorPickerViewControllerDidFinish is never called, so commit and close
    // the picker as soon as the user clicks a color.
    if isIOSAppOnMac, !continuously {
      commitNativeSelection()
      viewController.dismiss(animated: true)
      closeColorGrid()
    }
  }
}

/// Reports the start of the picker's dismissal — the only signal delivered on every platform.
/// colorPickerViewControllerDidFinish is not called at all on macOS, and on iOS it is not
/// called for the interactive swipe-down dismissal.
private final class DismissReportingColorPickerViewController: UIColorPickerViewController {
  var onDismiss: (() -> Void)?

  override func viewWillDisappear(_ animated: Bool) {
    // Skip transient disappearances (e.g. the eyedropper): commit only on a real dismissal.
    if isBeingDismissed {
      onDismiss?()
    }
    super.viewWillDisappear(animated)
  }
}
