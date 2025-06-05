#include "indexer/search_delimiters.hpp"

namespace search
{
// Delimiters --------------------------------------------------------------------------------------
bool Delimiters::operator()(strings::UniChar c) const
{
  // ascii ranges first
  if (c < '0')
    return true;
  if (c > '9' && c < 'A')
    return true;
  if (c > 'Z' && c < 'a')
    return true;
  if (c > 'z' && c < 0xC0)
    return true;

  // values are calculated by osm statistics on 26/05/11
  switch (c)
  {
  case 0x2116:  // NUMERO SIGN
  //  case 0x00B0:  // DEGREE SIGN
  case 0x2013:  // EN DASH
  case 0x2019:  // RIGHT SINGLE QUOTATION MARK
  case 0x00AB:  // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
  case 0x00BB:  // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
  case 0x3000:  // IDEOGRAPHIC SPACE
  case 0x30FB:  // KATAKANA MIDDLE DOT
  //  case 0x00B4:  // ACUTE ACCENT
  case 0x200E:  // LEFT-TO-RIGHT MARK
  case 0xFF08:  // FULLWIDTH LEFT PARENTHESIS
  //  case 0x00A0:  // NO-BREAK SPACE
  case 0xFF09:  // FULLWIDTH RIGHT PARENTHESIS
  case 0x2018:  // LEFT SINGLE QUOTATION MARK
  //  case 0x007E:  // TILDE
  case 0x2014:  // EM DASH
  //  case 0x007C:  // VERTICAL LINE
  case 0x0F0B:  // TIBETAN MARK INTERSYLLABIC TSHEG
  case 0x201C:  // LEFT DOUBLE QUOTATION MARK
  case 0x201E:  // DOUBLE LOW-9 QUOTATION MARK
  //  case 0x00AE:  // REGISTERED SIGN
  case 0xFFFD:  // REPLACEMENT CHARACTER
  case 0x200C:  // ZERO WIDTH NON-JOINER
  case 0x201D:  // RIGHT DOUBLE QUOTATION MARK
  case 0x3001:  // IDEOGRAPHIC COMMA
  case 0x300C:  // LEFT CORNER BRACKET
  case 0x300D:  // RIGHT CORNER BRACKET
  //  case 0x00B7:  // MIDDLE DOT
  case 0x061F:  // ARABIC QUESTION MARK
  case 0x2192:  // RIGHTWARDS ARROW
  case 0x2212:  // MINUS SIGN
  //  case 0x0091:  // <control>
  //  case 0x007D:  // RIGHT CURLY BRACKET
  //  case 0x007B:  // LEFT CURLY BRACKET
  //  case 0x00A9:  // COPYRIGHT SIGN
  case 0x200D:  // ZERO WIDTH JOINER
  case 0x200B:  // ZERO WIDTH SPACE
    return true;
  }
  return false;
}

// DelimitersWithExceptions ------------------------------------------------------------------------
DelimitersWithExceptions::DelimitersWithExceptions(std::vector<strings::UniChar> const & exceptions)
  : m_exceptions(exceptions)
{}

bool DelimitersWithExceptions::operator()(strings::UniChar c) const
{
  if (find(m_exceptions.begin(), m_exceptions.end(), c) != m_exceptions.end())
    return false;
  return m_delimiters(c);
}
}  // namespace search
