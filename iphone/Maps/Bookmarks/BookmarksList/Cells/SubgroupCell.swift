final class SubgroupCell: UITableViewCell {
  @IBOutlet var subgroupTitleLabel: UILabel!
  @IBOutlet var subgroupSubtitleLabel: UILabel!
  @IBOutlet var subgroupVisibleMark: Checkmark!

  typealias CheckHandlerClosure = (Bool) -> Void
  var checkHandler: CheckHandlerClosure?

  func config(_ subgroup: ISubgroupViewModel) {
    subgroupTitleLabel.text = subgroup.subgroupName
    subgroupSubtitleLabel.text = subgroup.subtitle
    subgroupVisibleMark.isChecked = subgroup.isVisible
  }

  @IBAction func onCheck(_ sender: Checkmark) {
    checkHandler?(sender.isChecked)
  }
}
