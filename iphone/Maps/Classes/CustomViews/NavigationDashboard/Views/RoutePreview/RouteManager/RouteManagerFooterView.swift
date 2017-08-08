final class RouteManagerFooterView: UIView {
  @IBOutlet private weak var cancelButton: UIButton! {
    didSet {
      cancelButton.setTitle(L("cancel"), for: .normal)
      cancelButton.setTitleColor(UIColor.linkBlue(), for: .normal)
    }
  }
  @IBOutlet private weak var planButton: UIButton! {
    didSet {
      planButton.setTitle(L("button_plan"), for: .normal)
      planButton.setTitleColor(UIColor.linkBlue(), for: .normal)
      planButton.setTitleColor(UIColor.buttonDisabledBlueText(), for: .disabled)
    }
  }
  @IBOutlet weak var separator: UIView! {
    didSet {
      separator.backgroundColor = UIColor.blackDividers()
    }
  }

  var isPlanButtonEnabled = true {
    didSet {
      planButton.isEnabled = isPlanButtonEnabled
    }
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    backgroundColor = UIColor.blackOpaque()
  }
}
