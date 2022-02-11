
final class SearchNoResultsViewController: MWMViewController {

  static var controller: SearchNoResultsViewController {
    let storyboard = UIStoryboard.instance(.main)
    return storyboard.instantiateViewController(withIdentifier: toString(self)) as! SearchNoResultsViewController
  }

  @IBOutlet private weak var container: UIView!
  @IBOutlet fileprivate weak var containerBottomOffset: NSLayoutConstraint!

  override func viewDidLoad() {
    super.viewDidLoad()

    container.addSubview(MWMSearchNoResults.view(with: nil,
                                                 title: L("search_not_found"),
                                                 text: L("search_not_found_query")))
  }
}
