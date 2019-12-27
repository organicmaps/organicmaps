@objc(MWMFacilitiesController)
final class FacilitiesController: MWMTableViewController {
  @objc var name: String?
  @objc var facilities: [HotelFacility]?

  override func viewDidLoad() {
    super.viewDidLoad()

    tableView.estimatedRowHeight = 44
    tableView.rowHeight = UITableView.automaticDimension
    tableView.register(cell: FacilityCell.self)

    title = name
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    facilities?.count ?? 0
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: FacilityCell.self)!
    cell.config(with: facilities![indexPath.row].name)
    return cell
  }
}

