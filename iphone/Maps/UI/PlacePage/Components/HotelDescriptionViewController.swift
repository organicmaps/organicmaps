protocol HotelDescriptionViewControllerDelegate: AnyObject {
  func hotelDescriptionDidPressMore()
}

class HotelDescriptionViewController: UIViewController {
  @IBOutlet var descriptionExpandableLabel: ExpandableLabel!
  @IBOutlet var moreButton: UIButton!

  var hotelDescription: String? {
    didSet {
      descriptionExpandableLabel.text = hotelDescription
    }
  }
  weak var delegate: HotelDescriptionViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()

    descriptionExpandableLabel.numberOfLines = 3
    descriptionExpandableLabel.text = hotelDescription
    descriptionExpandableLabel.font = UIFont.regular16()
    descriptionExpandableLabel.textColor = UIColor.blackPrimaryText()
    descriptionExpandableLabel.expandColor = UIColor.linkBlue()
    descriptionExpandableLabel.expandText = L("booking_show_more")
  }

  @IBAction func onMoreButton(_ sender: UIButton) {
    delegate?.hotelDescriptionDidPressMore()
  }
}
