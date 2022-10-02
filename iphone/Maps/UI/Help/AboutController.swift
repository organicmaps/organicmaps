import Foundation

final class AboutController: MWMViewController, UITableViewDataSource, UITableViewDelegate {

  // Returns a human-readable maps data version.
  static private func formattedMapsDataVersion() -> String {
    // First, convert version code like 220131 to a date.
    let df = DateFormatter()
    df.locale = Locale(identifier:"en_US_POSIX")
    df.dateFormat = "yyMMdd"
    let mapsVersionInt = FrameworkHelper.dataVersion()
    let mapsDate = df.date(from: String(mapsVersionInt))!
    // Second, print the date in the local user's format.
    df.locale = Locale.current
    df.dateStyle = .long
    df.timeStyle = .none
    return String(format: L("data_version"), df.string(from:mapsDate), mapsVersionInt)
  }

  override func loadView() {
    super.loadView()

    title = L("about_menu_title")

    // Menu items.
    var tableStyle: UITableView.Style
    if #available(iOS 13.0, *) {
      tableStyle = .insetGrouped
    } else {
      tableStyle = .grouped
    }
    let table = UITableView(frame: CGRect.zero, style: tableStyle)
    // Default grey background for table view sections.
    table.setValue("TableView:PressBackground", forKey: "styleName")
    // For full-width cell separators.
    table.separatorInset = .zero
    table.translatesAutoresizingMaskIntoConstraints = false
    table.dataSource = self
    table.delegate = self
    table.sectionHeaderHeight = UITableView.automaticDimension

    view.addSubview(table)
    NSLayoutConstraint.activate([
        table.leadingAnchor.constraint(equalTo: view.leadingAnchor),
        table.trailingAnchor.constraint(equalTo: view.trailingAnchor),
        table.topAnchor.constraint(equalTo: view.topAnchor),
        table.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }

  private lazy var header: UIView = { () -> UIView in
    // Setup header view with app's and data versions.
    let header = UIView()
    header.translatesAutoresizingMaskIntoConstraints = false
    let kMargin = 20.0

    // App icon.
    // TODO: Reduce memory cache footprint by loading the icon without caching.
    let icon = UIImageView(image: UIImage(named: "AppIcon60x60"))
    icon.layer.cornerRadius = icon.width / 4
    icon.clipsToBounds = true
    icon.translatesAutoresizingMaskIntoConstraints = false
    header.addSubview(icon)
    NSLayoutConstraint.activate([
      icon.leadingAnchor.constraint(equalTo: header.leadingAnchor, constant: kMargin),
      icon.topAnchor.constraint(equalTo: header.topAnchor, constant: kMargin),
      icon.widthAnchor.constraint(equalTo: icon.heightAnchor)
    ])

    // App version.
    let appVersion = CopyableLabel()
    appVersion.translatesAutoresizingMaskIntoConstraints = false
    appVersion.styleName = "blackPrimaryText"
    appVersion.adjustsFontSizeToFitWidth = true
    let appInfo = AppInfo.shared();
    // Use strong left-to-right unicode direction characters for the app version.
    appVersion.text = String(format: L("version"), "\u{2066}\(appInfo.bundleVersion)-\(appInfo.buildNumber)\u{2069}")
    header.addSubview(appVersion)
    NSLayoutConstraint.activate([
      appVersion.leadingAnchor.constraint(equalTo: icon.trailingAnchor, constant: kMargin),
      appVersion.topAnchor.constraint(equalTo: header.topAnchor, constant: kMargin),
      appVersion.trailingAnchor.constraint(equalTo: header.trailingAnchor, constant: -kMargin)
    ])

    // Maps data version.
    let mapsVersion = CopyableLabel()
    mapsVersion.translatesAutoresizingMaskIntoConstraints = false
    mapsVersion.styleName = "blackSecondaryText"
    mapsVersion.adjustsFontSizeToFitWidth = true
    mapsVersion.text = AboutController.formattedMapsDataVersion()
    header.addSubview(mapsVersion)
    NSLayoutConstraint.activate([
      mapsVersion.leadingAnchor.constraint(equalTo: icon.trailingAnchor, constant: kMargin),
      mapsVersion.trailingAnchor.constraint(equalTo: header.trailingAnchor, constant: -kMargin),
      mapsVersion.topAnchor.constraint(equalTo: appVersion.bottomAnchor, constant: kMargin)
    ])

    // Long description text.
    let about = UILabel()
    about.translatesAutoresizingMaskIntoConstraints = false
    about.styleName = "blackPrimaryText"
    about.numberOfLines = 0
    about.text = L("about_description")
    header.addSubview(about)
    NSLayoutConstraint.activate([
      about.leadingAnchor.constraint(equalTo: header.leadingAnchor, constant: kMargin),
      about.bottomAnchor.constraint(equalTo: header.bottomAnchor, constant: -kMargin),
      about.trailingAnchor.constraint(equalTo: header.trailingAnchor, constant: -kMargin),
      about.topAnchor.constraint(equalTo: icon.bottomAnchor, constant: kMargin),
      about.topAnchor.constraint(equalTo: mapsVersion.bottomAnchor, constant: kMargin)
    ])
    return header
  }()

  // MARK: - UITableView data source

  // Update didSelect... delegate and tools/python/clean_strings_txt.py after modifying this list.
  private let labels = [
    ["news", "faq", "report_a_bug", "how_to_support_us", "rate_the_app"],
    ["telegram", "github", "website", "email", "facebook", "twitter", "instagram", "matrix", "openstreetmap"],
    ["privacy_policy", "terms_of_use", "copyright"],
  ]

  // Additional section is used to properly resize the header view by putting it in the table cell.
  func numberOfSections(in tableView: UITableView) -> Int { return labels.count + 1 }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    if section == 0 {
      return 1
    }
    return labels[section - 1].count
  }

  private func getCell(tableView: UITableView, identifier: String) -> UITableViewCell {
    guard let cell = tableView.dequeueReusableCell(withIdentifier: identifier) else {
      return UITableViewCell(style: .default, reuseIdentifier: identifier)
    }
    return cell
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    var cell: UITableViewCell
    if indexPath[0] == 0 {
      cell = getCell(tableView: tableView, identifier: "header")
      cell.contentView.addSubview(header)
      NSLayoutConstraint.activate([
        header.topAnchor.constraint(equalTo: cell.contentView.topAnchor),
        header.leadingAnchor.constraint(equalTo: cell.contentView.leadingAnchor),
        header.trailingAnchor.constraint(equalTo: cell.contentView.trailingAnchor),
        header.bottomAnchor.constraint(equalTo: cell.contentView.bottomAnchor)
      ])
    } else {
      cell = getCell(tableView: tableView, identifier: "default")
      cell.textLabel!.text = L(labels[indexPath[0] - 1][indexPath[1]])
    }
    return cell
  }

  // MARK: - UITableView delegate

  private let kiOSEmail = "ios@organicmaps.app"

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    // See labels array above.
    switch indexPath[0] {
      // Header section click.
      case 0: self.openUrl("https://organicmaps.app/donate/")
      // First buttons section.
      case 1: switch indexPath[1] {
        case 0: self.openUrl("https://organicmaps.app/news/")
        case 1: self.navigationController?.pushViewController(FaqController(), animated: true)
        case 2: sendEmailWith(header: "Organic Maps Bugreport", toRecipients: [kiOSEmail])
        case 3: self.openUrl("https://organicmaps.app/support-us/")
        case 4: UIApplication.shared.rateApp()
        default: fatalError("Invalid cell0 \(indexPath)")
      }
      // Second section. Open urls in external Safari so logged-in users can easily follow us.
      case 2: switch indexPath[1] {
        case 0: self.openUrl(L("telegram_url"), inSafari: true)
        case 1: self.openUrl("https://github.com/organicmaps/organicmaps/", inSafari: true)
        case 2: self.openUrl("https://organicmaps.app/")
        case 3: sendEmailWith(header: "Organic Maps", toRecipients: [kiOSEmail])
        case 4: self.openUrl("https://facebook.com/OrganicMaps", inSafari: true)
        case 5: self.openUrl("https://twitter.com/OrganicMapsApp", inSafari: true)
        case 6: self.openUrl(L("instagram_url"), inSafari: true)
        case 7: self.openUrl("https://matrix.to/#/%23organicmaps:matrix.org", inSafari: true)
        case 8: self.openUrl("https://wiki.openstreetmap.org/wiki/About_OpenStreetMap", inSafari: true)
        default: fatalError("Invalid cell1 \(indexPath)")
      }
      // Third section.
      case 3: switch indexPath[1] {
        case 0: self.openUrl("https://organicmaps.app/privacy")
        case 1: self.openUrl("https://organicmaps.app/terms")
        case 2: showCopyright()
        default: fatalError("Invalid cell2 \(indexPath)")
      }
      default: fatalError("Invalid section \(indexPath[0])")
    }
  }

  private func showCopyright() {
    let path = Bundle.main.path(forResource: "copyright", ofType: "html")!
    let html = try! String(contentsOfFile: path, encoding: String.Encoding.utf8)
    let webViewController = WebViewController.init(html: html, baseUrl: nil, title: L("copyright"))!
    webViewController.openInSafari = true
    self.navigationController?.pushViewController(webViewController, animated: true)
  }

  private func emailSubject(subject: String) -> String {
    let appInfo = AppInfo.shared()
    return String(format:"[%@-%@ iOS] %@", appInfo.bundleVersion, appInfo.buildNumber, subject)
  }

  private func emailBody() -> String {
    let appInfo = AppInfo.shared()
    return String(format: "\n\n\n\n- %@ (%@)\n- Organic Maps %@-%@\n- %@-%@\n- %@\n",
                  appInfo.deviceModel, UIDevice.current.systemVersion,
                  appInfo.bundleVersion, appInfo.buildNumber,
                  Locale.current.languageCode ?? "",
                  Locale.current.regionCode ?? "",
                  Locale.preferredLanguages.joined(separator: ", "))
  }

  private func openOutlook(subject: String, body: String, recipients: [String]) -> Bool {
    var components = URLComponents(string: "ms-outlook://compose")!
    components.queryItems = [
      URLQueryItem(name: "to", value: recipients.joined(separator: ";")),
      URLQueryItem(name: "subject", value: subject),
      URLQueryItem(name: "body", value: body),
    ]

    if let url = components.url, UIApplication.shared.canOpenURL(url) {
      UIApplication.shared.open(url)
      return true
    }
    return false
  }

  private func openGmail(subject: String, body: String, recipients: [String]) -> Bool {
    var components = URLComponents(string: "googlegmail://co")!
    components.queryItems = [
      URLQueryItem(name: "to", value: recipients.joined(separator: ";")),
      URLQueryItem(name: "subject", value: subject),
      URLQueryItem(name: "body", value: body),
    ]

    if let url = components.url, UIApplication.shared.canOpenURL(url) {
      UIApplication.shared.open(url)
      return true
    }
    return false
  }

  private func sendEmailWith(header: String, toRecipients: [String]) {
    let subject = emailSubject(subject: header)
    let body = emailBody()

    // Before iOS 14, try to open alternate email apps first, assuming that if users installed them, they're using them.
    let os = ProcessInfo().operatingSystemVersion
    if (os.majorVersion < 14 && (openGmail(subject: subject, body: body, recipients: toRecipients) ||
                                 openOutlook(subject: subject, body: body, recipients: toRecipients))) {
      return
    }
    // From iOS 14, it is possible to change the default mail app, and mailto should open a default mail app.
    if MWMMailViewController.canSendMail() {
      let vc = MWMMailViewController()
      vc.mailComposeDelegate = self
      vc.setSubject(subject)
      vc.setMessageBody(body, isHTML:false)
      vc.setToRecipients(toRecipients)
      vc.navigationBar.tintColor = UIColor.whitePrimaryText()
      self.present(vc, animated: true, completion:nil)
    } else {
      let text = String(format:L("email_error_body"), toRecipients.joined(separator: ";"))
      let alert = UIAlertController(title: L("email_error_title"), message: text, preferredStyle: .alert)
      let action = UIAlertAction(title: L("ok"), style: .default, handler: nil)
      alert.addAction(action)
      present(alert, animated: true, completion: nil)
    }
  }
}

// To properly close Email popup.
extension AboutController: MFMailComposeViewControllerDelegate {
  func mailComposeController(_ controller: MFMailComposeViewController, didFinishWith result: MFMailComposeResult, error: Error?) {
    self.dismiss(animated: true, completion: nil)
  }
}
