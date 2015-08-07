#include "generator/xml_element.hpp"

#include "coding/parse_xml.hpp"
#include "base/string_utils.hpp"

#include "std/cstdio.hpp"
#include "std/algorithm.hpp"


void XMLElement::AddKV(string const & k, string const & v)
{
  childs.push_back(XMLElement());
  XMLElement & e = childs.back();

  e.tagKey = ET_TAG;
  e.k = k;
  e.v = v;
  e.parent = this;
}

void XMLElement::AddND(uint64_t ref)
{
  childs.push_back(XMLElement());
  XMLElement & e = childs.back();

  e.tagKey = ET_ND;
  e.ref = ref;
  e.parent = this;
}

void XMLElement::AddMEMBER(uint64_t ref, string const & type, string const & role)
{
  childs.push_back(XMLElement());
  XMLElement & e = childs.back();

  e.tagKey = ET_MEMBER;
  e.ref = ref;
  e.type = type;
  e.role = role;
  e.parent = this;
}

string XMLElement::ToString(string const & shift) const
{
  stringstream ss;
  ss << (shift.empty() ? "\n" : shift);
  switch (tagKey)
  {
    case ET_NODE:
      ss << "Node: " << id << " (" << fixed << setw(7) << lat << ", " << lng << ")";
      break;
    case ET_ND:
      ss << "Nd ref: " << ref;
      break;
    case ET_WAY:
      ss << "Way: " << id << " elements: " << childs.size();
      break;
    case ET_RELATION:
      ss << "Relation: " << id << " elements: " << childs.size();
      break;
    case ET_TAG:
      ss << "Tag: " << k << " = " << v;
      break;
    case ET_MEMBER:
      ss << "Member: " << ref << " type: " << type << " role: " << role;
      break;
    default:
      ss << "Unknown element";
  }
  if (!childs.empty())
  {
    string shift2 = shift;
    shift2 += shift2.empty() ? "\n  " : "  ";
    for ( auto const & e : childs )
      ss << e.ToString(shift2);
  }
  return ss.str();
}


string DebugPrint(XMLElement const & e)
{
  return e.ToString();
}


void BaseOSMParser::AddAttr(string const & key, string const & value)
{
  if (!m_current)
    return;

  if (key == "id")
    CHECK ( strings::to_uint64(value, m_current->id), ("Unknown element with invalid id : ", value) );
  else if (key == "lon")
    CHECK ( strings::to_double(value, m_current->lng), ("Bad node lon : ", value) );
  else if (key == "lat")
    CHECK ( strings::to_double(value, m_current->lat), ("Bad node lat : ", value) );
  else if (key == "ref")
    CHECK ( strings::to_uint64(value, m_current->ref), ("Bad node ref in way : ", value) );
  else if (key == "k")
    m_current->k = value;
  else if (key == "v")
    m_current->v = value;
  else if (key == "type")
    m_current->type = value;
  else if (key == "role")
    m_current->role = value;
}

bool BaseOSMParser::Push(string const & tagName)
{
  // As tagKey we use first two char of tag name.
  XMLElement::ETag tagKey = XMLElement::ETag(*reinterpret_cast<uint16_t const *>(tagName.data()));

  switch (tagKey)
  {
    // This tags will ignored in Push function.
    case XMLElement::ET_MEMBER:
    case XMLElement::ET_TAG:
    case XMLElement::ET_ND:
      return false;
    default: break;
  }

  switch (++m_depth)
  {
    case 1:
      m_current = nullptr;
      break;
    case 2:
      m_current = &m_parent;
      m_current->tagKey = tagKey;
      break;
    default:
      m_current = &m_child;
      m_current->tagKey = tagKey;
  }
  return true;
}

void BaseOSMParser::Pop(string const &)
{
  switch (--m_depth)
  {
    case 0:
      break;

    case 1:
      EmitElement(m_current);
      m_parent.Clear();
      break;

    default:
      switch (m_child.tagKey)
      {
        case XMLElement::ET_MEMBER:
          m_parent.AddMEMBER(m_child.ref, m_child.type, m_child.role);
          break;
        case XMLElement::ET_TAG:
          m_parent.AddKV(m_child.k, m_child.v);
          break;
        case XMLElement::ET_ND:
          m_parent.AddND(m_child.ref);
        default: break;
      }
      m_current = &m_parent;
      m_child.Clear();
  }
}
