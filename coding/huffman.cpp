#include "coding/huffman.hpp"

#include "base/logging.hpp"

namespace coding
{
HuffmanCoder::~HuffmanCoder()
{
  DeleteHuffmanTree(m_root);
}

void HuffmanCoder::Init(vector<strings::UniString> const & data)
{
  DeleteHuffmanTree(m_root);
  BuildHuffmanTree(data.begin(), data.end());
  BuildTables(m_root, 0);
}

bool HuffmanCoder::Encode(uint32_t symbol, Code & code) const
{
  auto it = m_encoderTable.find(symbol);
  if (it == m_encoderTable.end())
    return false;
  code = it->second;
  return true;
}

bool HuffmanCoder::Decode(Code const & code, uint32_t & symbol) const
{
  auto it = m_decoderTable.find(code);
  if (it == m_decoderTable.end())
    return false;
  symbol = it->second;
  return true;
}

void HuffmanCoder::BuildTables(Node * root, uint32_t path)
{
  if (!root)
    return;
  if (root->isLeaf)
  {
    Code code(path, root->depth);
    m_encoderTable[root->symbol] = code;
    m_decoderTable[code] = root->symbol;
    return;
  }
  BuildTables(root->l, path);
  BuildTables(root->r, path + (static_cast<uint32_t>(1) << root->depth));
}

void HuffmanCoder::DeleteHuffmanTree(Node * root)
{
  if (!root)
    return;
  DeleteHuffmanTree(root->l);
  DeleteHuffmanTree(root->r);
  delete root;
}

void HuffmanCoder::SetDepths(Node * root, uint32_t depth)
{
  if (!root)
    return;
  root->depth = depth;
  SetDepths(root->l, depth + 1);
  SetDepths(root->r, depth + 1);
}

}  //  namespace coding
