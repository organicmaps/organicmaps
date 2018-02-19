protocol BMCPermissionsCellDelegate {
  func permissionAction(permission: BMCPermission, anchor: UIView)
}

final class BMCPermissionsCell: MWMTableViewCell {
  @IBOutlet private weak var label: UILabel! {
    didSet {
      label.font = .regular14()
      label.textColor = .blackSecondaryText()
    }
  }

  @IBOutlet private weak var button: UIButton! {
    didSet {
      button.setTitleColor(.white, for: .normal)
      button.setBackgroundColor(.linkBlue(), for: .normal)
      button.setBackgroundColor(.linkBlueHighlighted(), for: .highlighted)
      button.titleLabel?.font = .regular14()
      button.layer.cornerRadius = 6
      button.clipsToBounds = true
    }
  }

  private var permission: BMCPermission! {
    didSet {
      switch permission! {
      case .signup:
        label.text = L("bookmarks_message_unauthorized_user")
        button.setTitle(L("authorization_button_sign_in").uppercased(), for: .normal)
      case .backup:
        label.text = L("bookmarks_message_authorized_user")
        button.setTitle(L("bookmarks_backup").uppercased(), for: .normal)
      case .restore: assertionFailure()
      }
    }
  }

  private var delegate: BMCPermissionsCellDelegate!

  func config(permission: BMCPermission, delegate: BMCPermissionsCellDelegate) -> UITableViewCell {
    self.permission = permission
    self.delegate = delegate
    isSeparatorHidden = true
    backgroundColor = .clear
    return self
  }

  @IBAction private func buttonAction() {
    delegate.permissionAction(permission: permission, anchor: button)
  }
}
