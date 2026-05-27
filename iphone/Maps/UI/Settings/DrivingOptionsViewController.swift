
class DrivingOptionsViewController: MWMTableViewController {
  let options = RoutingOptions()
  @IBOutlet var tollRoadsCell: SettingsTableViewSwitchCell!
  @IBOutlet var unpavedRoadsCell: SettingsTableViewSwitchCell!
  @IBOutlet var ferryCrossingsCell: SettingsTableViewSwitchCell!
  @IBOutlet var motorwaysCell: SettingsTableViewSwitchCell!
  @IBOutlet var publicBikeSharingCell: SettingsTableViewSwitchCell!

  private var isBicycleRouter: Bool { MWMRouter.type() == .bicycle }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = L("driving_options_title")
    tollRoadsCell.isOn = options.avoidToll
    unpavedRoadsCell.isOn = options.avoidDirty
    ferryCrossingsCell.isOn = options.avoidFerry
    motorwaysCell.isOn = options.avoidMotorway

    if isBicycleRouter {
      publicBikeSharingCell.isOn = options.publicBicycle
      publicBikeSharingCell.isHidden = false
    } else {
      publicBikeSharingCell.isHidden = true
    }
  }

  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    if !isBicycleRouter, tableView.indexPath(for: publicBikeSharingCell) == indexPath {
      return 0
    } else {
      return UITableView.automaticDimension
    }
  }
}

extension DrivingOptionsViewController: SettingsTableViewSwitchCellDelegate {
  func switchCell(_ cell: SettingsTableViewSwitchCell, didChangeValue _: Bool) {
    if cell == tollRoadsCell {
      options.avoidToll = cell.isOn
    } else if cell == unpavedRoadsCell {
      options.avoidDirty = cell.isOn
    } else if cell == ferryCrossingsCell {
      options.avoidFerry = cell.isOn
    } else if cell == motorwaysCell {
      options.avoidMotorway = cell.isOn
    } else if cell == publicBikeSharingCell {
      options.publicBicycle = cell.isOn
    } else {
      return
    }

    options.save()
  }
}
