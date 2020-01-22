
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
  
  func logChangeEvent() {
    let viewControllers = self.navigationController?.viewControllers ?? []
    var openFrom = kStatSettings
    if (viewControllers.dropLast().last as? MapViewController) != nil {
      openFrom = kStatRoute
    }
    Statistics.logEvent(kStatDrivingOptionsChange,
                        withParameters: [kStatFrom: openFrom,
                                         kStatToll: options.avoidToll ? 1 : 0,
                                         kStatUnpaved: options.avoidDirty ? 1 : 0,
                                         kStatFerry: options.avoidFerry ? 1 : 0,
                                         kStatMotorway: options.avoidMotorway ? 1 : 0])
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
    logChangeEvent()
  }
}
