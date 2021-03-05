
class DrivingOptionsViewController: MWMTableViewController {
  let options = RoutingOptions()
  @IBOutlet var tollRoadsCell: SettingsTableViewSwitchCell!
  @IBOutlet var unpavedRoadsCell: SettingsTableViewSwitchCell!
  @IBOutlet var ferryCrossingsCell: SettingsTableViewSwitchCell!
  @IBOutlet var motorwaysCell: SettingsTableViewSwitchCell!

  override func viewDidLoad() {
    super.viewDidLoad()
    title = L("driving_options_title")
    tollRoadsCell.isOn = options.avoidToll
    unpavedRoadsCell.isOn = options.avoidDirty
    ferryCrossingsCell.isOn = options.avoidFerry
    motorwaysCell.isOn = options.avoidMotorway
  }

  override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
    return L("driving_options_subheader")
  }
}

extension DrivingOptionsViewController: SettingsTableViewSwitchCellDelegate {
  func switchCell(_ cell: SettingsTableViewSwitchCell, didChangeValue value: Bool) {
    if cell == tollRoadsCell {
      options.avoidToll = cell.isOn
    } else if cell == unpavedRoadsCell {
      options.avoidDirty = cell.isOn
    } else if cell == ferryCrossingsCell {
      options.avoidFerry = cell.isOn
    } else if cell == motorwaysCell {
      options.avoidMotorway = cell.isOn
    }

    options.save()
  }
}
