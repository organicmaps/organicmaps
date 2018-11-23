protocol CategorySettingsViewControllerDelegate: AnyObject {
  func categorySettingsController(_ viewController: CategorySettingsViewController,
                                  didEndEditing categoryId: MWMMarkGroupID)
  func categorySettingsController(_ viewController: CategorySettingsViewController,
                                  didDelete categoryId: MWMMarkGroupID)
}

class CategorySettingsViewController: MWMTableViewController {
  
  var categoryId: MWMMarkGroupID?
  var maxCategoryNameLength: UInt?
  var minCategoryNameLength: UInt?
  
  var manager: MWMBookmarksManager {
    return MWMBookmarksManager.shared()
  }
  
  weak var delegate: CategorySettingsViewControllerDelegate?
  
  @IBOutlet private weak var accessStatusLabel: UILabel!
  @IBOutlet private weak var nameTextField: UITextField!
  @IBOutlet private weak var descriptionTextView: UITextView!
  @IBOutlet private weak var descriptionCell: UITableViewCell!
  @IBOutlet private weak var saveButton: UIBarButtonItem!
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = L("settings")
    
    assert(categoryId != nil && minCategoryNameLength != nil && maxCategoryNameLength != nil,
           "must provide category info")

    nameTextField.text = manager.getCategoryName(categoryId!)
    descriptionTextView.text = manager.getCategoryDescription(categoryId!)
    configureAccessStatus()
    
    navigationItem.rightBarButtonItem = saveButton
  }
  
  func configureAccessStatus() {
    switch MWMBookmarksManager.shared().getCategoryAccessStatus(categoryId!) {
    case .local:
      accessStatusLabel.text = L("not_shared")
    case .public:
      accessStatusLabel.text = L("bookmarks_public_access")
    case .private:
      accessStatusLabel.text = L("bookmarks_link_access")
    case .other:
      assert(false, "it's not ok that this category has such access status")
    }
  }
  
  @IBAction func deleteListButtonPressed(_ sender: Any) {
    guard let category = categoryId else {
      assert(false)
      return
    }
    
    manager.deleteCategory(category)
    delegate?.categorySettingsController(self, didDelete: category)
  }
  
  override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
    if let destinationVC = segue.destination as? BookmarksSharingViewController {
      destinationVC.categoryId = categoryId
      destinationVC.delegate = self
    }
  }
  
  @IBAction func onSave(_ sender: Any) {
    guard let category = categoryId,
      let newName = nameTextField.text,
      !newName.isEmpty else {
        assert(false)
        return
    }
    
    manager.setCategory(category, name: newName)
    manager.setCategory(category, description: descriptionTextView.text)
    delegate?.categorySettingsController(self, didEndEditing: category)
  }
  
  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return UITableViewAutomaticDimension
  }
}

extension CategorySettingsViewController: BookmarksSharingViewControllerDelegate {
  func didShareCategory() {
    configureAccessStatus()
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
}

extension CategorySettingsViewController: UITextFieldDelegate {
  func textField(_ textField: UITextField, shouldChangeCharactersIn range: NSRange,
                 replacementString string: String) -> Bool {
    let currentText = textField.text ?? ""
    
    guard let stringRange = Range(range, in: currentText) else { return false }
    let updatedText = currentText.replacingCharacters(in: stringRange, with: string)
    
    guard let maxAvailableLength = maxCategoryNameLength,
          let minAvailableLength = minCategoryNameLength else {
            assert(false)
            return true
    }
    
    saveButton.isEnabled = updatedText.count > minAvailableLength
    
    if updatedText.count > maxAvailableLength {
        return false
    }
    return true
  }
  
  func textFieldShouldClear(_ textField: UITextField) -> Bool {
    saveButton.isEnabled = false
    return true
  }
}
