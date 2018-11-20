protocol SharingPropertiesViewControllerDelegate: AnyObject {
  func sharingPropertiesViewController(_ viewController: SharingPropertiesViewController,
                                       didSelect userStatus: MWMCategoryAuthorType)
}

final class SharingPropertiesViewController: MWMTableViewController {
  weak var delegate: SharingPropertiesViewControllerDelegate?
  
  @IBOutlet weak var localButton: MWMButton! {
    didSet {
      localButton.setTitle(L("custom_props_local").uppercased(), for: .normal)
    }
  }
  
  @IBOutlet weak var travelerButton: MWMButton! {
    didSet {
      travelerButton.setTitle(L("custom_props_traveler").uppercased(), for: .normal)
    }
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = L("select_properties")
  }
  
  override func tableView(_ tableView: UITableView,
                          titleForHeaderInSection section: Int) -> String? {
    return section == 0 ? L("custom_props_title") : nil
  }
  
  @IBAction func localButtonWasPressed(_ sender: Any) {
    delegate?.sharingPropertiesViewController(self, didSelect: .local)
  }
  
  @IBAction func travelerButtonPressed(_ sender: Any) {
    delegate?.sharingPropertiesViewController(self, didSelect: .traveler)
  }
}
