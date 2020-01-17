final class BMCPermissionsPendingCell: MWMTableViewCell {
  @IBOutlet private weak var label: UILabel!

  @IBOutlet private weak var spinner: UIView!
  @IBOutlet private var spinnerBottom: NSLayoutConstraint!

  private var circularProgress: MWMCircularProgress!

  private var permission: BMCPermission! {
    didSet {
      switch permission! {
      case .signup:
        label.text = L("bookmarks_message_unauthorized_user")
        startSpinner(start: true)
      case .backup: assertionFailure()
      case let .restore(date):
        startSpinner(start: false)
        if let date = date {
          let formatter = DateFormatter()
          formatter.dateStyle = .short
          formatter.timeStyle = .none
          label.text = String(coreFormat: L("bookmarks_message_backuped_user"), arguments: [formatter.string(from: date)])
        } else {
          label.text = L("bookmarks_message_unbackuped_user")
        }
      }
    }
  }

  private func startSpinner(start: Bool) {
    if start {
      circularProgress = MWMCircularProgress(parentView: spinner)
      circularProgress.state = .spinner
      spinnerBottom.isActive = true
      spinner.isHidden = false
    } else {
      circularProgress = nil
      spinnerBottom.isActive = false
      spinner.isHidden = true
    }
  }

  func config(permission: BMCPermission) -> UITableViewCell {
    self.permission = permission
    isSeparatorHidden = true
    backgroundColor = .clear
    return self
  }
}
