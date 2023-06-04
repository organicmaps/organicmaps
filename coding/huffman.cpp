#include "coding/huffman.hpp"

#include "base/logging.hpp"

#include <queue>
#include <utility>

namespace coding
{
HuffmanCoder::~HuffmanCoder()
{
  DeleteHuffmanTree(m_root);
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

void HuffmanCoder::Clear()
{
  DeleteHuffmanTree(m_root);
  m_root = nullptr;
  m_encoderTable.clear();
  m_decoderTable.clear();
}

void HuffmanCoder::DeleteHuffmanTree(Node * root)
{
  if (!root)
    return;
  DeleteHuffmanTree(root->l);
  DeleteHuffmanTree(root->r);
  delete root;
}

void HuffmanCoder::BuildHuffmanTree(Freqs const & freqs)
{
  std::priority_queue<Node *, std::vector<Node *>, NodeComparator> pq;
  for (auto const & e : freqs.GetTable())
    pq.push(new Node(e.first, e.second, true /* isLeaf */));

  if (pq.empty())
  {
    m_root = nullptr;
    return;
  }

  while (pq.size() > 1)
  {
    auto a = pq.top();
    pq.pop();
    auto b = pq.top();
    pq.pop();
    if (a->symbol > b->symbol)
      std::swap(a, b);
    // Give it the smaller symbol to make the resulting encoding more predictable.
    auto ab = new Node(a->symbol, a->freq + b->freq, false /* isLeaf */);
    ab->l = a;
    ab->r = b;
    pq.push(ab);
  }

  m_root = pq.top();
  pq.pop();

  SetDepths(m_root, 0 /* depth */);
}

void HuffmanCoder::SetDepths(Node * root, uint32_t depth)
{
  // One would need more than 2^32 symbols to build a code that long.
  // On the other hand, 32 is short enough for our purposes, so do not
  // try to shrink the trees beyond this threshold.
  uint32_t const kMaxDepth = 32;

  if (!root)
    return;
  CHECK_LESS_OR_EQUAL(depth, kMaxDepth, ());
  root->depth = depth;
  SetDepths(root->l, depth + 1);
  SetDepths(root->r, depth + 1);
}
}  //  namespace coding
