final class ColorPicker: NSObject {
  static let shared = ColorPicker()

  // Strong reference: on macOS the "Show Colors…" button closes the picker and hands the
  // selection over to the system color panel, which keeps reporting through this picker's
  // delegate only while the picker is alive (FB8981193).
  private var pickerViewController: UIColorPickerViewController?
  private var onUpdateColorHandler: ((UIColor) -> Void)?
  private var pickedColor: UIColor?

  /// Presents the native arbitrary-color picker. Used for both bookmarks and tracks.
  /// `currentColor` preselects the color in the palette; pass nil when there is no current
  /// color (e.g. when selecting a color for the whole group).
  /// With an `anchor` the picker is shown as a popover: it adapts to a sheet on iPhone and
  /// becomes a compact panel anchored to the tapped view on iPad and macOS.
  func present(from rootViewController: UIViewController,
               anchor: UIView?,
               currentColor: UIColor?,
               completionHandler: ((UIColor) -> Void)?) {
    let picker = DismissReportingColorPickerViewController()
    picker.supportsAlpha = false
    if let currentColor {
      picker.selectedColor = currentColor
    }
    picker.delegate = self
    picker.onDismiss = { [weak self] in
      guard let self else { return }
      self.commitSelection()
      // On macOS the system color panel ("Show Colors…") keeps reporting through the
      // dismissed picker's delegate; on iOS the picker is done once dismissed.
      if !ProcessInfo.processInfo.isiOSAppOnMac {
        self.pickerViewController = nil
        self.onUpdateColorHandler = nil
      }
    }
    if let anchor {
      picker.modalPresentationStyle = .popover
      picker.popoverPresentationController?.sourceView = anchor
      picker.popoverPresentationController?.sourceRect = anchor.bounds
    }
    pickerViewController = picker
    pickedColor = nil
    onUpdateColorHandler = completionHandler
    rootViewController.present(picker, animated: true)
  }

  /// Commits only an explicitly picked color: closing the picker without selecting
  /// anything should not recolor the target.
  private func commitSelection() {
    guard let pickedColor else { return }
    self.pickedColor = nil
    onUpdateColorHandler?(pickedColor)
  }
}

// MARK: - UIColorPickerViewControllerDelegate

extension ColorPicker: UIColorPickerViewControllerDelegate {
  func colorPickerViewController(_ viewController: UIColorPickerViewController, didSelect color: UIColor, continuously: Bool) {
    pickedColor = color.sRGBColor
    // On macOS colorPickerViewControllerDidFinish is never called, so commit and close
    // the picker as soon as the user clicks a color.
    if ProcessInfo.processInfo.isiOSAppOnMac, !continuously {
      commitSelection()
      viewController.dismiss(animated: true)
    }
  }
}

/// Reports the actual dismissal: the only signal delivered on every platform.
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
