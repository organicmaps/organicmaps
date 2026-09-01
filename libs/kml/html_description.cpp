#include "kml/html_description.hpp"

#include "coding/url.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace kml
{
namespace
{
size_t constexpr kHtmlDescNpos = std::string_view::npos;

// Single forward pass html tokenizer, see html_description.hpp for the scanned resources.
// The position never moves backwards and every loop iteration advances it, hence the linear time.
class HtmlDescriptionScanner
{
public:
  explicit HtmlDescriptionScanner(std::string_view html) : m_html(html) {}

  HtmlDescriptionScan Scan()
  {
    while (m_pos < m_html.size())
    {
      size_t const lt = m_html.find('<', m_pos);
      if (lt == kHtmlDescNpos)
        break;
      m_pos = lt + 1;
      ScanTag();
      if (m_bestPriority == kPoster)
        break;  // Neither output can change after the first strictly parsed poster.
    }
    return std::move(m_result);
  }

private:
  // Resource importance, the smallest one wins; ties are resolved by the document order.
  enum Priority : uint8_t
  {
    kPoster = 1,
    kImg = 2,
    kSvgImage = 3,
    kNetworkOnly = 4
  };

  static bool IsSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }

  static bool IsCssNameChar(char c)
  {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || strings::IsASCIIDigit(c) || c == '-' || c == '_' ||
           c == '\\' || static_cast<unsigned char>(c) >= 0x80;
  }

  enum class Tag : uint8_t
  {
    Other,
    Ignored,
    Img,
    Source,
    Image,
    Video,
    Script,
    Style,
    Svg,
    Math,
    ForeignObject,
    Title,
    Desc,
    MathText,
    AnnotationXml,
    Font,
    Template,
    Textarea,
    RawText,
    Plaintext
  };

  enum class Namespace : uint8_t
  {
    Html,
    Svg,
    Math
  };

  struct Attributes
  {
    std::string_view m_src;
    bool m_selfClosing = false;
    bool m_htmlIntegration = false;
  };

  static Tag ToTag(std::string_view name)
  {
    static constexpr std::pair<std::string_view, Tag> kTags[] = {
        {"img", Tag::Img},
        {"source", Tag::Source},
        {"image", Tag::Image},
        {"video", Tag::Video},
        {"script", Tag::Script},
        {"style", Tag::Style},
        {"svg", Tag::Svg},
        {"math", Tag::Math},
        {"foreignObject", Tag::ForeignObject},
        {"title", Tag::Title},
        {"desc", Tag::Desc},
        {"mi", Tag::MathText},
        {"mo", Tag::MathText},
        {"mn", Tag::MathText},
        {"ms", Tag::MathText},
        {"mtext", Tag::MathText},
        {"annotation-xml", Tag::AnnotationXml},
        {"font", Tag::Font},
        {"template", Tag::Template},
        {"textarea", Tag::Textarea},
        {"xmp", Tag::RawText},
        {"iframe", Tag::RawText},
        {"noembed", Tag::RawText},
        {"noframes", Tag::RawText},
        {"plaintext", Tag::Plaintext},
    };
    for (auto const & [tagName, tag] : kTags)
      if (strings::EqualAsciiNoCase(name, tagName))
        return tag;
    return Tag::Other;
  }

  Namespace CurrentNamespace() const
  {
    if (!m_contexts.empty())
    {
      if (m_contexts.back() == Tag::Svg)
        return Namespace::Svg;
      if (m_contexts.back() == Tag::Math)
        return Namespace::Math;
    }
    return Namespace::Html;
  }

  static bool LeavesForeignContent(std::string_view name)
  {
    static constexpr std::string_view kNames[] = {
        "b",     "big",   "blockquote", "body",   "br",   "center", "code",  "dd", "div",  "dl",   "dt",
        "em",    "embed", "h1",         "h2",     "h3",   "h4",     "h5",    "h6", "head", "hr",   "i",
        "img",   "li",    "listing",    "menu",   "meta", "nobr",   "ol",    "p",  "pre",  "ruby", "s",
        "small", "span",  "strong",     "strike", "sub",  "sup",    "table", "tt", "u",    "ul",   "var"};
    return std::ranges::any_of(kNames, [name](auto n) { return strings::EqualAsciiNoCase(name, n); });
  }

  // Position of the first non-whitespace character at or after pos.
  static size_t SkipSpaces(std::string_view s, size_t pos)
  {
    while (pos < s.size() && IsSpace(s[pos]))
      ++pos;
    return pos;
  }

  // A tag or an attribute name ends here.
  static bool IsNameEnd(char c) { return IsSpace(c) || c == '/' || c == '>'; }

  // An unquoted css url() value ends here: at the closing ')' or at a character that a css url
  // token cannot contain at all.
  static bool IsUrlEnd(char c) { return IsSpace(c) || c == ')' || c == '(' || c == '"' || c == '\''; }

  // Scans the tag that starts at m_pos, right after its '<'.
  void ScanTag()
  {
    if (m_pos >= m_html.size())
      return;

    if (m_html.substr(m_pos, 3) == "!--")
    {
      // Include the opening dashes so the short comments "<!-->" and "<!--->" also close.
      for (size_t end = m_pos + 1; (end = m_html.find('>', end)) != kHtmlDescNpos; ++end)
      {
        if (m_html.substr(end - 2, 2) == "--" || (end >= m_pos + 6 && m_html.substr(end - 3, 3) == "--!"))
        {
          m_pos = end + 1;
          return;
        }
      }
      m_pos = m_html.size();
      return;
    }

    char c = m_html[m_pos];
    if (c == '!' || c == '?')  // Declaration or processing instruction.
      return SkipPast('>');
    bool const endTag = c == '/';
    if (endTag)
    {
      if (++m_pos == m_html.size())
        return;
      c = m_html[m_pos];
    }
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    {
      if (endTag)
        SkipPast('>');
      return;  // A lone '<' is text; a malformed end tag is a bogus comment.
    }

    size_t const nameStart = m_pos;
    while (m_pos < m_html.size() && !IsNameEnd(m_html[m_pos]))
      ++m_pos;
    auto const name = m_html.substr(nameStart, m_pos - nameStart);
    Tag tag = ToTag(name);

    // HTML breakouts leave the foreign namespace even without an explicit </svg> or </math>.
    if (CurrentNamespace() != Namespace::Html &&
        (endTag ? strings::EqualAsciiNoCase(name, "p") || strings::EqualAsciiNoCase(name, "br")
                : LeavesForeignContent(name)))
      while (CurrentNamespace() != Namespace::Html)
        m_contexts.pop_back();
    auto const ns = CurrentNamespace();

    if (endTag)
    {
      ScanAttributes(Tag::Ignored);  // End-tag attributes consume quotes but cannot load resources.
      if (!m_contexts.empty() && m_contexts.back() == tag)
        m_contexts.pop_back();
      if (ns == Namespace::Html && tag == Tag::Template && m_templateDepth != 0)
        --m_templateDepth;
      return;
    }

    if (tag == Tag::Image && ns == Namespace::Html)
      tag = Tag::Img;  // The HTML <image> alias uses src/srcset, not SVG href attributes.

    auto const attrs = ScanAttributes(tag);
    if (m_templateDepth == 0 &&
        ((ns == Namespace::Html && tag == Tag::Img) || (ns == Namespace::Svg && tag == Tag::Image)))
      OnCandidate(attrs.m_src, tag == Tag::Img ? kImg : kSvgImage);
    if (tag == Tag::Font && attrs.m_htmlIntegration)
      while (CurrentNamespace() != Namespace::Html)
        m_contexts.pop_back();

    if (ns == Namespace::Html && tag == Tag::Template)
      ++m_templateDepth;

    if (!attrs.m_selfClosing &&
        (tag == Tag::Svg || tag == Tag::Math ||
         (ns == Namespace::Svg && (tag == Tag::ForeignObject || tag == Tag::Title || tag == Tag::Desc)) ||
         (ns == Namespace::Math && (tag == Tag::MathText || (tag == Tag::AnnotationXml && attrs.m_htmlIntegration)))))
      m_contexts.push_back(tag);

    if (ns != Namespace::Html && attrs.m_selfClosing)
      return;
    if (ns == Namespace::Html && tag == Tag::Plaintext)
    {
      m_pos = m_html.size();
      return;
    }

    if (tag == Tag::Script || tag == Tag::Style ||
        (ns == Namespace::Html && (tag == Tag::Title || tag == Tag::Textarea || tag == Tag::RawText)))
    {
      size_t const end = tag == Tag::Script && ns == Namespace::Html ? FindScriptEnd() : FindEndTag(name);
      if (tag == Tag::Style && m_templateDepth == 0)
        ScanCss(m_html.substr(m_pos, end - m_pos));
      m_pos = end;
    }
  }

  static uint8_t AttributeBit(std::string_view name)
  {
    static constexpr std::string_view kNames[] = {"src", "srcset", "href", "xlink:href", "poster", "style", "encoding"};
    uint8_t bit = 1;
    for (auto const attrName : kNames)
    {
      if (strings::EqualAsciiNoCase(name, attrName))
        return bit;
      bit <<= 1;
    }
    return 0;
  }

  Attributes ScanAttributes(Tag tag)
  {
    uint8_t seen = 0;
    bool hasHref = false;
    Attributes attrs;
    while (m_pos < m_html.size())
    {
      m_pos = SkipSpaces(m_html, m_pos);
      if (m_pos >= m_html.size())
        return attrs;

      char const c = m_html[m_pos];
      if (c == '>')
      {
        ++m_pos;
        return attrs;
      }
      if (c == '/' && m_html.substr(m_pos, 2) == "/>")
      {
        m_pos += 2;
        attrs.m_selfClosing = true;
        return attrs;
      }
      if (c == '/' || c == '=')  // A stray '/' or '=' does not start an attribute name.
      {
        ++m_pos;
        continue;
      }

      size_t const nameStart = m_pos;
      while (m_pos < m_html.size() && !IsNameEnd(m_html[m_pos]) && m_html[m_pos] != '=')
        ++m_pos;
      std::string_view const name = m_html.substr(nameStart, m_pos - nameStart);

      std::string_view value;
      m_pos = SkipSpaces(m_html, m_pos);
      if (m_pos < m_html.size() && m_html[m_pos] == '=')
      {
        ++m_pos;
        m_pos = SkipSpaces(m_html, m_pos);
        value = ReadValue();
      }

      if (tag == Tag::Ignored)
        continue;
      uint8_t const bit = AttributeBit(name);
      if ((seen & bit) != 0)
        continue;
      seen |= bit;
      if (tag == Tag::AnnotationXml && strings::EqualAsciiNoCase(name, "encoding"))
      {
        attrs.m_htmlIntegration =
            strings::EqualAsciiNoCase(value, "text/html") || strings::EqualAsciiNoCase(value, "application/xhtml+xml");
        continue;
      }
      if (tag == Tag::Font && (strings::EqualAsciiNoCase(name, "color") || strings::EqualAsciiNoCase(name, "face") ||
                               strings::EqualAsciiNoCase(name, "size")))
        attrs.m_htmlIntegration = true;
      strings::Trim(value);
      // Defer fallbacks so srcset and SVG href take precedence regardless of attribute order.
      if (tag == Tag::Img && strings::EqualAsciiNoCase(name, "src"))
        attrs.m_src = value;
      else if (tag == Tag::Image && strings::EqualAsciiNoCase(name, "href"))
      {
        attrs.m_src = value;
        hasHref = true;  // Even an empty href suppresses xlink:href.
      }
      else if (tag == Tag::Image && strings::EqualAsciiNoCase(name, "xlink:href"))
      {
        if (!hasHref)
          attrs.m_src = value;
      }
      else
        OnAttribute(tag, name, value);
    }
    return attrs;
  }

  std::string_view ReadValue()
  {
    if (m_pos >= m_html.size())
      return {};

    std::string_view value;
    char const quote = m_html[m_pos];
    if (quote == '"' || quote == '\'')
    {
      ++m_pos;
      // An unterminated quote swallows the rest of the input, exactly like in a browser.
      size_t const end = m_html.find(quote, m_pos);
      value = m_html.substr(m_pos, (end == kHtmlDescNpos ? m_html.size() : end) - m_pos);
      m_pos = (end == kHtmlDescNpos) ? m_html.size() : end + 1;
    }
    else
    {
      size_t const start = m_pos;
      while (m_pos < m_html.size() && !IsSpace(m_html[m_pos]) && m_html[m_pos] != '>')
        ++m_pos;
      value = m_html.substr(start, m_pos - start);
    }

    return value;
  }

  void OnAttribute(Tag tag, std::string_view name, std::string_view value)
  {
    if (m_templateDepth != 0 || value.empty())
      return;

    using strings::EqualAsciiNoCase;
    if (EqualAsciiNoCase(name, "style"))
      return ScanCss(value);
    if (CurrentNamespace() != Namespace::Html)
      return;

    switch (tag)
    {
    case Tag::Img:
      if (EqualAsciiNoCase(name, "srcset"))
        ScanSrcset(value);
      break;
    case Tag::Source:
      if (EqualAsciiNoCase(name, "srcset"))
        ScanSrcset(value, kNetworkOnly);
      break;
    case Tag::Video:
      if (EqualAsciiNoCase(name, "poster"))
        OnCandidate(value, kPoster);
      break;
    default: break;
    }
  }

  // "url1 2x, url2 640w, url3": a candidate url is the first non-whitespace run of every entry.
  void ScanSrcset(std::string_view srcset, Priority priority = kImg)
  {
    size_t pos = 0;
    while (pos < srcset.size())
    {
      while (pos < srcset.size() && (IsSpace(srcset[pos]) || srcset[pos] == ','))
        ++pos;

      size_t const start = pos;
      while (pos < srcset.size() && !IsSpace(srcset[pos]))
        ++pos;

      std::string_view url = srcset.substr(start, pos - start);
      if (!url.ends_with(','))
      {
        // Skip the descriptors ("2x", "640w"): they are not urls and never start with a scheme.
        size_t const comma = srcset.find(',', pos);
        pos = (comma == kHtmlDescNpos) ? srcset.size() : comma + 1;
      }
      while (url.ends_with(','))  // A sticking comma ends the entry, the next one starts right here.
        url.remove_suffix(1);

      OnCandidate(url, priority);
    }
  }

  void ScanCss(std::string_view css)
  {
    size_t pos = 0;
    while (pos < css.size())
    {
      if (css.substr(pos, 2) == "/*")
      {
        size_t const end = css.find("*/", pos + 2);
        pos = end == kHtmlDescNpos ? css.size() : end + 2;
        continue;
      }
      if (css[pos] == '\'' || css[pos] == '"')
      {
        char const quote = css[pos++];
        while (pos < css.size())
          if (css[pos] == '\\')
            pos += std::min<size_t>(2, css.size() - pos);
          else if (css[pos] == quote || css[pos] == '\n' || css[pos] == '\r' || css[pos] == '\f')
          {
            ++pos;
            break;
          }
          else
            ++pos;
        continue;
      }
      size_t const paren = pos++;
      if (css[paren] != '(')
        continue;

      // A CSS function name must be the whole identifier and touch its '('.
      if (paren < 3 || !strings::EqualAsciiNoCase(css.substr(paren - 3, 3), "url") ||
          (paren > 3 && IsCssNameChar(css[paren - 4])))
        continue;

      pos = SkipSpaces(css, pos);
      std::string_view const quote = OpeningQuote(css.substr(pos));
      pos += quote.size();

      size_t end;
      if (quote.empty())
      {
        end = pos;
        while (end < css.size() && !IsUrlEnd(css[end]))
          ++end;
      }
      else
      {
        end = css.find(quote, pos);
        if (end == kHtmlDescNpos)
          end = css.size();
      }

      std::string_view url = css.substr(pos, end - pos);
      strings::Trim(url);  // Url parsing strips the whitespace around a quoted value.

      // Reject a malformed function suffix; CSS also permits an implicit close at end of input.
      pos = SkipSpaces(css, std::min(end + quote.size(), css.size()));
      if (pos < css.size() && css[pos] != ')')
        continue;

      OnCandidate(url, kNetworkOnly);
    }
  }

  // The quote that opens a css url() value, plain or entity encoded, empty when there is none.
  // Other entities are outside this lexer's supported subset.
  static std::string_view OpeningQuote(std::string_view css)
  {
    static constexpr std::string_view kQuotes[] = {"\"", "'", "&quot;", "&apos;", "&#34;", "&#39;", "&#x22;", "&#x27;"};
    for (auto const quote : kQuotes)
      if (strings::EqualAsciiNoCase(css.substr(0, quote.size()), quote))
        return quote;
    return {};
  }

  void OnCandidate(std::string_view url, Priority priority)
  {
    // Deliberately generous: a browser may fetch urls that the strict origin parser rejects, so
    // the permission prompt must not depend on it.
    if (!strings::EqualAsciiNoCase(url.substr(0, 6), "https:") && !url.starts_with("//"))
      return;
    m_result.m_requiresNetwork = true;
    if (priority >= m_bestPriority)  // The first resource of the best priority wins.
      return;

    // Both accepted prefixes parse as an https origin, if they parse at all.
    auto origin = url::ParseHttpOrigin(url, true /* allowProtocolRelative */);
    if (origin.empty())
      return;

    m_result.m_baseUrl = std::move(origin);
    m_bestPriority = priority;
  }

  void SkipPast(char c)
  {
    size_t const pos = m_html.find(c, m_pos);
    m_pos = (pos == kHtmlDescNpos) ? m_html.size() : pos + 1;
  }

  static bool HasTagName(std::string_view text, std::string_view name)
  {
    return text.size() > name.size() && strings::EqualAsciiNoCase(text.substr(0, name.size()), name) &&
           IsNameEnd(text[name.size()]);
  }

  // HTML script comments can double-escape <script>, making the next </script> text too.
  size_t FindScriptEnd() const
  {
    enum class State
    {
      Normal,
      Escaped,
      DoubleEscaped
    };
    State state = State::Normal;
    for (size_t pos = m_pos; pos < m_html.size(); ++pos)
    {
      auto const rest = m_html.substr(pos);
      if (state != State::Normal && rest.starts_with("-->"))
        state = State::Normal;
      else if (state == State::Normal && rest.starts_with("<!--"))
      {
        state = State::Escaped;
        pos += 3;
      }
      else if (rest.starts_with("</") && HasTagName(rest.substr(2), "script"))
      {
        if (state != State::DoubleEscaped)
          return pos;
        state = State::Escaped;
        pos += 7;
      }
      else if (state == State::Escaped && rest.starts_with('<') && HasTagName(rest.substr(1), "script"))
      {
        state = State::DoubleEscaped;
        pos += 6;
      }
    }
    return m_html.size();
  }

  // Position of the "</name" that ends a raw text element, or the end of the input.
  size_t FindEndTag(std::string_view name) const
  {
    for (size_t pos = m_pos; (pos = m_html.find('<', pos)) != kHtmlDescNpos; ++pos)
    {
      std::string_view const rest = m_html.substr(pos + 1);
      if (rest.starts_with('/') && HasTagName(rest.substr(1), name))
        return pos;
    }
    return m_html.size();
  }

  std::string_view m_html;
  size_t m_pos = 0;
  // Only foreign namespace roots and HTML integration points need stack entries, not ordinary tags.
  std::vector<Tag> m_contexts;
  size_t m_templateDepth = 0;
  Priority m_bestPriority = kNetworkOnly;
  HtmlDescriptionScan m_result;
};
}  // namespace

std::string DebugPrint(HtmlDescriptionScan const & scan)
{
  return "HtmlDescriptionScan [ baseUrl: " + (scan.m_baseUrl.empty() ? "<none>" : scan.m_baseUrl) +
         ", requiresNetwork: " + (scan.m_requiresNetwork ? "true" : "false") + " ]";
}

HtmlDescriptionScan ScanHtmlDescription(std::string_view html)
{
  return HtmlDescriptionScanner(html).Scan();
}
}  // namespace kml
