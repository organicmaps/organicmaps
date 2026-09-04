final class BookmarksListSubgroupCell: UITableViewCell {
  @IBOutlet private var subgroupTitleLabel: UILabel!
  @IBOutlet private var subgroupSubtitleLabel: UILabel!
  @IBOutlet private var subgroupVisibleMark: Checkmark!
  @IBOutlet private var subgroupDisclosureImageView: UIImageView!

  typealias CheckHandlerClosure = (Bool) -> Void
  var checkHandler: CheckHandlerClosure?

  override func layoutSubviews() {
    super.layoutSubviews()
    updateSeparatorInset(leading: separatorLeadingInset, trailing: separatorTrailingInset)
  }

  private var separatorLeadingInset: CGFloat {
    leadingInset(of: subgroupTitleLabel)
  }

  private var separatorTrailingInset: CGFloat {
    trailingInset(of: subgroupDisclosureImageView)
  }

  func config(_ subgroup: ISubgroupViewModel) {
    subgroupTitleLabel.text = subgroup.subgroupName
    subgroupSubtitleLabel.text = subgroup.subtitle
    subgroupVisibleMark.isChecked = subgroup.isVisible
  }

  @IBAction private func onCheck(_ sender: Checkmark) {
    checkHandler?(sender.isChecked)
  }
}
