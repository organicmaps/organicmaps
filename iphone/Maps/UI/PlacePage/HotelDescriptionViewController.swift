protocol HotelDescriptionViewControllerDelegate: AnyObject {
  func hotelDescriptionDidPressMore()
}

class HotelDescriptionViewController: UIViewController {
  @IBOutlet var descriptionExpandableLabel: ExpandableLabel!
  @IBOutlet var moreButton: UIButton!

  var hotelDescription: String? {
    didSet {
      descriptionExpandableLabel.textLabel.text = hotelDescription
    }
  }
  weak var delegate: HotelDescriptionViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()

    descriptionExpandableLabel.textLabel.numberOfLines = 3
    descriptionExpandableLabel.textLabel.text = hotelDescription
    descriptionExpandableLabel.textLabel.font = UIFont.regular16()
    descriptionExpandableLabel.textLabel.textColor = UIColor.blackPrimaryText()
    descriptionExpandableLabel.expandButton.setTitleColor(UIColor.linkBlue(), for: .normal)
    descriptionExpandableLabel.expandButton.titleLabel?.font = UIFont.regular16()
    descriptionExpandableLabel.expandButton.setTitle(L("booking_show_more"), for: .normal)
  }

  @IBAction func onMoreButton(_ sender: UIButton) {
    delegate?.hotelDescriptionDidPressMore()
  }
}
