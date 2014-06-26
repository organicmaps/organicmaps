#include "BinPacker.hpp"
#include <cassert>
#include <algorithm>

// ---------------------------------------------------------------------------
void BinPacker::Pack(
    const std::vector<int>&          rects,
    std::vector< std::vector<int> >& packs,
    int                              packSize,
    bool                             allowRotation)
{
    assert(!(rects.size() % 2));

    Clear();

    m_packSize = packSize;

    // Add rects to member array, and check to make sure none is too big
    for (size_t i = 0; i < rects.size(); i += 2) {
        if (rects[i] > m_packSize || rects[i + 1] > m_packSize) {
            assert(!"All rect dimensions must be <= the pack size");
        }
        m_rects.push_back(Rect(0, 0, rects[i], rects[i + 1], i >> 1));
    }

    // Sort from greatest to least area
    std::sort(m_rects.rbegin(), m_rects.rend());

    // Pack
    while (m_numPacked < (int)m_rects.size()) {
        int i = m_packs.size();
        m_packs.push_back(Rect(m_packSize));
        m_roots.push_back(i);
        Fill(i, allowRotation);
    }

    // Write out
    packs.resize(m_roots.size());
    for (size_t i = 0; i < m_roots.size(); ++i) {
        packs[i].clear();
        AddPackToArray(m_roots[i], packs[i]);
    }

    // Check and make sure all rects were packed
    for (size_t i = 0; i < m_rects.size(); ++i) {
        if (!m_rects[i].packed) {
            assert(!"Not all rects were packed");
        }
    }
}
// ---------------------------------------------------------------------------
void BinPacker::Clear()
{
    m_packSize = 0;
    m_numPacked = 0;
    m_rects.clear();
    m_packs.clear();
    m_roots.clear();
}
// ---------------------------------------------------------------------------
void BinPacker::Fill(int pack, bool allowRotation)
{
    assert(PackIsValid(pack));

    int i = pack;

    // For each rect
    for (size_t j = 0; j < m_rects.size(); ++j) {
        // If it's not already packed
        if (!m_rects[j].packed) {
            // If it fits in the current working area
            if (Fits(m_rects[j], m_packs[i], allowRotation)) {
                // Store in lower-left of working area, split, and recurse
                ++m_numPacked;
                Split(i, j);
                Fill(m_packs[i].children[0], allowRotation);
                Fill(m_packs[i].children[1], allowRotation);
                return;
            }
        }
    }
}
// ---------------------------------------------------------------------------
void BinPacker::Split(int pack, int rect)
{
    assert(PackIsValid(pack));
    assert(RectIsValid(rect));

    int i = pack;
    int j = rect;

    // Split the working area either horizontally or vertically with respect
    // to the rect we're storing, such that we get the largest possible child
    // area.

    Rect left = m_packs[i];
    Rect right = m_packs[i];
    Rect bottom = m_packs[i];
    Rect top = m_packs[i];

    left.y += m_rects[j].h;
    left.w = m_rects[j].w;
    left.h -= m_rects[j].h;
    right.x += m_rects[j].w;
    right.w -= m_rects[j].w;

    bottom.x += m_rects[j].w;
    bottom.h = m_rects[j].h;
    bottom.w -= m_rects[j].w;
    top.y += m_rects[j].h;
    top.h -= m_rects[j].h;

    int maxLeftRightArea = left.GetArea();
    if (right.GetArea() > maxLeftRightArea) {
        maxLeftRightArea = right.GetArea();
    }

    int maxBottomTopArea = bottom.GetArea();
    if (top.GetArea() > maxBottomTopArea) {
        maxBottomTopArea = top.GetArea();
    }

    if (maxLeftRightArea > maxBottomTopArea) {
        if (left.GetArea() > right.GetArea()) {
            m_packs.push_back(left);
            m_packs.push_back(right);
        } else {
            m_packs.push_back(right);
            m_packs.push_back(left);
        }
    } else {
        if (bottom.GetArea() > top.GetArea()) {
            m_packs.push_back(bottom);
            m_packs.push_back(top);
        } else {
            m_packs.push_back(top);
            m_packs.push_back(bottom);
        }
    }

    // This pack area now represents the rect we've just stored, so save the
    // relevant info to it, and assign children.
    m_packs[i].w = m_rects[j].w;
    m_packs[i].h = m_rects[j].h;
    m_packs[i].ID = m_rects[j].ID;
    m_packs[i].rotated = m_rects[j].rotated;
    m_packs[i].children[0] = m_packs.size() - 2;
    m_packs[i].children[1] = m_packs.size() - 1;

    // Done with the rect
    m_rects[j].packed = true;
}
// ---------------------------------------------------------------------------
bool BinPacker::Fits(Rect& rect1, const Rect& rect2, bool allowRotation)
{
    // Check to see if rect1 fits in rect2, and rotate rect1 if that will
    // enable it to fit.

    if (rect1.w <= rect2.w && rect1.h <= rect2.h) {
        return true;
    } else if (allowRotation && rect1.h <= rect2.w && rect1.w <= rect2.h) {
        rect1.Rotate();
        return true;
    } else {
        return false;
    }
}
// ---------------------------------------------------------------------------
void BinPacker::AddPackToArray(int pack, std::vector<int>& array) const
{
    assert(PackIsValid(pack));

    int i = pack;
    if (m_packs[i].ID != -1) {
        array.push_back(m_packs[i].ID);
        array.push_back(m_packs[i].x);
        array.push_back(m_packs[i].y);
        array.push_back(m_packs[i].rotated);

        if (m_packs[i].children[0] != -1) {
            AddPackToArray(m_packs[i].children[0], array);
        }
        if (m_packs[i].children[1] != -1) {
            AddPackToArray(m_packs[i].children[1], array);
        }
    }
}
// ---------------------------------------------------------------------------
bool BinPacker::RectIsValid(int i) const
{
    return i >= 0 && i < (int)m_rects.size();
}
// ---------------------------------------------------------------------------
bool BinPacker::PackIsValid(int i) const
{
    return i >= 0 && i < (int)m_packs.size();
}
// ---------------------------------------------------------------------------
