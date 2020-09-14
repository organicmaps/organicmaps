protocol GuideSharingNameViewControllerDelegate {
  func viewController(_ viewController: GuideSharingNameViewController, didFinishEditing text: String)
}

class GuideSharingNameViewController: MWMTableViewController {
  @IBOutlet weak var nextBarButton: UIBarButtonItem!
  @IBOutlet weak var nameTextField: UITextField!

  var guideName: String?
  var delegate: GuideSharingNameViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()
    title = L("name")
    nameTextField.placeholder = L("name_placeholder")
    nameTextField.text = guideName
    nameTextField.becomeFirstResponder()
    nextBarButton.title = L("whats_new_next_button")
    nextBarButton.isEnabled = (nameTextField.text?.count ?? 0) > 0
  }

  override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
    return L("name_title")
  }

  override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
    return L("name_comment1") + "\n\n" + L("name_comment2")
  }

  @IBAction func onEditName(_ sender: UITextField) {
    let length = sender.text?.count ?? 0
    nextBarButton.isEnabled = length > 0
  }

  @IBAction func onNext(_ sender: UIBarButtonItem) {
    delegate?.viewController(self, didFinishEditing: nameTextField.text ?? "")
  }
}

extension GuideSharingNameViewController: UITextFieldDelegate {
  func textField(_ textField: UITextField,
                 shouldChangeCharactersIn range: NSRange,
                 replacementString string: String) -> Bool {
    guard let text = textField.text as NSString? else { return true }
    let resultText = text.replacingCharacters(in: range, with: string)
    return resultText.count <= 42
  }
}
