#include "testing/testing.hpp"

#include "descriptions/serdes.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <map>
#include <string>
#include <vector>

using namespace descriptions;

struct RawDescription
{
  FeatureIndex m_idx;
  std::vector<std::pair<LangCode, std::string>> m_strings;
};

template <typename Reader>
std::string GetDescription(Reader & reader, FeatureIndex fid, std::vector<int8_t> const & langPriority)
{
  Deserializer des;
  return des.Deserialize(reader, fid, langPriority);
}

DescriptionsCollection Convert(std::vector<RawDescription> const & rawDescriptions)
{
  DescriptionsCollection descriptions;
  for (auto const & desc : rawDescriptions)
  {
    descriptions.m_features.push_back({});
    FeatureDescription & ftDesc = descriptions.m_features.back();
    ftDesc.m_ftIndex = desc.m_idx;

    for (auto const & translation : desc.m_strings)
    {
      ftDesc.m_strIndices.emplace_back(translation.first, descriptions.m_strings.size());
      descriptions.m_strings.push_back(translation.second);
    }
  }
  return descriptions;
}

UNIT_TEST(Descriptions_SerDes)
{
  std::vector<RawDescription> const data = {
      {100, {{10, "Description of feature 100, language 10."}, {11, "Описание фичи 100, язык 11."}}},
      {101, {{11, "Описание фичи 101, язык 11."}}},
      {102, {{11, "Описание фичи 102, язык 11."}, {10, "Description of feature 102, language 10."}}}};

  auto testData = [](MemReader & reader)
  {
    TEST_EQUAL(GetDescription(reader, 102, {11, 10}), "Описание фичи 102, язык 11.", ());
    TEST_EQUAL(GetDescription(reader, 100, {12, 10}), "Description of feature 100, language 10.", ());
    TEST_EQUAL(GetDescription(reader, 101, {12}), "", ());
    TEST_EQUAL(GetDescription(reader, 0, {10, 11}), "", ());
    TEST_EQUAL(GetDescription(reader, 102, {10}), "Description of feature 102, language 10.", ());
  };

  {
    std::vector<uint8_t> buffer;
    {
      auto descriptionsCollection = Convert(data);
      Serializer ser(std::move(descriptionsCollection));
      MemWriter<decltype(buffer)> writer(buffer);
      ser.Serialize(writer);
    }

    MemReader reader(buffer.data(), buffer.size());

    testData(reader);
  }

  {
    size_t const kDummyBytesCount = 100;
    std::vector<uint8_t> buffer(kDummyBytesCount);
    {
      auto descriptionsCollection = Convert(data);
      Serializer ser(std::move(descriptionsCollection));
      MemWriter<decltype(buffer)> writer(buffer);
      writer.Seek(kDummyBytesCount);
      ser.Serialize(writer);

      std::vector<uint8_t> buffer2(kDummyBytesCount);
      writer.Write(buffer2.data(), buffer2.size());
    }

    MemReader reader(buffer.data(), buffer.size());
    auto subReader = reader.SubReader(kDummyBytesCount, buffer.size() - kDummyBytesCount);

    testData(subReader);
  }
}

UNIT_TEST(Descriptions_Html)
{
  std::vector<RawDescription> const data = {
      {100,
       {{1,
         "<div class=\"section\">\n"
         "<p lang=\"en\">Map data &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> contributors, <a href=\"http://opendatacommons.org/licenses/odbl/\">ODbL</a>.</p>\n"
         "</div>"},
        {2,
         "<div class=\"section\">\n"
         "<p lang=\"ru\">Картографические данные &copy; участники <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a>, <a href=\"http://opendatacommons.org/licenses/odbl/\">ODbL</a>.</p>\n"
         "</div>"},
        {3,
         "<div class=\"section\">\n"
         "<p lang=\"vi\">Dữ liệu bản đồ &copy; Cộng tác viên của <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a>, ODbL.</p>\n"
         "</div>"},
        {4,
         "<div class=\"section\">\n"
         "<p lang=\"tr\">Harita verileri &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> katkıları, ODbL.</p>\n"
         "</div>"},
        {5,
         "<div class=\"section\">\n"
         "<p lang=\"th\">ข้อมูลแผนที่ &copy; ผู้มีส่วนร่วม <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> ODbL.</p>\n"
         "</div>"},
        {6,
         "<div class=\"section\">\n"
         "<p lang=\"sv\">Map data &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a>-bidragsgivare, ODbL.</p>\n"
         "</div>"},
        {7,
         "<div class=\"section\">\n"
         "<p lang=\"es\">Contribuidores de los datos de mapas &copy;"
         " <a href=\"https://www.openstreetmap.org/\">OpenStreetMap</a>, Licencia ODbL.</p>\n"
         "</div>"},
        {8,
         "<div class=\"section\">\n"
         "<p lang=\"pt\">Dados de mapas de contribuintes do &copy;"
         " <a href=\"https://www.openstreetmap.org/\">OpenStreetMap</a>, ODbL.</p>\n"
         "</div>"},
        {9,
         "<div class=\"section\">\n"
         "<p lang=\"pl\">Dane map &copy; Współautorzy"
         " <a href=\"https://www.openstreetmap.org/\">OpenStreetMap</a>, ODbL.</p>\n"
         "</div>"},
        {10,
         "<div class=\"section\">\n"
         "<p lang=\"nb\">Kartdata &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> bidragsytere, ODbL.</p>\n"
         "</div>"},
        {11,
         "<div class=\"section\">\n"
         "<p lang=\"ko\">지도 데이터 &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> contributors, ODbL.</p>\n"
         "</div>"},
        {12,
         "<div class=\"section\">\n"
         "<p lang=\"ja\">地図データ &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a>貢献者、ODbL。</p>\n"
         "</div>"},
        {13,
         "<div class=\"section\">\n"
         "<p lang=\"it\">Dati delle mappe &copy; Contenuti <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a>, ODbL.</p>\n"
         "</div>"},
        {14,
         "<div class=\"section\">\n"
         "<p lang=\"id\">Data Peta &copy; Kontributor <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a>, ODbL.</p>\n"
         "</div>"},
        {15,
         "<div class=\"section\">\n"
         "<p lang=\"hu\">Térképadat &copy; az <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> közreműködői, ODbL.</p>\n"
         "</div>"},
        {16,
         "<div class=\"section\">\n"
         "<p lang=\"de\">Kartendaten &copy; <a href=\"http://www.opentreetmap.org/\">"
         "OpenStreetMap</a>-Mitwirkende ODbL.</p>\n"
         "</div>"},
        {17,
         "<div class=\"section\">\n"
         "<p lang=\"fr\">Données de la carte sous &copy; des contributeurs d'"
         "<a href=\"https://www.openstreetmap.org/\">OpenStreetMap</a>, licence ODbL.</p>\n"
         "</div>"},
        {18,
         "<div class=\"section\">\n"
         "<p lang=\"fi\">Karttatiedot &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a>-avustajat, ODbL.</p>\n"
         "</div>"},
        {19,
         "<div class=\"section\">\n"
         "<p lang=\"nl\">Kaartgegevens &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> bjdragers, ODbL.</p>\n"
         "</div>"},
        {20,
         "<div class=\"section\">\n"
         "<p lang=\"cs\">Mapová data &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> přispěvatelé, ODbL.</p>\n"
         "</div>"},
        {21,
         "<div class=\"section\">\n"
         "<p lang=\"zh-Hans\">地图数据 &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> 贡献者, ODbL.</p>\n"
         "</div>"},
        {22,
         "<div class=\"section\">\n"
         "<p lang=\"zh-Hant\">地圖數據 &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> 貢獻者, ODbL.</p>\n"
         "</div>"},
        {23,
         "<div class=\"section\">\n"
         "<p lang=\"ar\">المساهمون في بيانات خريطة &copy; <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a> و ODbL.</p>\n"
         "</div>"},
        {24,
         "<div class=\"section\">\n"
         "<p lang=\"uk\">Картографічні дані &copy; учасники <a href=\"https://www.openstreetmap.org/\">"
         "OpenStreetMap</a>, ODbL.</p>"
         "</div>"}}},
      {101,
       {{1,
         "<p lang=\"en\"><strong>omaps.app</strong> would not be possible without the generous contributions of "
         "the following projects:</p>\n"},
        {2,
         "<p lang=\"ru\">Приложение <strong>omaps.app</strong> было бы невозможно без участия следующих"
         " проектов:</p>\n"},
        {3,
         "<p lang=\"vi\"><strong>omaps.app</strong> sẽ không thành hiện thực nếu không có sự đóng góp hào phóng"
         " từ các dự án sau:</p>\n"},
        {4,
         "<p lang=\"tr\"><strong>omaps.app</strong>, aşağıdaki projelerin cömert katkıları olmadan mümkün "
         "olmazdı:</p>\n"},
        {5,
         "<p lang=\"th\"><strong>omaps.app</strong> จะสำเร็จลุล่วงไม่ได้เลยหากปราศจากความเอื้อเฟื้อเพื่อการร่วมมือของโป"
         "รเจกต์ดังต่อไปนี้:</p>\n"},
        {6,
         "<p lang=\"sv\"><strong>omaps.app</strong> skulle inte vara möjlig utan följande projekts generösa"
         " bidrag:</p>\n"},
        {7,
         "<p lang=\"es\"><strong>omaps.app</strong> no sería posible sin las generosas aportaciones de los"
         " siguientes proyectos:</p>\n"},
        {8,
         "<p lang=\"pt\">O <strong>omaps.app</strong> não seria possível sem as contribuições generosas dos "
         "seguintes projetos:</p>\n"},
        {9,
         "<p lang=\"pl\">Aplikacja <strong>omaps.app</strong> nie powstałaby bez znaczącego wkładu ze strony "
         "twórców poniższych projektów:</p>\n"},
        {10,
         "<p lang=\"nb\"><strong>omaps.app</strong> ville ikke vært mulig uten de generøse bidragene fra "
         "følgende prosjekter:</p>\n"},
        {11, "<p lang=\"ko\"><strong>omaps.app</strong>는 다음 프로젝트의 아낌없는 기부없이 가능하지 않습니다:</p>\n"},
        {12, "<p lang=\"ja\"><strong>omaps.app</strong>は次のプロジェクトの手厚い貢献なしには不可能です：</p>\n"},
        {13,
         "<p lang=\"it\"><strong>omaps.app</strong> non sarebbe realizzabile senza il generoso contributo "
         "dei seguenti progetti:</p>\n"},
        {14,
         "<p lang=\"id\"><strong>omaps.app</strong> tidak mungkin tercipta tanpa kontribusi yang tulus dari "
         "proyek-proyek berikut ini:</p>\n"},
        {15,
         "<p lang=\"hu\">A <strong>omaps.app</strong> nem jöhetett volna létre az alábbi projektek nagylelkű "
         "közreműködése nélkül:</p>\n"},
        {16,
         "<p lang=\"de\"><strong>omaps.app</strong> wäre ohne die großzügigen Spenden der folgenden Projekte "
         "nicht möglich:</p>\n"},
        {17,
         "<p lang=\"fr\">L'existence de <strong>omaps.app</strong> serait impossible sans les généreuses "
         "contributions des projets suivants :</p>\n"},
        {18,
         "<p lang=\"fi\"><strong>omaps.app</strong> ei olisi mahdollinen ilman seuraavien projektien aulista "
         "tukea:</p>\n"},
        {19,
         "<p lang=\"nl\"><strong>omaps.app</strong> zou niet mogelijk zijn zonder de genereuze bijdragen voor "
         "de volgende projecten:</p>\n"},
        {20,
         "<p lang=\"cs\"><strong>omaps.app</strong> by nemohlo existovat bez štědrých přispění následujících "
         "projektů:</p>\n"},
        {21, "<p lang=\"zh-Hans\">沒有下面項目的慷慨貢獻，<strong>omaps.app</strong> 不可能出現：</p>\n"},
        {22, "<p lang=\"zh-Hant\">沒有下面項目的慷慨貢獻，<strong>omaps.app</strong> 不可能出現：</p>\n"},
        {23,
         "<p lang=\"ar\">    ما كان لـ <strong>maps.me"
         "</strong> أن تأتي للوجود بدون المساهمات العظيمة للمشاريع التالية:</p>"},
        {24, "<p lang=\"uk\"><strong>omaps.app</strong> був би неможливим без щедрої участі таких проектів:</p>"}}},
  };

  {
    std::vector<uint8_t> buffer;
    {
      auto descriptionsCollection = Convert(data);
      Serializer ser(std::move(descriptionsCollection));
      MemWriter<decltype(buffer)> writer(buffer);
      ser.Serialize(writer);
    }

    MemReader reader(buffer.data(), buffer.size());

    for (auto const & rawDesc : data)
      for (auto const & translation : rawDesc.m_strings)
        TEST_EQUAL(GetDescription(reader, rawDesc.m_idx, {translation.first}), translation.second, ());
  }
}
