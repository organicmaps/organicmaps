@objc
protocol CategorySettingsViewControllerDelegate: AnyObject {
  func categorySettingsController(_ viewController: CategorySettingsViewController,
                                  didEndEditing categoryId: MWMMarkGroupID)
  func categorySettingsController(_ viewController: CategorySettingsViewController,
                                  didDelete categoryId: MWMMarkGroupID)
}

class CategorySettingsViewController: MWMTableViewController {
  
  @objc var category: MWMCategory!
  @objc var maxCategoryNameLength: UInt = 60
  @objc var minCategoryNameLength: UInt = 0
  private var changesMade = false
  
  var manager: MWMBookmarksManager {
    return MWMBookmarksManager.shared()
  }
  
  @objc weak var delegate: CategorySettingsViewControllerDelegate?
  
  @IBOutlet private weak var nameTextField: UITextField!
  @IBOutlet private weak var descriptionTextView: UITextView!
  @IBOutlet private weak var descriptionCell: UITableViewCell!
  @IBOutlet private weak var saveButton: UIBarButtonItem!
  @IBOutlet private weak var deleteListButton: UIButton!
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    assert(category != nil, "must provide category info")

    title = L("list_settings")
    deleteListButton.isEnabled = (manager.userCategories().count > 1)
    nameTextField.text = category.title
    descriptionTextView.text = category.description
    
    navigationItem.rightBarButtonItem = saveButton
  }
  
  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    if isMovingFromParentViewController && !changesMade {
      Statistics.logEvent(kStatBookmarkSettingsCancel)
    }
  }
  
  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch section {
    case 0:
      // if there are no bookmarks in category, hide 'sharing options' row
      return category.isEmpty ? 1 : 2
    default:
      return 1
    }
  }
  
  @IBAction func deleteListButtonPressed(_ sender: Any) {
    manager.deleteCategory(category.categoryId)
    delegate?.categorySettingsController(self, didDelete: category.categoryId)
    Statistics.logEvent(kStatBookmarkSettingsClick,
                        withParameters: [kStatOption : kStatDelete])
  }
  
  override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
    if let destinationVC = segue.destination as? BookmarksSharingViewController {
      destinationVC.category = category
      Statistics.logEvent(kStatBookmarkSettingsClick,
                          withParameters: [kStatOption : kStatSharingOptions])
    }
  }
  
  @IBAction func onSave(_ sender: Any) {
    guard let newName = nameTextField.text, !newName.isEmpty else {
      assert(false)
      return
    }

    category.title = newName
    category.detailedAnnotation = descriptionTextView.text
    changesMade = true
    delegate?.categorySettingsController(self, didEndEditing: category.categoryId)
    Statistics.logEvent(kStatBookmarkSettingsConfirm)
  }
  
  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return UITableViewAutomaticDimension
  }
}

extension CategorySettingsViewController: UITextViewDelegate {
  func textViewDidChange(_ textView: UITextView) {
    let size = textView.bounds.size
    let newSize = textView.sizeThatFits(CGSize(width: size.width,
                                               height: CGFloat.greatestFiniteMagnitude))
    
    // Resize the cell only when cell's size is changed
    if abs(size.height - newSize.height) >= 1 {
      UIView.setAnimationsEnabled(false)
      tableView.beginUpdates()
      tableView.endUpdates()
      UIView.setAnimationsEnabled(true)
      
      if let thisIndexPath = tableView.indexPath(for: descriptionCell) {
        tableView?.scrollToRow(at: thisIndexPath, at: .bottom, animated: false)
      }
    }
  }
  
  func textViewDidBeginEditing(_ textView: UITextView) {
    Statistics.logEvent(kStatBookmarkSettingsClick,
                        withParameters: [kStatOption : kStatAddDescription])
  }
}

extension CategorySettingsViewController: UITextFieldDelegate {
  func textField(_ textField: UITextField, shouldChangeCharactersIn range: NSRange,
                 replacementString string: String) -> Bool {
    let currentText = textField.text ?? ""
    guard let stringRange = Range(range, in: currentText) else { return false }
    let updatedText = currentText.replacingCharacters(in: stringRange, with: string)
    saveButton.isEnabled = updatedText.count > minCategoryNameLength
    if updatedText.count > maxCategoryNameLength { return false }
    return true
  }
  
  func textFieldShouldClear(_ textField: UITextField) -> Bool {
    saveButton.isEnabled = false
    return true
  }
}
