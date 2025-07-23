#import "MWMEditorAdditionalNamesTableViewController.h"
#import "MWMTableViewCell.h"

#import <CoreApi/StringUtils.h>

@interface MWMEditorAdditionalNamesTableViewController ()

@property(weak, nonatomic) id<MWMEditorAdditionalNamesProtocol> delegate;

@end

@implementation MWMEditorAdditionalNamesTableViewController
{
  StringUtf8Multilang m_name;
  std::vector<StringUtf8Multilang::Lang> m_languages;
  std::vector<NSInteger> m_additionalSkipLanguageCodes;
}

#pragma mark - UITableViewDataSource

- (void)configWithDelegate:(id<MWMEditorAdditionalNamesProtocol>)delegate
                           name:(StringUtf8Multilang const &)name
    additionalSkipLanguageCodes:(std::vector<NSInteger>)additionalSkipLanguageCodes
{
  self.delegate = delegate;
  m_name = name;
  m_additionalSkipLanguageCodes = additionalSkipLanguageCodes;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"choose_language");
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  auto const getIndex = [](std::string_view lang) { return StringUtf8Multilang::GetLangIndex(lang); };
  StringUtf8Multilang::Languages const & supportedLanguages = StringUtf8Multilang::GetSupportedLanguages();
  m_languages.clear();

  auto constexpr kDefaultCode = StringUtf8Multilang::kDefaultCode;
  for (auto const & language : supportedLanguages)
  {
    auto const langIndex = getIndex(language.m_code);
    if (langIndex != kDefaultCode && m_name.HasString(langIndex))
      continue;
    auto it = std::find(m_additionalSkipLanguageCodes.begin(), m_additionalSkipLanguageCodes.end(), langIndex);
    if (it == m_additionalSkipLanguageCodes.end())
      m_languages.push_back(language);
  }

  std::sort(m_languages.begin(), m_languages.end(),
            [&getIndex](StringUtf8Multilang::Lang const & lhs, StringUtf8Multilang::Lang const & rhs)
  {
    // Default name can be changed in advanced mode, but it should be last in list of names.
    if (getIndex(lhs.m_code) == kDefaultCode && getIndex(rhs.m_code) != kDefaultCode)
      return false;
    if (getIndex(lhs.m_code) != kDefaultCode && getIndex(rhs.m_code) == kDefaultCode)
      return true;

    return std::string(lhs.m_code) < std::string(rhs.m_code);
  });
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMTableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"ListCellIdentifier"];
  NSInteger const index = indexPath.row;
  StringUtf8Multilang::Lang const & lang = m_languages[index];
  cell.textLabel.text = ToNSString(lang.m_name);
  cell.detailTextLabel.text = ToNSString(lang.m_code);
  cell.accessoryType = UITableViewCellAccessoryNone;
  return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_languages.size();
}

#pragma mark - UITableViewDataSource

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSInteger const index = indexPath.row;
  StringUtf8Multilang::Lang const & language = m_languages[index];

  [self.delegate addAdditionalName:StringUtf8Multilang::GetLangIndex(language.m_code)];
  [self.navigationController popViewControllerAnimated:YES];
}

@end
