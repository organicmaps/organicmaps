#include "tizen_utils.hpp"
#include "../../std/vector.hpp"
#include "../../tizen/inc/FBase.hpp"
#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include <FSystem.h>


string FromTizenString(Tizen::Base::String const & str_tizen)
{
  string utf8Str;
  if (str_tizen.GetLength() == 0)
    return utf8Str;
  Tizen::Base::ByteBuffer * pBuffer = Tizen::Base::Utility::StringUtil::StringToUtf8N(str_tizen);
  if (pBuffer)
  {
    int byteCount = pBuffer->GetLimit() - 1; // Don't copy Zero at the end
    if (byteCount > 0)
    {
      vector<char> chBuf(byteCount);
      pBuffer->GetArray((byte *)&chBuf[0], 0, byteCount);
      utf8Str.assign(chBuf.begin(), chBuf.end());
    }
    delete pBuffer;
  }
  return utf8Str;
}

string GetTizenLocale()
{
  Tizen::Base::String languageCode;
  Tizen::System::SettingInfo::GetValue(L"http://tizen.org/setting/locale.language", languageCode);
  Tizen::Base::String languageCode_truncated;
  languageCode.SubString(0, 3, languageCode_truncated);
  return CodeFromISO369_2to_1(FromTizenString(languageCode_truncated));
}

string CodeFromISO369_2to_1(string const & code)
{
  static char const * ar [] =
  {
    "aar",	"aa",
    "abk",	"ab",
    "afr",	"af",
    "aka",	"ak",
    "sqi",	"sq",
    "amh",	"am",
    "ara",	"ar",
    "arg",	"an",
    "hye",	"hy",
    "asm",	"as",
    "ava",	"av",
    "ave",	"ae",
    "aym",	"ay",
    "aze",	"az",
    "bak",	"ba",
    "bam",	"bm",
    "eus",	"eu",
    "bel",	"be",
    "ben",	"bn",
    "bih",	"bh",
    "bis",	"bi",
    "bod",	"bo",
    "bos",	"bs",
    "bre",	"br",
    "bul",	"bg",
    "mya",	"my",
    "cat",	"ca",
    "ces",	"cs",
    "cha",	"ch",
    "che",	"ce",
    "zho",	"zh",
    "chu",	"cu",
    "chv",	"cv",
    "cor",	"kw",
    "cos",	"co",
    "cre",	"cr",
    "cym",	"cy",
    "ces",	"cs",
    "dan",	"da",
    "deu",	"de",
    "div",	"dv",
    "nld",	"nl",
    "dzo",	"dz",
    "ell",	"el",
    "eng",	"en",
    "epo",	"eo",
    "est",	"et",
    "eus",	"eu",
    "ewe",	"ee",
    "fao",	"fo",
    "fas",	"fa",
    "fij",	"fj",
    "fin",	"fi",
    "fra",	"fr",
    "fra",	"fr",
    "fry",	"fy",
    "ful",	"ff",
    "kat",	"ka",
    "deu",	"de",
    "gla",	"gd",
    "gle",	"ga",
    "glg",	"gl",
    "glv",	"gv",
    "ell",	"el",
    "grn",	"gn",
    "guj",	"gu",
    "hat",	"ht",
    "hau",	"ha",
    "heb",	"he",
    "her",	"hz",
    "hin",	"hi",
    "hmo",	"ho",
    "hrv",	"hr",
    "hun",	"hu",
    "hye",	"hy",
    "ibo",	"ig",
    "ice",	"is",
    "ido",	"io",
    "iii",	"ii",
    "iku",	"iu",
    "ile",	"ie",
    "ina",	"ia",
    "ind",	"id",
    "ipk",	"ik",
    "isl",	"is",
    "ita",	"it",
    "jav",	"jv",
    "jpn",	"ja",
    "kal",	"kl",
    "kan",	"kn",
    "kas",	"ks",
    "kat",	"ka",
    "kau",	"kr",
    "kaz",	"kk",
    "khm",	"km",
    "kik",	"ki",
    "kin",	"rw",
    "kir",	"ky",
    "kom",	"kv",
    "kon",	"kg",
    "kor",	"ko",
    "kua",	"kj",
    "kur",	"ku",
    "lao",	"lo",
    "lat",	"la",
    "lav",	"lv",
    "lim",	"li",
    "lin",	"ln",
    "lit",	"lt",
    "ltz",	"lb",
    "lub",	"lu",
    "lug",	"lg",
    "mkd",	"mk",
    "mah",	"mh",
    "mal",	"ml",
    "mri",	"mi",
    "mar",	"mr",
    "msa",	"ms",
    "mkd",	"mk",
    "mlg",	"mg",
    "mlt",	"mt",
    "mon",	"mn",
    "mri",	"mi",
    "msa",	"ms",
    "mya",	"my",
    "nau",	"na",
    "nav",	"nv",
    "nbl",	"nr",
    "nde",	"nd",
    "ndo",	"ng",
    "nep",	"ne",
    "nld",	"nl",
    "nno",	"nn",
    "nob",	"nb",
    "nor",	"no",
    "nya",	"ny",
    "oci",	"oc",
    "oji",	"oj",
    "ori",	"or",
    "orm",	"om",
    "oss",	"os",
    "pan",	"pa",
    "fas",	"fa",
    "pli",	"pi",
    "pol",	"pl",
    "por",	"pt",
    "pus",	"ps",
    "que",	"qu",
    "roh",	"rm",
    "ron",	"ro",
    "ron",	"ro",
    "run",	"rn",
    "rus",	"ru",
    "sag",	"sg",
    "san",	"sa",
    "sin",	"si",
    "slk",	"sk",
    "slk",	"sk",
    "slv",	"sl",
    "sme",	"se",
    "smo",	"sm",
    "sna",	"sn",
    "snd",	"sd",
    "som",	"so",
    "sot",	"st",
    "spa",	"es",
    "sqi",	"sq",
    "srd",	"sc",
    "srp",	"sr",
    "ssw",	"ss",
    "sun",	"su",
    "swa",	"sw",
    "swe",	"sv",
    "tah",	"ty",
    "tam",	"ta",
    "tat",	"tt",
    "tel",	"te",
    "tgk",	"tg",
    "tgl",	"tl",
    "tha",	"th",
    "bod",	"bo",
    "tir",	"ti",
    "ton",	"to",
    "tsn",	"tn",
    "tso",	"ts",
    "tuk",	"tk",
    "tur",	"tr",
    "twi",	"tw",
    "uig",	"ug",
    "ukr",	"uk",
    "urd",	"ur",
    "uzb",	"uz",
    "ven",	"ve",
    "vie",	"vi",
    "vol",	"vo",
    "cym",	"cy",
    "wln",	"wa",
    "wol",	"wo",
    "xho",	"xh",
    "yid",	"yi",
    "yor",	"yo",
    "zha",	"za",
    "zho",	"zh",
    "zul",	"zu"
  };
  for (size_t i = 0; i < ARRAY_SIZE(ar); i += 2)
  {
    if (code == ar[i])
    {
      return ar[i + 1];
    }
  }
  LOG(LDEBUG, ("Language not found", code));
  return "en";
}
