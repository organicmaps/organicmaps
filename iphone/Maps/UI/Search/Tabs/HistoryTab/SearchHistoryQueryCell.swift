final class SearchHistoryQueryCell: MWMTableViewCell {
  @IBOutlet weak var queryLabel: UILabel!
  
  func update(with query: String) {
    queryLabel.text = query
  }
}
