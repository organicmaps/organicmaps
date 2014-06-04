#include "tizen_utils.hpp"
#include "../../std/vector.hpp"
#include "../../tizen/inc/FBase.hpp"
#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include <FSystem.h>
#include <FLocales.h>

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

string GetLanguageCode(Tizen::Locales::LanguageCode code)
{
  using namespace Tizen::Locales;
  switch(code)
  {
  case LANGUAGE_INVALID: return "";
  case LANGUAGE_AAR: return "aar";
  case LANGUAGE_ABK: return "abk";
  case LANGUAGE_ACE: return "ace";
  case LANGUAGE_ACH: return "ach";
  case LANGUAGE_ADA: return "ada";
  case LANGUAGE_ADY: return "ady";
  case LANGUAGE_AFA: return "afa";
  case LANGUAGE_AFH: return "afh";
  case LANGUAGE_AFR: return "afr";
  case LANGUAGE_AIN: return "ain";
  case LANGUAGE_AKA: return "aka";
  case LANGUAGE_AKK: return "akk";
  case LANGUAGE_SQI: return "sqi";
  case LANGUAGE_ALE: return "ale";
  case LANGUAGE_ALG: return "alg";
  case LANGUAGE_ALT: return "alt";
  case LANGUAGE_AMH: return "amh";
  case LANGUAGE_ANG: return "ang";
  case LANGUAGE_ANP: return "anp";
  case LANGUAGE_APA: return "apa";
  case LANGUAGE_ARA: return "ara";
  case LANGUAGE_ARC: return "arc";
  case LANGUAGE_ARG: return "arg";
  case LANGUAGE_HYE: return "hye";
  case LANGUAGE_ARN: return "arn";
  case LANGUAGE_ARP: return "arp";
  case LANGUAGE_ART: return "art";
  case LANGUAGE_ARW: return "arw";
  case LANGUAGE_ASM: return "asm";
  case LANGUAGE_AST: return "ast";
  case LANGUAGE_ATH: return "ath";
  case LANGUAGE_AUS: return "aus";
  case LANGUAGE_AVA: return "ava";
  case LANGUAGE_AVE: return "ave";
  case LANGUAGE_AWA: return "awa";
  case LANGUAGE_AYM: return "aym";
  case LANGUAGE_AZE: return "aze";
  case LANGUAGE_BAD: return "bad";
  case LANGUAGE_BAI: return "bai";
  case LANGUAGE_BAK: return "bak";
  case LANGUAGE_BAL: return "bal";
  case LANGUAGE_BAM: return "bam";
  case LANGUAGE_BAN: return "ban";
  case LANGUAGE_EUS: return "eus";
  case LANGUAGE_BAS: return "bas";
  case LANGUAGE_BAT: return "bat";
  case LANGUAGE_BEJ: return "bej";
  case LANGUAGE_BEL: return "bel";
  case LANGUAGE_BEM: return "bem";
  case LANGUAGE_BEN: return "ben";
  case LANGUAGE_BER: return "ber";
  case LANGUAGE_BHO: return "bho";
  case LANGUAGE_BIH: return "bih";
  case LANGUAGE_BIK: return "bik";
  case LANGUAGE_BIN: return "bin";
  case LANGUAGE_BIS: return "bis";
  case LANGUAGE_BLA: return "bla";
  case LANGUAGE_BNT: return "bnt";
  case LANGUAGE_BOS: return "bos";
  case LANGUAGE_BRA: return "bra";
  case LANGUAGE_BRE: return "bre";
  case LANGUAGE_BTK: return "btk";
  case LANGUAGE_BUA: return "bua";
  case LANGUAGE_BUG: return "bug";
  case LANGUAGE_BUL: return "bul";
  case LANGUAGE_MYA: return "mya";
  case LANGUAGE_BYN: return "byn";
  case LANGUAGE_CAD: return "cad";
  case LANGUAGE_CAI: return "cai";
  case LANGUAGE_CAR: return "car";
  case LANGUAGE_CAT: return "cat";
  case LANGUAGE_CAU: return "cau";
  case LANGUAGE_CEB: return "ceb";
  case LANGUAGE_CEL: return "cel";
  case LANGUAGE_CHA: return "cha";
  case LANGUAGE_CHB: return "chb";
  case LANGUAGE_CHE: return "che";
  case LANGUAGE_CHG: return "chg";
  case LANGUAGE_ZHO: return "zho";
  case LANGUAGE_CHK: return "chk";
  case LANGUAGE_CHM: return "chm";
  case LANGUAGE_CHN: return "chn";
  case LANGUAGE_CHO: return "cho";
  case LANGUAGE_CHP: return "chp";
  case LANGUAGE_CHR: return "chr";
  case LANGUAGE_CHU: return "chu";
  case LANGUAGE_CHV: return "chv";
  case LANGUAGE_CHY: return "chy";
  case LANGUAGE_CMC: return "cmc";
  case LANGUAGE_COP: return "cop";
  case LANGUAGE_COR: return "cor";
  case LANGUAGE_COS: return "cos";
  case LANGUAGE_CPE: return "cpe";
  case LANGUAGE_CPF: return "cpf";
  case LANGUAGE_CPP: return "cpp";
  case LANGUAGE_CRE: return "cre";
  case LANGUAGE_CRH: return "crh";
  case LANGUAGE_CRP: return "crp";
  case LANGUAGE_CSB: return "csb";
  case LANGUAGE_CUS: return "cus";
  case LANGUAGE_CES: return "ces";
  case LANGUAGE_DAK: return "dak";
  case LANGUAGE_DAN: return "dan";
  case LANGUAGE_DAR: return "dar";
  case LANGUAGE_DAY: return "day";
  case LANGUAGE_DEL: return "del";
  case LANGUAGE_DEN: return "den";
  case LANGUAGE_DGR: return "dgr";
  case LANGUAGE_DIN: return "din";
  case LANGUAGE_DIV: return "div";
  case LANGUAGE_DOI: return "doi";
  case LANGUAGE_DRA: return "dra";
  case LANGUAGE_DSB: return "dsb";
  case LANGUAGE_DUA: return "dua";
  case LANGUAGE_DUM: return "dum";
  case LANGUAGE_NLD: return "nld";
  case LANGUAGE_DYU: return "dyu";
  case LANGUAGE_DZO: return "dzo";
  case LANGUAGE_EFI: return "efi";
  case LANGUAGE_EGY: return "egy";
  case LANGUAGE_EKA: return "eka";
  case LANGUAGE_ELX: return "elx";
  case LANGUAGE_ENG: return "eng";
  case LANGUAGE_ENM: return "enm";
  case LANGUAGE_EPO: return "epo";
  case LANGUAGE_EST: return "est";
  case LANGUAGE_EWE: return "ewe";
  case LANGUAGE_EWO: return "ewo";
  case LANGUAGE_FAN: return "fan";
  case LANGUAGE_FAO: return "fao";
  case LANGUAGE_FAT: return "fat";
  case LANGUAGE_FIJ: return "fij";
  case LANGUAGE_FIL: return "fil";
  case LANGUAGE_FIN: return "fin";
  case LANGUAGE_FIU: return "fiu";
  case LANGUAGE_FON: return "fon";
  case LANGUAGE_FRA: return "fra";
  case LANGUAGE_FRM: return "frm";
  case LANGUAGE_FRO: return "fro";
  case LANGUAGE_FRR: return "frr";
  case LANGUAGE_FRS: return "frs";
  case LANGUAGE_FRY: return "fry";
  case LANGUAGE_FUL: return "ful";
  case LANGUAGE_FUR: return "fur";
  case LANGUAGE_GAA: return "gaa";
  case LANGUAGE_GAY: return "gay";
  case LANGUAGE_GBA: return "gba";
  case LANGUAGE_GEM: return "gem";
  case LANGUAGE_KAT: return "kat";
  case LANGUAGE_DEU: return "deu";
  case LANGUAGE_GEZ: return "gez";
  case LANGUAGE_GIL: return "gil";
  case LANGUAGE_GLA: return "gla";
  case LANGUAGE_GLE: return "gle";
  case LANGUAGE_GLG: return "glg";
  case LANGUAGE_GLV: return "glv";
  case LANGUAGE_GMH: return "gmh";
  case LANGUAGE_GOH: return "goh";
  case LANGUAGE_GON: return "gon";
  case LANGUAGE_GOR: return "gor";
  case LANGUAGE_GOT: return "got";
  case LANGUAGE_GRB: return "grb";
  case LANGUAGE_GRC: return "grc";
  case LANGUAGE_ELL: return "ell";
  case LANGUAGE_GRN: return "grn";
  case LANGUAGE_GSW: return "gsw";
  case LANGUAGE_GUJ: return "guj";
  case LANGUAGE_GWI: return "gwi";
  case LANGUAGE_HAI: return "hai";
  case LANGUAGE_HAT: return "hat";
  case LANGUAGE_HAU: return "hau";
  case LANGUAGE_HAW: return "haw";
  case LANGUAGE_HEB: return "heb";
  case LANGUAGE_HER: return "her";
  case LANGUAGE_HIL: return "hil";
  case LANGUAGE_HIM: return "him";
  case LANGUAGE_HIN: return "hin";
  case LANGUAGE_HIT: return "hit";
  case LANGUAGE_HMN: return "hmn";
  case LANGUAGE_HMO: return "hmo";
  case LANGUAGE_HRV: return "hrv";
  case LANGUAGE_HSB: return "hsb";
  case LANGUAGE_HUN: return "hun";
  case LANGUAGE_HUP: return "hup";
  case LANGUAGE_IBA: return "iba";
  case LANGUAGE_IBO: return "ibo";
  case LANGUAGE_ISL: return "isl";
  case LANGUAGE_IDO: return "ido";
  case LANGUAGE_III: return "iii";
  case LANGUAGE_IJO: return "ijo";
  case LANGUAGE_IKU: return "iku";
  case LANGUAGE_ILE: return "ile";
  case LANGUAGE_ILO: return "ilo";
  case LANGUAGE_INA: return "ina";
  case LANGUAGE_INC: return "inc";
  case LANGUAGE_IND: return "ind";
  case LANGUAGE_INE: return "ine";
  case LANGUAGE_INH: return "inh";
  case LANGUAGE_IPK: return "ipk";
  case LANGUAGE_IRA: return "ira";
  case LANGUAGE_IRO: return "iro";
  case LANGUAGE_ITA: return "ita";
  case LANGUAGE_JAV: return "jav";
  case LANGUAGE_JBO: return "jbo";
  case LANGUAGE_JPN: return "jpn";
  case LANGUAGE_JPR: return "jpr";
  case LANGUAGE_JRB: return "jrb";
  case LANGUAGE_KAA: return "kaa";
  case LANGUAGE_KAB: return "kab";
  case LANGUAGE_KAC: return "kac";
  case LANGUAGE_KAL: return "kal";
  case LANGUAGE_KAM: return "kam";
  case LANGUAGE_KAN: return "kan";
  case LANGUAGE_KAR: return "kar";
  case LANGUAGE_KAS: return "kas";
  case LANGUAGE_KAU: return "kau";
  case LANGUAGE_KAW: return "kaw";
  case LANGUAGE_KAZ: return "kaz";
  case LANGUAGE_KBD: return "kbd";
  case LANGUAGE_KHA: return "kha";
  case LANGUAGE_KHI: return "khi";
  case LANGUAGE_KHM: return "khm";
  case LANGUAGE_KHO: return "kho";
  case LANGUAGE_KIK: return "kik";
  case LANGUAGE_KIN: return "kin";
  case LANGUAGE_KIR: return "kir";
  case LANGUAGE_KMB: return "kmb";
  case LANGUAGE_KOK: return "kok";
  case LANGUAGE_KOM: return "kom";
  case LANGUAGE_KON: return "kon";
  case LANGUAGE_KOR: return "kor";
  case LANGUAGE_KOS: return "kos";
  case LANGUAGE_KPE: return "kpe";
  case LANGUAGE_KRC: return "krc";
  case LANGUAGE_KRL: return "krl";
  case LANGUAGE_KRO: return "kro";
  case LANGUAGE_KRU: return "kru";
  case LANGUAGE_KUA: return "kua";
  case LANGUAGE_KUM: return "kum";
  case LANGUAGE_KUR: return "kur";
  case LANGUAGE_KUT: return "kut";
  case LANGUAGE_LAD: return "lad";
  case LANGUAGE_LAH: return "lah";
  case LANGUAGE_LAM: return "lam";
  case LANGUAGE_LAO: return "lao";
  case LANGUAGE_LAT: return "lat";
  case LANGUAGE_LAV: return "lav";
  case LANGUAGE_LEZ: return "lez";
  case LANGUAGE_LIM: return "lim";
  case LANGUAGE_LIN: return "lin";
  case LANGUAGE_LIT: return "lit";
  case LANGUAGE_LOL: return "lol";
  case LANGUAGE_LOZ: return "loz";
  case LANGUAGE_LTZ: return "ltz";
  case LANGUAGE_LUA: return "lua";
  case LANGUAGE_LUB: return "lub";
  case LANGUAGE_LUG: return "lug";
  case LANGUAGE_LUI: return "lui";
  case LANGUAGE_LUN: return "lun";
  case LANGUAGE_LUO: return "luo";
  case LANGUAGE_LUS: return "lus";
  case LANGUAGE_MKD: return "mkd";
  case LANGUAGE_MAD: return "mad";
  case LANGUAGE_MAG: return "mag";
  case LANGUAGE_MAH: return "mah";
  case LANGUAGE_MAI: return "mai";
  case LANGUAGE_MAK: return "mak";
  case LANGUAGE_MAL: return "mal";
  case LANGUAGE_MAN: return "man";
  case LANGUAGE_MRI: return "mri";
  case LANGUAGE_MAP: return "map";
  case LANGUAGE_MAR: return "mar";
  case LANGUAGE_MAS: return "mas";
  case LANGUAGE_MSA: return "msa";
  case LANGUAGE_MDF: return "mdf";
  case LANGUAGE_MDR: return "mdr";
  case LANGUAGE_MEN: return "men";
  case LANGUAGE_MGA: return "mga";
  case LANGUAGE_MIC: return "mic";
  case LANGUAGE_MIN: return "min";
  case LANGUAGE_MIS: return "mis";
  case LANGUAGE_MKH: return "mkh";
  case LANGUAGE_MLG: return "mlg";
  case LANGUAGE_MLT: return "mlt";
  case LANGUAGE_MNC: return "mnc";
  case LANGUAGE_MNI: return "mni";
  case LANGUAGE_MNO: return "mno";
  case LANGUAGE_MOH: return "moh";
  case LANGUAGE_MON: return "mon";
  case LANGUAGE_MOS: return "mos";
  case LANGUAGE_MUL: return "mul";
  case LANGUAGE_MUN: return "mun";
  case LANGUAGE_MUS: return "mus";
  case LANGUAGE_MWL: return "mwl";
  case LANGUAGE_MWR: return "mwr";
  case LANGUAGE_MYN: return "myn";
  case LANGUAGE_MYV: return "myv";
  case LANGUAGE_NAH: return "nah";
  case LANGUAGE_NAI: return "nai";
  case LANGUAGE_NAP: return "nap";
  case LANGUAGE_NAU: return "nau";
  case LANGUAGE_NAV: return "nav";
  case LANGUAGE_NBL: return "nbl";
  case LANGUAGE_NDE: return "nde";
  case LANGUAGE_NDO: return "ndo";
  case LANGUAGE_NDS: return "nds";
  case LANGUAGE_NEP: return "nep";
  case LANGUAGE_NEW: return "new";
  case LANGUAGE_NIA: return "nia";
  case LANGUAGE_NIC: return "nic";
  case LANGUAGE_NIU: return "niu";
  case LANGUAGE_NNO: return "nno";
  case LANGUAGE_NOB: return "nob";
  case LANGUAGE_NOG: return "nog";
  case LANGUAGE_NON: return "non";
  case LANGUAGE_NOR: return "nor";
  case LANGUAGE_NQO: return "nqo";
  case LANGUAGE_NSO: return "nso";
  case LANGUAGE_NUB: return "nub";
  case LANGUAGE_NWC: return "nwc";
  case LANGUAGE_NYA: return "nya";
  case LANGUAGE_NYM: return "nym";
  case LANGUAGE_NYN: return "nyn";
  case LANGUAGE_NYO: return "nyo";
  case LANGUAGE_NZI: return "nzi";
  case LANGUAGE_OCI: return "oci";
  case LANGUAGE_OJI: return "oji";
  case LANGUAGE_ORI: return "ori";
  case LANGUAGE_ORM: return "orm";
  case LANGUAGE_OSA: return "osa";
  case LANGUAGE_OSS: return "oss";
  case LANGUAGE_OTA: return "ota";
  case LANGUAGE_OTO: return "oto";
  case LANGUAGE_PAA: return "paa";
  case LANGUAGE_PAG: return "pag";
  case LANGUAGE_PAL: return "pal";
  case LANGUAGE_PAM: return "pam";
  case LANGUAGE_PAN: return "pan";
  case LANGUAGE_PAP: return "pap";
  case LANGUAGE_PAU: return "pau";
  case LANGUAGE_PEO: return "peo";
  case LANGUAGE_FAS: return "fas";
  case LANGUAGE_PHI: return "phi";
  case LANGUAGE_PHN: return "phn";
  case LANGUAGE_PLI: return "pli";
  case LANGUAGE_POL: return "pol";
  case LANGUAGE_PON: return "pon";
  case LANGUAGE_POR: return "por";
  case LANGUAGE_PRA: return "pra";
  case LANGUAGE_PRO: return "pro";
  case LANGUAGE_PUS: return "pus";
  case LANGUAGE_QUE: return "que";
  case LANGUAGE_RAJ: return "raj";
  case LANGUAGE_RAP: return "rap";
  case LANGUAGE_RAR: return "rar";
  case LANGUAGE_ROA: return "roa";
  case LANGUAGE_ROH: return "roh";
  case LANGUAGE_ROM: return "rom";
  case LANGUAGE_RON: return "ron";
  case LANGUAGE_RUN: return "run";
  case LANGUAGE_RUP: return "rup";
  case LANGUAGE_RUS: return "rus";
  case LANGUAGE_SAD: return "sad";
  case LANGUAGE_SAG: return "sag";
  case LANGUAGE_SAH: return "sah";
  case LANGUAGE_SAI: return "sai";
  case LANGUAGE_SAL: return "sal";
  case LANGUAGE_SAM: return "sam";
  case LANGUAGE_SAN: return "san";
  case LANGUAGE_SAS: return "sas";
  case LANGUAGE_SAT: return "sat";
  case LANGUAGE_SCN: return "scn";
  case LANGUAGE_SCO: return "sco";
  case LANGUAGE_SEL: return "sel";
  case LANGUAGE_SEM: return "sem";
  case LANGUAGE_SGA: return "sga";
  case LANGUAGE_SGN: return "sgn";
  case LANGUAGE_SHN: return "shn";
  case LANGUAGE_SID: return "sid";
  case LANGUAGE_SIN: return "sin";
  case LANGUAGE_SIO: return "sio";
  case LANGUAGE_SIT: return "sit";
  case LANGUAGE_SLA: return "sla";
  case LANGUAGE_SLK: return "slk";
  case LANGUAGE_SLV: return "slv";
  case LANGUAGE_SMA: return "sma";
  case LANGUAGE_SME: return "sme";
  case LANGUAGE_SMI: return "smi";
  case LANGUAGE_SMJ: return "smj";
  case LANGUAGE_SMN: return "smn";
  case LANGUAGE_SMO: return "smo";
  case LANGUAGE_SMS: return "sms";
  case LANGUAGE_SNA: return "sna";
  case LANGUAGE_SND: return "snd";
  case LANGUAGE_SNK: return "snk";
  case LANGUAGE_SOG: return "sog";
  case LANGUAGE_SOM: return "som";
  case LANGUAGE_SON: return "son";
  case LANGUAGE_SOT: return "sot";
  case LANGUAGE_SPA: return "spa";
  case LANGUAGE_SRD: return "srd";
  case LANGUAGE_SRN: return "srn";
  case LANGUAGE_SRP: return "srp";
  case LANGUAGE_SRR: return "srr";
  case LANGUAGE_SSA: return "ssa";
  case LANGUAGE_SSW: return "ssw";
  case LANGUAGE_SUK: return "suk";
  case LANGUAGE_SUN: return "sun";
  case LANGUAGE_SUS: return "sus";
  case LANGUAGE_SUX: return "sux";
  case LANGUAGE_SWA: return "swa";
  case LANGUAGE_SWE: return "swe";
  case LANGUAGE_SYC: return "syc";
  case LANGUAGE_SYR: return "syr";
  case LANGUAGE_TAH: return "tah";
  case LANGUAGE_TAI: return "tai";
  case LANGUAGE_TAM: return "tam";
  case LANGUAGE_TAT: return "tat";
  case LANGUAGE_TEL: return "tel";
  case LANGUAGE_TEM: return "tem";
  case LANGUAGE_TER: return "ter";
  case LANGUAGE_TET: return "tet";
  case LANGUAGE_TGK: return "tgk";
  case LANGUAGE_TGL: return "tgl";
  case LANGUAGE_THA: return "tha";
  case LANGUAGE_BOD: return "bod";
  case LANGUAGE_TIG: return "tig";
  case LANGUAGE_TIR: return "tir";
  case LANGUAGE_TIV: return "tiv";
  case LANGUAGE_TKL: return "tkl";
  case LANGUAGE_TLH: return "tlh";
  case LANGUAGE_TLI: return "tli";
  case LANGUAGE_TMH: return "tmh";
  case LANGUAGE_TOG: return "tog";
  case LANGUAGE_TON: return "ton";
  case LANGUAGE_TPI: return "tpi";
  case LANGUAGE_TSI: return "tsi";
  case LANGUAGE_TSN: return "tsn";
  case LANGUAGE_TSO: return "tso";
  case LANGUAGE_TUK: return "tuk";
  case LANGUAGE_TUM: return "tum";
  case LANGUAGE_TUP: return "tup";
  case LANGUAGE_TUR: return "tur";
  case LANGUAGE_TUT: return "tut";
  case LANGUAGE_TVL: return "tvl";
  case LANGUAGE_TWI: return "twi";
  case LANGUAGE_TYV: return "tyv";
  case LANGUAGE_UDM: return "udm";
  case LANGUAGE_UGA: return "uga";
  case LANGUAGE_UIG: return "uig";
  case LANGUAGE_UKR: return "ukr";
  case LANGUAGE_UMB: return "umb";
  case LANGUAGE_UND: return "und";
  case LANGUAGE_URD: return "urd";
  case LANGUAGE_UZB: return "uzb";
  case LANGUAGE_VAI: return "vai";
  case LANGUAGE_VEN: return "ven";
  case LANGUAGE_VIE: return "vie";
  case LANGUAGE_VLS: return "vls";
  case LANGUAGE_VOL: return "vol";
  case LANGUAGE_VOT: return "vot";
  case LANGUAGE_WAK: return "wak";
  case LANGUAGE_WAL: return "wal";
  case LANGUAGE_WAR: return "war";
  case LANGUAGE_WAS: return "was";
  case LANGUAGE_CYM: return "cym";
  case LANGUAGE_WEN: return "wen";
  case LANGUAGE_WLN: return "wln";
  case LANGUAGE_WOL: return "wol";
  case LANGUAGE_XAL: return "xal";
  case LANGUAGE_XHO: return "xho";
  case LANGUAGE_YAO: return "yao";
  case LANGUAGE_YAP: return "yap";
  case LANGUAGE_YID: return "yid";
  case LANGUAGE_YOR: return "yor";
  case LANGUAGE_YPK: return "ypk";
  case LANGUAGE_ZAP: return "zap";
  case LANGUAGE_ZBL: return "zbl";
  case LANGUAGE_ZEN: return "zen";
  case LANGUAGE_ZHA: return "zha";
  case LANGUAGE_ZND: return "znd";
  case LANGUAGE_ZUL: return "zul";
  case LANGUAGE_ZUN: return "zun";
  case LANGUAGE_ZXX: return "zxx";
  case LANGUAGE_ZZA: return "zza";
  default:
    return "";
  }
}
