protocol BMCPermissionsCellDelegate: AnyObject {
  func permissionAction(permission: BMCPermission, anchor: UIView)
}

final class BMCPermissionsCell: MWMTableViewCell {
  @IBOutlet private weak var label: UILabel!

  @IBOutlet private weak var button: UIButton!

  private var permission: BMCPermission! {
    didSet {
      switch permission! {
      case .signup:
        label.text = L("bookmarks_message_unauthorized_user")
        button.setTitle(L("authorization_button_sign_in").uppercased(), for: .normal)
      case .backup:
        label.text = L("bookmarks_message_authorized_user")
        button.setTitle(L("bookmarks_backup").uppercased(), for: .normal)
      case let .restore(date):
        if let date = date {
          let formatter = DateFormatter()
          formatter.dateStyle = .short
          formatter.timeStyle = .none
          label.text = String(coreFormat: L("bookmarks_message_backuped_user"),
                              arguments: [formatter.string(from: date)])
        } else {
          label.text = L("bookmarks_message_unbackuped_user")
        }

        button.setTitle(L("bookmarks_restore"), for: .normal)
      }
    }
  }

  private weak var delegate: BMCPermissionsCellDelegate?

  func config(permission: BMCPermission, delegate: BMCPermissionsCellDelegate) -> UITableViewCell {
    self.permission = permission
    self.delegate = delegate
    isSeparatorHidden = true
    backgroundColor = .clear
    return self
  }

  @IBAction private func buttonAction() {
    delegate?.permissionAction(permission: permission, anchor: button)
  }
}
