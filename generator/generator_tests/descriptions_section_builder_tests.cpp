#include "testing/testing.hpp"

#include "generator/descriptions_section_builder.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/osm2meta.hpp"

#include "descriptions/serdes.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_mwm.hpp"

#include "coding/file_name_utils.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <numeric>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace generator;

namespace
{
class Feature : public tests_support::TestFeature
{
public:
  Feature() = default;

  void SetMetadata(feature::Metadata const & metadata) { m_metadata = metadata; }
  void SetTypes(std::vector<uint32_t> const & types) { m_types = types; }

  template <typename ToDo>
  void ForEachType(ToDo && f)
  {
    for (auto const & type : m_types)
      f(type);
  }

  // TestFeature overrides:
  std::string ToString() const override { return m_name; }

private:
  std::vector<uint32_t> m_types;
};
}  // namespace

namespace generator_tests
{
class TestDescriptionSectionBuilder
{
public:
  struct WikiData
  {
    std::string m_url;
    // A collection of pairs of languages ​​and content.
    std::vector<std::pair<std::string, std::string>> m_pages;
  };

  static std::string const kMwmFile;
  static std::string const kDirPages;
  static std::vector<WikiData> const kWikiData;

  TestDescriptionSectionBuilder()
    : m_writableDir(GetPlatform().WritableDir())
    , m_wikiDir(base::JoinPath(m_writableDir, kDirPages))
  {
    for (auto const & m : kWikiData)
    {
      auto const dir = DescriptionsCollectionBuilder::MakePath(m_wikiDir, m.m_url);
      CHECK(Platform::MkDirRecursively(dir), ());
      for (auto const & d : m.m_pages)
      {
        auto const file = base::JoinPath(dir, d.first + ".html");
        std::ofstream stream(file);
        stream << d.second;
      }
    }

    classificator::Load();
  }

  ~TestDescriptionSectionBuilder()
  {
    CHECK(Platform::RmDirRecursively(m_wikiDir), ());
  }

  void MakeDescriptions() const
  {
    DescriptionsCollectionBuilder b(m_wikiDir, kMwmFile);
    auto const descriptionList = b.MakeDescriptions<Feature, ForEachFromDatMockAdapt>();
    auto const  & stat = b.GetStat();
    TEST_EQUAL(GetTestDataPages(), descriptionList.size(), ());
    TEST_EQUAL(GetTestDataPages(), stat.GetPages(), ());
    TEST_EQUAL(GetTestDataSize(), stat.GetSize(), ());
    TEST(CheckLangs(stat.GetLangStatistics()), ());
  }

  void MakePath() const
  {
    std::string trueAnswer = "/wikiDir/en.wikipedia.org/wiki/Helsinki_Olympic_Stadium";
    {
      std::string const wikiDir = "/wikiDir/";
      std::string const wikiUrl = "http://en.wikipedia.org/wiki/Helsinki_Olympic_Stadium/";
      auto const answer = DescriptionsCollectionBuilder::MakePath(wikiDir, wikiUrl);
      TEST_EQUAL(trueAnswer, answer, ());
    }
    {
      std::string const wikiDir = "/wikiDir";
      std::string const wikiUrl = "https://en.wikipedia.org/wiki/Helsinki_Olympic_Stadium";
      auto const answer = DescriptionsCollectionBuilder::MakePath(wikiDir, wikiUrl);
      TEST_EQUAL(trueAnswer, answer, ());
    }
  }

  void FindPageAndFill() const
  {
    {
      DescriptionsCollectionBuilder b(m_wikiDir, kMwmFile);
      CHECK(!kWikiData.empty(), ());
      auto const & first = kWikiData.front();
      StringUtf8Multilang str;
      auto const size = b.FindPageAndFill(first.m_url, str);
      TEST(size, ());
      TEST_EQUAL(*size, GetPageSize(first.m_pages), ());
      TEST(CheckLangs(str, first.m_pages), ());
    }
    {
      DescriptionsCollectionBuilder b(m_wikiDir, kMwmFile);
      StringUtf8Multilang str;
      std::string const badUrl = "https://en.wikipedia.org/wiki/Not_exists";
      auto const size = b.FindPageAndFill(badUrl, str);
      TEST(!size, ());
    }
  }

  // The test hopes that the kWikiData.front() has a 'en' lang.
  void FillStringFromFile() const
  {
    CHECK(!kWikiData.empty(), ());
    auto const & first = kWikiData.front();
    std::string const lang = "en";
    auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
    auto const path = DescriptionsCollectionBuilder::MakePath(m_wikiDir, first.m_url);
    auto const fullPath = base::JoinPath(path, (lang + ".html"));
    StringUtf8Multilang str;
    // This is a private function and should take the right path fullPath.
    auto const size = DescriptionsCollectionBuilder::FillStringFromFile(fullPath, langIndex, str);
    auto const it = std::find_if(std::begin(first.m_pages), std::end(first.m_pages),
                                 [&](auto const & p) { return p.first == lang; });
    CHECK(it != std::end(first.m_pages), ());
    TEST_EQUAL(size, it->second.size(), ());
    TEST(CheckLangs(str, first.m_pages), ());
  }

  void GetFeatureDescription() const
  {
    DescriptionsCollectionBuilder b(m_wikiDir, kMwmFile);
    CHECK(!kWikiData.empty(), ());
    auto const & first = kWikiData.front();
    auto const featureId = 0;
    auto ft = MakeFeature(first.m_url);
    descriptions::FeatureDescription description;
    auto const wikiUrl = ft.GetMetadata().GetWikiURL();
    auto const size = b.GetFeatureDescription(wikiUrl, featureId, description);

    TEST_EQUAL(size, GetPageSize(first.m_pages), ());
    CHECK_NOT_EQUAL(size, 0, ());
    TEST_EQUAL(description.m_featureIndex, featureId, ());
    TEST(CheckLangs(description.m_description, first.m_pages), ());
  }

  void BuildDescriptionsSection() const
  {
    using namespace platform;
    using namespace platform::tests_support;
    auto const testMwm = kMwmFile + DATA_FILE_EXTENSION;
    ScopedMwm testScopedMwm(testMwm);
    DescriptionsSectionBuilder<Feature, ForEachFromDatMockAdapt>::Build(m_wikiDir,
                                                                        testScopedMwm.GetFullPath());
    FilesContainerR container(testScopedMwm.GetFullPath());
    TEST(container.IsExist(DESCRIPTIONS_FILE_TAG), ());

    descriptions::Deserializer d;
    auto reader = container.GetReader(DESCRIPTIONS_FILE_TAG);
    for (descriptions::FeatureIndex i = 0; i < kWikiData.size(); ++i)
    {
      auto const & pages = kWikiData[i].m_pages;
      for (auto const & p : pages)
      {
        auto const featureId = i;
        if (!IsSupportedLang(p.first))
          continue;

        auto const langIndex = StringUtf8Multilang::GetLangIndex(p.first);
        std::string str;
        d.Deserialize(*reader.GetPtr(), featureId, {langIndex,}, str);
        TEST_EQUAL(str, p.second, ());
      }
    }
  }

private:
  template <class ToDo>
  static void ForEachFromDatMock(string const &, ToDo && toDo)
  {
    for (size_t i = 0; i < kWikiData.size(); ++i)
    {
      auto ft = MakeFeature(kWikiData[i].m_url);
      toDo(ft, static_cast<uint32_t>(i));
    }
  }

  template <class T>
  struct ForEachFromDatMockAdapt
  {
    void operator()(std::string const & str, T && fn) const
    {
      ForEachFromDatMock(str, std::forward<T>(fn));
    }
  };

  static std::map<std::string, size_t> GetTestDataMapLang()
  {
    std::map<std::string, size_t> langs;
    for (auto const & m : kWikiData)
    {
      for (auto const & d : m.m_pages)
      {
        if (IsSupportedLang(d.first))
          langs[d.first] += 1;
      }
    }

    return langs;
  }

  static size_t GetTestDataPages()
  {
    size_t size = 0;
    for (auto const & m : kWikiData)
    {
      auto const & pages = m.m_pages;
      auto const exists = std::any_of(std::begin(pages), std::end(pages), [](auto const & d) {
        return IsSupportedLang(d.first);
      });

      if (exists)
        ++size;
    }

    return size;
  }

  static size_t GetTestDataSize()
  {
    size_t size = 0;
    for (auto const & m : kWikiData)
    {
      for (auto const & d : m.m_pages)
      {
        if (IsSupportedLang(d.first))
          size += d.second.size();
      }
    }

    return size;
  }

  static bool IsSupportedLang(std::string const & lang)
  {
    return StringUtf8Multilang::GetLangIndex(lang) != StringUtf8Multilang::kUnsupportedLanguageCode;
  }

  static size_t GetPageSize(std::vector<std::pair<std::string, std::string>> const & p)
  {
    return std::accumulate(std::begin(p), std::end(p), size_t{0}, [] (size_t acc, auto const & n) {
      return acc + n.second.size();
    });
  }

  static Feature MakeFeature(std::string const & url)
  {
    FeatureParams params;
    MetadataTagProcessor p(params);
    feature::Metadata & md = params.GetMetadata();
    p("wikipedia", url);
    Feature ft;
    ft.SetMetadata(md);

    auto const & wikiChecker = ftypes::WikiChecker::Instance();
    CHECK(!wikiChecker.kTypesForWiki.empty(), ());
    auto const itFirst = std::begin(wikiChecker.kTypesForWiki);
    auto const type = classif().GetTypeByPath({itFirst->first, itFirst->second});
    ft.SetTypes({type});
    return ft;
  }

  static bool CheckLangs(DescriptionsCollectionBuilderStat::LangStatistics const & stat)
  {
    auto const langs =  GetTestDataMapLang();
    for (size_t code = 0; code < stat.size(); ++code)
    {
      if (stat[code] == 0)
        continue;

      auto const strLang = StringUtf8Multilang::GetLangByCode(static_cast<int8_t>(code));
      if (langs.count(strLang) == 0)
        return false;

      if (langs.at(strLang) != stat[code])
        return false;
    }

    return true;
  }

  static bool CheckLangs(StringUtf8Multilang const & str, std::vector<std::pair<std::string, std::string>> const & p)
  {
    bool result = true;
    str.ForEach([&](auto code, auto const &) {
      auto const it = std::find_if(std::begin(p), std::end(p), [&](auto const & p) {
        return StringUtf8Multilang::GetLangIndex(p.first) == code;
      });

      if (it == std::end(p))
        result = false;
    });

    return result;
  }

  std::string const m_writableDir;
  std::string const m_wikiDir;
};

std::string const TestDescriptionSectionBuilder::kMwmFile = "MwmFile";

std::string const TestDescriptionSectionBuilder::kDirPages = "wiki";
}  // namespace generator_tests

using namespace generator_tests;

UNIT_CLASS_TEST(TestDescriptionSectionBuilder, DescriptionsCollectionBuilder_MakeDescriptions)
{
  TestDescriptionSectionBuilder::MakeDescriptions();
}

UNIT_CLASS_TEST(TestDescriptionSectionBuilder, DescriptionsCollectionBuilder_MakePath)
{
  TestDescriptionSectionBuilder::MakePath();
}

UNIT_CLASS_TEST(TestDescriptionSectionBuilder, DescriptionsCollectionBuilder_FindPageAndFill)
{
  TestDescriptionSectionBuilder::FindPageAndFill();
}

UNIT_CLASS_TEST(TestDescriptionSectionBuilder, DescriptionsCollectionBuilder_FillStringFromFile)
{
  TestDescriptionSectionBuilder::FillStringFromFile();
}

UNIT_CLASS_TEST(TestDescriptionSectionBuilder, DescriptionsCollectionBuilder_GetFeatureDescription)
{
  TestDescriptionSectionBuilder::GetFeatureDescription();
}

UNIT_CLASS_TEST(TestDescriptionSectionBuilder, DescriptionsCollectionBuilder_BuildDescriptionsSection)
{
  TestDescriptionSectionBuilder::BuildDescriptionsSection();
}

namespace generator_tests
{

// http://en.wikipedia.org/wiki/Helsinki_Olympic_Stadium/ -  en, de, ru, fr
// https://en.wikipedia.org/wiki/Turku_Cathedral - en, ru
std::vector<TestDescriptionSectionBuilder::WikiData> const TestDescriptionSectionBuilder::kWikiData = {
  {"http://en.wikipedia.org/wiki/Helsinki_Olympic_Stadium/",
   {
     {
       "en",
       R"(
       <p class="mw-empty-elt">
       </p>
       <p>The <b>Helsinki Olympic Stadium</b> (Finnish: <i lang="fi">Helsingin Olympiastadion</i>; Swedish: <i lang="sv">Helsingfors Olympiastadion</i>), located in the Töölö district about 2.3 kilometres (1.4 mi) from the centre of the Finnish capital Helsinki, is the largest stadium in the country, nowadays mainly used for hosting sports events and big concerts. The stadium is best known for being the centre of activities in the 1952 Summer Olympics. During those games, it hosted athletics, equestrian show jumping, and the football finals.
       </p><p>The stadium was also the venue for the first Bandy World Championship in 1957, the first World Athletics Championships in 1983 as well as for the 2005 World Championships in Athletics. It hosted the European Athletics Championships in 1971, 1994 and 2012.
       </p><p>It is also the home stadium of the Finland national football team. The stadium is closed temporarily since March 2016 for renovation works and scheduled of reopening in 2019.
       </p>

       <h2>History</h2>
       <p>The Olympic Stadium was designed in functionalistic style by the architects Yrjö Lindegren and Toivo Jäntti. Construction of the Olympic Stadium began in 1934 and it was completed in 1938, with the intent to host the 1940 Summer Olympics, which were moved from Tokyo to Helsinki before being cancelled due to World War II. It hosted the 1952 Summer Olympics over a decade later instead. The stadium was also to be the main venue for the cancelled 1943 Workers' Summer Olympiad.
       </p><p>It was the venue for the first ever Bandy World Championship in 1957.
       </p><p>The stadium was completely modernized in 1990–1994 and also renovated just before the 2005 World Championships in Athletics.
       </p><p>In 2006 an American TV series, <i>The Amazing Race 10</i>, had one of its episodes ending at The Olympic Stadium Tower. As a task, teams had to do a face-first rappel (known as the Angel Dive) down the Helsinki Olympic Tower.
       </p><p>Since March 2007, a Eurasian eagle-owl has been spotted living in and around the stadium. On June 6, 2007, during a Euro 2008 qualifying match, the owl delayed play by ten minutes after perching on a goalpost. The owl was later christened Bubi and was named as Helsinki's Resident of the Year.
       </p>

       <p>The 50th anniversary of the Helsinki Olympic Games hosted in the Helsinki Olympic Stadium was the main motif for one of the first Finnish euro silver commemorative coins, the 50th anniversary of the Helsinki Olympic Games commemorative coin, minted in 2002. On the reverse, a view of the Helsinki Olympic Stadium can be seen.  On the right, the 500 markka commemorative coin minted in 1952 celebrating the occasion is depicted.
       </p>

       <h2>Features</h2>
       <p>The stadium's spectator capacity was at its maximum during the 1952 Summer Olympics with over 70,000 spectator places. Nowadays the stadium has 40,600 spectator places. During concerts, depending on the size of the stage, the capacity is 45,000–50,000.
       </p><p>The tower of the stadium, a distinct landmark with a height of 72.71 metres (238.5 ft), a measurement of the length of the gold-medal win by Matti Järvinen in javelin throw of 1932 Summer Olympics, is open for visitors and offers impressive views over Helsinki. It is possible to see into the adjacent Telia 5G -areena.
       </p><p>A Youth Hostel is located within the Stadium complex.
       </p>

       <h2>Future</h2>
       <p>Major renovation work at the stadium started in the spring of 2016. The stadium will be closed during the construction and will reopen in 2019. During renovation all the spectator stands will be covered with canopies and the field area and the tracks will be renewed. It will also offer extended restaurant areas and more indoor sport venues.</p><p>Projected costs for the renovation is 209 million euros and it will be funded by Finnish state and the city of Helsinki.</p>

       <h2>Events</h2>
       <h3>Sport events</h3>
       <ul><li>1952 Summer Olympics</li>
       <li>1957 Bandy World Championship</li>
       <li>1971 European Athletics Championships</li>
       <li>1983 World Championships in Athletics</li>
       <li>1994 European Athletics Championships</li>
       <li>2005 World Championships in Athletics</li>
       <li>UEFA Women's Euro 2009 (4 Group matches and a Final)</li>
       <li>2012 European Athletics Championships</li></ul>

       <h3>Concerts</h3>
       <h2>References</h2>
       <h2>External links</h2>
       <p> Media related to Helsingin olympiastadion at Wikimedia Commons
       </p>
       <ul><li>1952 Summer Olympics official report. pp. 44–7.</li>
       <li>Stadion.fi – Official site</li>
       <li>History of the stadium</li>
       <li>Panoramic virtual tour from the stadium tower</li></ul>
       )"
     },
     {
       "de",
       R"(
       <p>Das <b>Olympiastadion Helsinki</b> (<span>finnisch</span> <span lang="fi-Latn"><i>Helsingin olympiastadion</i></span>, <span>schwedisch</span> <span lang="sv-Latn"><i>Helsingfors Olympiastadion</i></span>) ist das größte Stadion Finnlands. Das Stadionrund mit Leichtathletikanlage ist die Heimstätte der finnischen Fußballnationalmannschaft. Es liegt rund zwei Kilometer vom Stadtzentrum von Helsinki entfernt im Stadtteil Töölö. 2006 wurde es unter Denkmalschutz gestellt. Das Nationalstadion wird ab 2016 umfangreich renoviert und soll nach drei Jahren 2019 wiedereröffnet werden.</p>

       <h2>Architektur</h2>
       <p>Der funktionalistische Bau wurde von den Architekten Yrjö Lindegren und Toivo Jäntti entworfen. Das Stadionwahrzeichen ist der 72,71 Meter hohe Turm, dessen Höhe genau der Siegweite des finnischen Speerwurf-Olympiasiegers von 1932, Matti Järvinen, entspricht. Außerhalb von Veranstaltungen ist der Turm für Besucher als Aussichtsturm geöffnet.
       </p>

       <h2>Geschichte</h2>
       <p>Das Olympiastadion wurde von 1934 bis 1938 erbaut – mit dem Ziel der Austragung der Olympischen Sommerspiele 1940, die aber wegen des Zweiten Weltkrieges ausfielen. Von 1994 bis 1999 wurde das Stadion umgebaut, dabei wurde die Zahl der Zuschauerplätze von 70.000 auf 40.000 verringert. Es verfügte bis Ende 2015 über 39.784 Sitzplätze. Vor der Einführung des Euro als finnische Landeswährung war das Stadion auf der 10-Finnmark-Banknote zusammen mit dem Langstreckenläufer Paavo Nurmi abgebildet. Vor dem Stadion ist eine Statue des neunfachen finnischen Olympiasiegers Nurmi mit einer Gedenktafel aufgestellt. In direkter Nachbarschaft zum Olympiastadion liegt das 1915 eingeweihte Fußballstadion Töölön Pallokenttä. 2000 wurde das Gelände durch ein weiteres Fußballstadion, das Sonera Stadium mit rund 10.000 Plätzen, ergänzt.
       </p><p>Zu Beginn der 2010er Jahre begannen die Planungen für eine umfassende Stadionrenovierung. Ende 2014 gab die Stadt Helsinki genauere Angaben zu den Umbauplänen. Dabei soll der historische Entwurf aus den 1930er Jahren aber erhalten bleiben. So bleibt das äußere Bild nahezu unberührt und die sichtbaren Veränderungen betreffen den Stadioninnenraum. Die Ränge bekommen eine neue Bestuhlung. Dies wird die Anzahl der Plätze auf rund 35.000 bis 36.000 verringern. Zudem sollen die Tribünen des Olympiastadions überdacht werden. Die neue Überdachung wird, wie bei dem bestehenden Dach der Haupttribüne und der Gegentribüne, von Stützpfeilern auf den Rängen getragen. Das Stadion erhält des Weiteren ein neues Besucherzentrum und die Zahl der Eingänge wird erhöht. Insgesamt werden die Räumlichkeiten der Sportstätte umfangreich renoviert. Die Bauarbeiten sollen im Frühjahr 2016 beginnen und im Frühjahr 2019 soll die Anlage wiedereröffnet werden. Während der Renovierung wird das Olympiastadion geschlossen bleiben. Die Kosten für die Umbauten nach den Plänen des Arkkitehtitoimisto K2S werden auf 209 Millionen Euro veranschlagt.</p><p>Seit dem 23. Dezember 2015 ist das Olympiastadion im Hinblick auf die Renovierung für die Öffentlichkeit geschlossen. Zuvor konnten sich Interessierte Souvenirs von den hölzernen Sitzbänke sichern. Rund 10.000 Andenkensammler kamen innerhalb einer Woche und konnten gegen Zahlung von 18 Euro ihre persönliche Erinnerung an das alte Olympiastadion abmontieren. Es war der symbolische Auftakt für die Bauarbeiten.</p><p>Im Januar 2017 wurde bekannt, dass der geplante Umbau des traditionsreichen Stadions teuerer wird als erwartet. Die Summe von 209 Mio. wurde auf 261 Mio. Euro nach oben korrigiert. Noch im Januar des Jahres soll ein Generalunternehmer für den Umbau ausgewählt werden.</p>

       <h2>Veranstaltungen</h2>
       <p>Die Anlage war das Hauptstadion der Olympischen Sommerspiele 1952. Nachdem das Sportstätte bereits die Leichtathletik-Weltmeisterschaften 1983 beherbergt hatte, wurden hier auch die Leichtathletik-Weltmeisterschaften 2005 ausgetragen. Im Vorfeld wurde das Dach erweitert, das jetzt die Geraden und einen kleinen Teil der Kurve überspannt. Das Stadion war einer der fünf Spielorte der Fußball-Europameisterschaft der Frauen 2009. Im Jahr 2012 war das Stadion zum dritten Mal nach 1971 und 1994 Schauplatz von Leichtathletik-Europameisterschaften. Die erste Bandy-Weltmeisterschaft 1957 wurde ebenfalls in der Sportstätte durchgeführt.
       </p><p>Das Olympiastadion wird auch als Konzertarena genutzt; so waren z. B. Michael Jackson, U2, Tina Turner, The Rolling Stones, AC/DC, Dire Straits, Paul McCartney, Genesis, Metallica, Bon Jovi, Iron Maiden, Bruce Springsteen u. a. zu Gast im Stadion der Hauptstadt.</p>

       <h2>Weblinks</h2>
       <ul><li>stadion.fi: Website des Olympiastadions (englisch)</li>
       <li>stadiumdb.com: Olympiastadion Helsinki (englisch)</li>
       <li>stadionwelt.de: Bildergalerie</li></ul>

       <h2>Einzelnachweise</h2>
       )"
     },
     {
       "ru",
       R"(
       <p><b>Олимпийский стадион Хельсинки</b> (фин. <span lang="fi">Helsingin olympiastadion</span>, швед. <span lang="sv">Helsingfors Olympiastadion</span>) — крупнейшая спортивная арена Финляндии. Стадион расположен в районе Тёёлё, примерно в двух километрах от исторического центра Хельсинки. В летнее время, помимо спортивных мероприятий, на стадионе проводят и музыкальные концерты.
       </p>

       <h2>История</h2>
       <p>Началом истории Олимпийского стадиона принято считать 11 декабря 1927 года, когда под эгидой городских властей Хельсинки был основан фонд, призванный собрать средства для строительства спортивной арены, способной принять Летние Олимпийские игры.
       </p><p>Строительство стадиона продолжалось с 12 февраля 1934 года по 12 июня 1938 года.</p><p>Стадион готовился принять XII Олимпийские Игры 1940 года, от проведения которых отказался Токио из-за участия Японии во Второй японско-китайской войне
       Однако, Игры были окончательно отменены МОК по причине начала Второй мировой войны.
       </p><p>Спустя 12 лет Олимпийский стадион Хельсинки стал главной ареной XV Летних Олимпийских Игр 1952 года.
       В 1990—1994 гг. на стадионе прошла генеральная реконструкция.
       </p><p>Олимпийский стадион Хельсинки, построенный по проекту двух архитекторов: Юрьё Линдегрена (fi:Yrjö Lindegren) и Тойво Янтти (fi:Toivo Jäntti), ставший ярким образцом архитектурного стиля функционализма, отличается простотой линий и органичным сочетанием с окружающим его классическим финским пейзажем: гранитными скалами и сосновым лесом.
       </p>

       <h2>Размеры</h2>
       <p>Башня Олимпийского стадиона имеет высоту 72 метра 71 сантиметр в честь рекорда Матти Ярвинена (fi:Matti Järvinen) в метании копья на Олимпийских Играх 1932 года.
       Чаша стадиона достигает в длину 243 метра и в ширину 159 метров.
       Наибольшая вместимость в 70 435 человек была во время проведения Олимпийских Игр 1952 года, и до 2002-го, за счёт мест скамеечного типа. А сейчас сокращена до 40 000 зрителей, после реконструкции и установлении 41 тысячи индивидуальных сидений.
       Внутренний же вид арены напоминает древние стадионы античности.
       </p>

       <h2>События</h2>
       <h3>1952 год. XV Олимпийские Игры</h3>
       <p>Проходили с 19 июля по 3 августа 1952 года.
       В Играх приняло участие 4 955 атлетов (519 женщин и 4 436 мужчин) из 69 стран мира. Разыграно 149 комплектов медалей по 17 видам спорта.
       Впервые приняли участие национальные сборные СССР и ФРГ.
       Олимпийское пламя зажгли Пааво Нурми и Ханнес Колехмайнен. Открыл Игры президент Финляндской республики Юхо Кусти Паасикиви. Закрывал их президент МОК Зигфрид Эдстрем, однако он забыл после торжественной речи сказать слова «Объявляю Игры XV Олимпиады закрытыми», поэтому игры формально считаются до сих пор незакрытыми.
       </p>

       <h3>1957 год. Чемпионат мира по хоккею с мячом</h3>
       <p>Проходил с 28 февраля по 3 марта 1957 года.
       </p>

       <h3>1971 год. Чемпионат Европы по лёгкой атлетике</h3>
       <p>Проходил с 10 по 15 августа 1971 года.
       Участвовало 857 атлетов.
       </p>

       <h3>1983 год. Чемпионат мира по лёгкой атлетике</h3>
       <p>Проходил с 7 по 14 августа 1983 года.
       Участвовал 1 355 атлетов.
       </p>

       <h3>1994 год. Чемпионат Европы по лёгкой атлетике</h3>
       <p>Проходил с 9 по 14 августа 1994 года.
       </p>

       <h3>2005 год. Чемпионат мира по лёгкой атлетике</h3>
       <p>Проходил с 6 по 14 августа 2005 года.
       Приняло участие более 1 900 атлетов.
       </p>

       <h3>2012 год. Чемпионат Европы по лёгкой атлетике</h3>
       <p>Прошёл с 27 июня по 1 июля 2012 года.
       </p><p>Зимой-летом 2005 года Олимпийский стадион подвергся модернизации: был уставлен козырёк над частью трибун и новая система освещения чаши стадиона.
       </p>

       <h2>Футбольная сборная Финляндии</h2>
       <p>Олимпийский стадион Хельсинки является домашней ареной сборной Финляндии по футболу
       </p>

       <h2>Дополнительные сведения</h2>
       <p>В комплексе Олимпийского стадиона находится Музей финского спорта (фин. <span lang="fi">Suomen Urheilumuseo</span>).
       В северной части стадиона расположен один из самых дешевых хостелов в центре финской столицы (Stadion Hostel).
       Так же организованы экскурсии на 72-метровую башню, с которой открывается прекрасная панорама города и Финского залива. (Часы работы: пн-пт 9-20, сб-вс 9-18)
       </p>

       <h2>Примечания</h2>
       <h2>Ссылки</h2>
       <ul><li>Olympiastadion</li>
       <li>Suomen Urheilumuseo</li></ul>
       )"
     },
     {
       "fr",
       R"(
       <p>Le <b>stade olympique d'Helsinki</b> (en finnois et en suédois : <i>Olympiastadion</i>) situé dans le quartier de Töölö, à environ 2 <abbr class="abbr" title="kilomètre">km</abbr> du centre-ville d'Helsinki est le plus grand stade en Finlande. C'est le stade principal des Jeux olympiques d'été de 1952. Il a été construit pour y célébrer les Jeux olympiques d'été de 1940 qui ont été finalement annulés (auparavant attribués à Tokyo) en raison de la Seconde Guerre mondiale.
       </p><p>Le stade a également été le siège des Championnats du monde d'athlétisme, les tout premiers en 1983 et à nouveau en 2005. Il hébergera pour la <abbr class="abbr" title="Troisième">3<sup>e</sup></abbr> fois les Championnats d'Europe d'athlétisme en 2012, après les éditions de 1971 et de 1994. C'est aussi le stade de l'Équipe de Finlande de football.
       </p>

       <h2>Histoire</h2>
       <p>Le stage est conçu par Yrjö Lindegren et Toivo Jäntti. Ce dernier s'occupera de toutes les modifications ultérieures.
       La construction du stade olympique a commencé en 1934 et elle a été achevée en 1938. Le stade a été entièrement modernisé en 1990-1994 et a également été rénové en 2005 pour les Championnats du monde d'athlétisme. Sa capacité de spectateurs était à son maximum au cours de Jeux olympiques de 1952 avec plus de 70 000 spectateurs. De nos jours, le stade a en moyenne 40 000 spectateurs.
       </p><p>Il est caractérisé par une tour haute de 72 <abbr class="abbr" title="mètre">m</abbr> (dont la hauteur serait aussi celle d'un essai de lancer du javelot, un sport national).
       </p>

       <h2>Événements</h2>
       <ul><li>Jeux olympiques d'été de 1952</li>
       <li>Championnats d'Europe d'athlétisme 1971</li>
       <li>Coupe d'Europe des nations d'athlétisme 1977</li>
       <li>Championnats du monde d'athlétisme 1983</li>
       <li>Championnats d'Europe d'athlétisme 1994</li>
       <li>Championnats du monde d'athlétisme 2005</li>
       <li>Championnats d'Europe d'athlétisme 2012</li></ul>

       <h2>Concerts</h2>
       <ul><li>The Rolling Stones, 2 septembre 1970</li>
       <li>Dire Straits, 4 août 1992</li>
       <li>The Rolling Stones, 6 juin 1995</li>
       <li>Bon Jovi, 19 juillet 1996</li>
       <li>Tina Turner, 7 août 1996</li>
       <li>U2, 9 août 1997</li>
       <li>Michael Jackson, 18 et 20 août 1997</li>
       <li>Elton John, 25 juin 1998</li>
       <li>Rolling Stones, 5 août 1998</li>
       <li>Mestarit, 5 août 1999</li>
       <li>AC/DC, 26 juin 2001</li>
       <li>Bruce Springsteen, 16 et 17 juin 2003</li>
       <li>The Rolling Stones, 16 juillet 2003</li>
       <li>Metallica, 28 mai 2004</li>
       <li>Paul McCartney, 17 juin 2004</li>
       <li>Genesis, 11 juin 2007</li>
       <li>Metallica, 15 juillet 2007</li>
       <li>The Rolling Stones, <abbr class="abbr" title="Premier">1<sup>er</sup></abbr> août 2007</li>
       <li>Bruce Springsteen, 11 juillet 2008</li>
       <li>Iron Maiden, 18 juillet 2008</li>
       <li>U2, 20 et 21 août 2010</li></ul>

       <h2>Futur</h2>
       <p>Le stade sera rénové en deux parties, l'une en 2016, et pour terminer en 2019. La rénovation coûtera 209 millions d'euros.
       </p><p>La structure  extérieure, où il y aura des panneaux de bois orneront la partie haute de la façade. Pour ce qui est de la partie intérieure du toit, elle sera en bois, un toit qui couvrira toutes les tribunes d'ailleurs.
       </p>

       <h2>Notes</h2>
       <ul><li>Le stade olympique a été, avec les coureurs Paavo Nurmi et Erik Bruun l'illustration de l'ancien billet 10 mark finnois.</li>
       <li>Une série télévisée américaine, The Amazing Race, avait fait un de leurs épisodes qui se terminait à La tour du stade olympique en 2006.</li>
       <li>Une auberge de jeunesse est située dans le complexe du stade.</li></ul>

       <h2>Voir aussi</h2>
       <h3>Liens externes</h3>
       <ul><li><abbr class="abbr indicateur-langue" title="Langue : finnois">(fi)</abbr> Site officiel</li>
       <li><abbr class="abbr indicateur-langue" title="Langue : finnois">(fi)</abbr> Histoire du Stade Olympique</li></ul>

       <h3>Liens internes</h3>
       <ul><li>Piscine olympique d'Helsinki</li></ul><p><br></p>
       <ul id="bandeau-portail" class="bandeau-portail"><li><span><span></span> <span>Portail de l’architecture et de l’urbanisme</span> </span></li> <li><span><span></span> <span>Portail du football</span> </span></li> <li><span><span></span> <span>Portail de l’athlétisme</span> </span></li> <li><span><span></span> <span>Portail des Jeux olympiques</span> </span></li> <li><span><span></span> <span>Portail d’Helsinki</span> </span></li>                </ul>
       )"
     }
   }
  },
  {
    "https://en.wikipedia.org/wiki/Turku_Cathedral",
    {
      {
        "en",
        R"(
        <p><b>Turku Cathedral</b> (Finnish: <i lang="fi">Turun tuomiokirkko</i>, Swedish: <i lang="sv">Åbo domkyrka</i>) is the previous catholic cathedral of Finland, today the Mother Church of the Evangelical Lutheran Church of Finland. It is the central church of the Lutheran Archdiocese of Turku and the seat of the Lutheran Archbishop of Finland, Kari Mäkinen. It is also regarded as one of the major records of Finnish architectural history.
        </p><p>Considered to be the most important religious building in Finland, the cathedral has borne witness to many important events in the nation's history and has become one of the city's most recognizable symbols. The cathedral is situated in the heart of Turku next to the Old Great Square, by the river Aura. Its presence extends beyond the local precinct by having the sound of its bells chiming at noon broadcast on national radio. It is also central to Finland's annual Christmas celebrations.
        </p><p>The cathedral was originally built out of wood in the late 13th century, and was dedicated as the main cathedral of Finland in 1300, the seat of the Catholic bishop of Turku. It was considerably expanded in the 14th and 15th centuries, mainly using stone as the construction material. The cathedral was badly damaged during the Great Fire of Turku in 1827, and was rebuilt to a great extent afterwards.
        </p>

        <h2>History</h2>
        <p>As the town of Turku began to emerge in the course of the 13th century as the most important trading centre in Finland, the Bishop's see of the Diocese of Finland was transferred from its previous location at Koroinen, some distance further up on the bank of Aura river, to the middle of the town. By the end of the 13th century, a new stone church had been completed on the site of the former wooden-built parish church on Unikankare Mound, and it was consecrated in 1300 as the Cathedral Church of the Blessed Virgin Mary and Saint Henry, the first Bishop of Finland.
        </p><p>At its earliest the cathedral was smaller than the present building. Its east front was where the pulpit stands now, and its roof was considerably lower than at the moment. Extensions were made to the cathedral throughout the Middle Ages. During the 14th century a new choir was added, from which the octagonal Gothic pillars in the present chancel originate. Throughout the Middle Ages, the High Altar was located opposite the easternmost pillars of the nave, until it was transferred to its present location in the apse, in what had previously been the Chapel of All Saints, in the mid-17th century.
        </p><p>During the 15th century, side-chapels were added along the north and south sides of the nave, containing altars dedicated to various saints. By the end of the Middle Ages these numbered 42 in total. The roof-vaults were also raised during the latter part of the 15th century to their present height of 24 meters. Thus, by the beginning of the Modern era, the church had approximately taken on its present shape. The major later addition to the cathedral is the tower, which has been rebuilt several times, as a result of repeated fires. The worst damage was caused by the Great Fire of Turku in 1827, when most of the town was destroyed, along with the interior of both the tower and the nave and the old tower roof. The present spire of the tower, constructed after the great fire, reaches a height of 101 meters above sea level, and is visible over a considerable distance as the symbol of both the cathedral and the city of Turku itself.
        </p><p>In the reformation the cathedral was taken by the Lutheran Church of Finland (Sweden). Most of the present interior also dates from the restoration carried out in the 1830s, following the Great Fire. The altarpiece, depicting the Transfiguration of Jesus, was painted in 1836 by the Swedish artist Fredrik Westin. The reredos behind the High Altar, and the pulpit in the crossing, also both date from the 1830s, and were designed by german architect Carl Ludvig Engel, known in Finland for his several other highly regarded works. The walls and roof in the chancel are decorated with frescos in the Romantic style by the court painter Robert Wilhelm Ekman, which depict events from the life of Jesus, and the two key events in the history of the Finnish Church: the baptism of the first Finnish Christians by Bishop Henry by the spring at Kupittaa, and the presentation to King Gustav Vasa by the Reformer Michael Agricola of the first Finnish translation of the New Testament.
        </p><p>The Cathedral houses three organs. The current main organ of the Cathedral was built by Veikko Virtanen Oy of Espoo, Finland, in 1980 and features 81 ranks with a mechanical action.
        </p>

        <h2>Notable people buried in the cathedral</h2>
        <ul><li>Blessed Bishop Hemming (c. 1290–1366), Bishop of Turku</li>
        <li>Paulus Juusten (1516–1576), Bishop of Vyborg and later Bishop of Turku</li>
        <li>Karin Månsdotter (1550–1612), Queen of Sweden</li>
        <li>Princess Sigrid of Sweden (1566–1633), Swedish princess</li>
        <li>Samuel Cockburn (1574–1621), Scottish mercenary leader</li>
        <li>Torsten Stålhandske (1593–1644), officer in the Swedish army during the Thirty Years' War</li>
        <li>Åke Henriksson Tott (1598–1640), Swedish soldier and politician</li></ul>

        <h2>Gallery</h2>
        <ul class="gallery mw-gallery-traditional"><li class="gallerybox" style="width: 155px">
        <li class="gallerybox" style="width: 155px">
        <li class="gallerybox" style="width: 155px">
        <li class="gallerybox" style="width: 155px">
        <li class="gallerybox" style="width: 155px">
        <li class="gallerybox" style="width: 155px">
        <li class="gallerybox" style="width: 155px">
        </ul>

        <h2>See also</h2>
        <ul><li>Archdiocese of Turku</li>
        <li>Evangelical Lutheran Church of Finland</li>
        <li>Great Fire of Turku</li>
        <li>Helena Escholin</li>
        <li>List of tallest churches in the world</li>
        <li>Turku</li></ul>

        <h2>References</h2>
        <h2>External links</h2>
        <ul><li><span><span>Official website</span></span></li>
        <li>Virtual Turku – sound and images from the cathedral</li>
        <li>Medieval Turku</li>
        <li>Turku Cathedral – photos</li>
        <li>Turku church organs</li></ul>
        )"
      },
      {
        "ru",
        R"(
        <p><b>Кафедра́льный собо́р Ту́рку</b> (швед. <span lang="sv">Åbo domkyrka</span>, фин. <span lang="fi">Turun tuomiokirkko</span>) — главный лютеранский храм в Финляндии. Построен во второй половине XIII века, освящён в 1300 году в честь Девы Марии и первого епископа страны — святого Генриха, крестившего Финляндию.
        </p>

        <h2>История</h2>
        <p>Собор был заложен в 1258 году и построен в северо-готическом стиле, ставшим на долгое время образцом для строительства других церквей в Финляндии. Первый каменный собор был намного меньше нынешнего. Его фасад находился на том месте, где ныне расположена кафедра. Ниже был и свод, перекрывавший пространство.
        </p><p>В Средние века собор неоднократно перестраивался и расширялся. В XV веке к собору были пристроены боковые капеллы. Немного позже высота свода центрального нефа была увеличена до современных размеров (24 метра). В 1827 году собор серьёзно пострадал от пожара. 101-метровая башня собора была построена при восстановлении собора и стала символом города Турку.
        </p><p>Внутреннее убранство собора выполнено в 1830-х гг. Фредриком Вестином и Карлом Энгелем. Стены и свод алтарной части украшены фресками Р. В. Экмана. В 1870-х гг. северные капеллы были украшены витражами работы Владимира Сверчкова.
        </p><p>В 1980 году в соборе был установлен новый 81-регистровый орган.
        </p>

        <p>В капелле Святого Георгия похоронен епископ Хемминг, причисленный к лику святых. Наиболее известное надгробие собора — саркофаг Катарины Монсдоттэр, жены короля Эрика XIV. В разных частях собора также покоятся Мауну II Таваст, Олави Маунунпойка, Конрад Биц, Мауну III Сяркилахти, Кнут Поссе, Павел Юстен, Исаак Ротовиус, Турстэн Столхандскэ, Окэ Тотт, Эверт Хурн.
        </p><p>В южной галерее собора расположен Кафедральный музей, открытый в 1982 году после завершения реставрационных работ.
        </p><p>Перед собором установлен памятник реформатору церкви Микаэлю Агриколе.
        </p>

        <h2>Ссылки</h2>
        <ul><li class="mw-empty-elt">
        <li> На Викискладе есть медиафайлы по теме Абосский собор</li></ul>
        <p><br></p>
        )"
      }
    }
  }
};
}  // namespace generator_tests
