import AVFoundation

class MWMTTSLanguageViewController: MWMTableViewController {
  private var languages: [Language] = []
  private var selectedRow: Int?
  
  override func viewDidLoad() {
    languages = MWMTextToSpeech.ttsLanguages().map { identifier in
      Language(identifier: identifier)
    }.sorted()
    
    self.title = L("pref_tts_language_title")
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return languages.count
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(withIdentifier: "SettingsTableViewSelectableCell", for: indexPath)
    as! SettingsTableViewSelectableCell
    
    cell.config(title: languages[indexPath.row].localizedName)
    cell.accessoryType = .none
    
    if UserDefaults.standard.ttsLanguage == languages[indexPath.row].identifier {
      cell.accessoryType = .checkmark
      selectedRow = indexPath.row
    }
    
    return cell
  }
  
  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: false)
    
    if selectedRow == indexPath.row {
      return
    }
    
    if let row = selectedRow, let previousCell = tableView.cellForRow(at: IndexPath(row: row, section: indexPath.section)) {
      previousCell.accessoryType = .none
    }
    
    if let cell = tableView.cellForRow(at: indexPath) {
      cell.accessoryType = .checkmark
    }
    
    selectedRow = indexPath.row
    
    UserDefaults.standard.ttsLanguage = languages[indexPath.row].identifier
  }
  
  private struct Language: Equatable, Comparable {
    let identifier: String
    var localizedName: String {
      return Locale(identifier: identifier).localizedString(forIdentifier: identifier) ?? identifier
    }
    
    static func < (lhs: Language, rhs: Language) -> Bool {
      lhs.localizedName < rhs.localizedName
    }
  }
}

extension UserDefaults {
  static let ttsLanguageKey = "UserDefaultsTTSLanguageBcp47"
  
  @objc dynamic var ttsLanguage: String {
    get { return string(forKey: UserDefaults.ttsLanguageKey) ?? Locale.preferredLanguages.first ?? "en-UK" }
    set { set(newValue, forKey: UserDefaults.ttsLanguageKey) }
  }
}
