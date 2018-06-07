@objcMembers
@objc(MWMTabViewController)
class TabViewController: MWMViewController {
  var viewControllers: [UIViewController] = []
  var selectedIndex = 0
  var tabView: TabView {
    get {
      return view as! TabView
    }
  }

  override func loadView() {
    let v = TabView()
    v.dataSource = self
    view = v
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    viewControllers.forEach { (vc) in
      self.addChildViewController(vc)
      vc.didMove(toParentViewController: self)
    }
  }
}

extension TabViewController: TabViewDataSource {
  func numberOfPages(in tabView: TabView) -> Int {
    return viewControllers.count
  }

  func tabView(_ tabView: TabView, viewAt index: Int) -> UIView {
    return viewControllers[index].view
  }

  func tabView(_ tabView: TabView, titleAt index: Int) -> String? {
    return viewControllers[index].title
  }
}
