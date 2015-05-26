//
//  text_engine.h
//  FreetypeLabels
//
//  Created by Sergey Yershov on 09.03.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#pragma once
#ifndef __ML__TEXT_ENGINE_H__
#define __ML__TEXT_ENGINE_H__

#include <string>
#include <stdexcept>
#include <map>
#include <vector>
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

//#include "base/utility.h"
#include "point.h"
#include "rect.h"
//#include "images/style.h"

#include "base/string_utils.hpp"

namespace ml
{
class text_engine;

class face
{
  double m_height;
  double m_ascender;
  double m_descender;
  std::string m_face_name;
  size_t m_face_size;
  FT_Face m_face;
  bool m_flip_y;

  typedef std::map<FT_UInt, FT_Glyph> glyph_cache_type;

  ml::text_engine & m_engine;
  glyph_cache_type m_glyph_cache;

public:
  face();
  //		~face() {} // TODO: clear cache before exit

  bool flip_y() const { return m_flip_y; }
  void flip_y(bool f) { m_flip_y = f; }

  face(ml::text_engine & e, std::string const & name, size_t size);
  inline std::string const & face_name() const { return m_face_name; }
  inline size_t face_size() const { return m_face_size; }
  FT_Glyph glyph(unsigned int code, unsigned int prev_code = 0, ml::point_d * kerning = NULL);
  inline double ascender() const { return m_ascender; }
  inline double descender() const { return m_descender; }
  inline double height() const { return m_height; }
};

struct text_engine_exception : public std::runtime_error
{
  text_engine_exception(std::string const & s) : std::runtime_error(s) {}
};

class text_renderer
{
protected:
  bool m_outline;

public:
  text_renderer() {}
  virtual void operator()(ml::point_d const & pt, size_t width, size_t height,
                          unsigned char const * data)
  {
  }
  bool outline() const { return m_outline; }
  void outline(bool f) { m_outline = f; }
  virtual double outlinewidth() const { return 0; }
};

class text_options
{
  bool m_flip_y;
  ml::point_d m_offset;
  size_t m_horizontal_align;
  size_t m_vertical_align;
  float m_linegap;
  float m_ascender;

public:
  text_options(face const & f)
      : m_flip_y(f.flip_y()),
        m_offset(0, 0),
        m_horizontal_align(0),
        m_vertical_align(0),
        m_linegap(f.height()),
        m_ascender(f.ascender())
  {
  }

  bool flip_y() const { return m_flip_y; }
  bool flip_y(bool flip) { return (m_flip_y = flip); }

  ml::point_d const & offset() const { return m_offset; }
  ml::point_d const & offset(ml::point_d const & p) { return (m_offset = p); }

  size_t horizontal_align() const { return m_horizontal_align; }
  size_t horizontal_align(size_t v) { return m_horizontal_align = v; }

  size_t vertical_align() const { return m_vertical_align; }
  size_t vertical_align(size_t v) { return m_vertical_align = v; }

  float linegap() const { return m_linegap; }
  float linegap(float v) { return m_linegap = v; }

  float ascender() const { return m_ascender; }
  float ascender(float v) { return m_ascender = v; }
};

class text
{
public:
  class symbol_holder
  {
  public:
    ml::rect_d m_bounds;
    float advance;
    ml::point_d pt;
    float angle;
    unsigned int charcode;

    symbol_holder() : advance(0), angle(0.0), charcode(0) {}
    symbol_holder(unsigned int code) : advance(0), angle(0.0), charcode(code) {}
    ml::rect_d const & bounds() const { return m_bounds; }
  };

  typedef symbol_holder symbol_type;
  typedef std::vector<symbol_type> text_type;

protected:
  struct string_range
  {
    size_t begin;
    size_t end;
    string_range() : begin(0), end(0) {}
    string_range(size_t b, size_t e) : begin(b), end(e) {}
  };

  typedef std::vector<string_range> string_array_type;

  text_type m_text;
  string_array_type m_string_array;
  ml::rect_d m_bounds;

public:
  enum text_align_e
  {
    align_center = 0,
    align_left = 1,
    align_right = 4,
    align_top = 8,
    align_bottom = 16
  };

  text() {}
  text(strings::UniString const & src) { set_text(src); }

  void swap(text & t)
  {
    m_text.swap(t.m_text);
    m_string_array.swap(t.m_string_array);
    m_bounds = t.m_bounds;
  }

  void set_text(strings::UniString const & src);

  ml::rect_d const & calc_bounds(ml::face & face, double outlinewidth = 0);

  ml::rect_d const & bounds() const { return m_bounds; }

  bool is_intersect_with(ml::text const & t) const;
  bool is_intersect_with(ml::rect_d const & r) const;

  bool empty() const { return m_string_array.empty(); }

  void render(ml::face & face, text_renderer & renderer) const;
  void apply_font(ml::face & face, double symbol_space = 0.0);
  void warp(std::vector<ml::point_d> const & path, ml::text_options const & options);

  text_type const & symbols() const { return m_text; }

protected:
  bool is_whitespace(unsigned int ch) const;
  double string_width(text::string_range const & r) const;
  ml::rect_d const & calc_bounds(ml::face & face, symbol_holder & sym, double outlinewidth = 0);
  void render(ml::face & face, symbol_holder const & sym, text_renderer & renderer) const;
  void warp_text_line(string_range const & text_line, std::vector<ml::point_d> const & path,
                      ml::point_d & shift, bool flip);
};

class text_engine
{
  typedef std::map<std::string, FT_Face> face_map_type;
  typedef std::map<pair<size_t, size_t>, face> face_cache_type;
  FT_Library m_library;
  face_map_type m_faces;
  FT_Face m_current_face;
  face_cache_type m_cache;

  text_engine & select_face(std::string const & face_name);
  text_engine & set_size(size_t height, size_t width = 0);

public:
  ml::face & get_face(size_t font_key, std::string const & name, size_t size);
  FT_Face current_face() { return m_current_face; }

  text_engine & load_face(std::string const & face_name, std::string const & face_path);
  text_engine & load_face(std::string const & face_name, char const * face_data,
                          size_t face_data_size);
  text_engine();
  //        ~text_engine() {} // TODO: destroy all faces before exit
};

}  // namespace ml

#endif  // __ML__TEXT_ENGINE_H__
