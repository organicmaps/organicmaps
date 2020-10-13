final class SubgroupCell: UITableViewCell {
  @IBOutlet private var subgroupTitleLabel: UILabel!
  @IBOutlet private var subgroupSubtitleLabel: UILabel!
  @IBOutlet private var subgroupVisibleMark: Checkmark!

  typealias CheckHandlerClosure = (Bool) -> Void
  var checkHandler: CheckHandlerClosure?

  func config(_ subgroup: ISubgroupViewModel) {
    subgroupTitleLabel.text = subgroup.subgroupName
    subgroupSubtitleLabel.text = subgroup.subtitle
    subgroupVisibleMark.isChecked = subgroup.isVisible
  }

  @IBAction private func onCheck(_ sender: Checkmark) {
    checkHandler?(sender.isChecked)
  }
}
