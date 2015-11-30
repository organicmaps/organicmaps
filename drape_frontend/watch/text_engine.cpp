#include "text_engine.h"

#include <sstream>
#include <boost/format.hpp>

extern "C" const char default_font_data[741536];

namespace ml
{
void text::set_text(strings::UniString const & src)
{
  //		std::wstring const & text = ia::utility::from_utf8_to_ucs4(src);
  m_text.resize(src.size());
  size_t begin = 0;
  for (size_t i = 0; i < src.size(); ++i)
  {
    m_text[i].charcode = src[i];
    if (src[i] == '\n')
    {
      m_string_array.push_back(string_range(begin, i + 1));
      begin = i + 1;
    }
  }
  m_string_array.push_back(string_range(begin, src.size()));
}

bool text::is_whitespace(unsigned int ch) const
{
  switch (ch)
  {
    case '\n':
    case ' ':
      return true;
  }
  return false;
}

void text::render(ml::face & face, text_renderer & renderer) const
{
  for (text_type::const_iterator it = m_text.begin(); it != m_text.end(); ++it)
  {
    if (is_whitespace(it->charcode))
      continue;
    render(face, *it, renderer);
  }
}

ml::rect_d const & text::calc_bounds(ml::face & face, double outlinewidth)
{
  m_bounds = ml::rect_d::void_rect();
  for (text_type::iterator it = m_text.begin(); it != m_text.end(); ++it)
  {
    if (is_whitespace(it->charcode))
      continue;
    m_bounds.extend(calc_bounds(face, *it, outlinewidth));
  }
  return m_bounds;
}

bool text::is_intersect_with(ml::text const & t) const
{
  if (m_bounds.location(t.m_bounds) != ml::point_d::out)
  {
    for (text_type::const_iterator it = m_text.begin(); it != m_text.end(); ++it)
    {
      if (is_whitespace(it->charcode))
        continue;
      if (t.is_intersect_with(it->bounds()))
        return true;
    }
  }
  return false;
}

bool text::is_intersect_with(ml::rect_d const & r) const
{
  for (text_type::const_iterator it = m_text.begin(); it != m_text.end(); ++it)
  {
    if (is_whitespace(it->charcode))
      continue;
    if (r.location(it->bounds()) != ml::point_d::out)
      return true;
  }
  return false;
}

double text::string_width(text::string_range const & r) const
{
  double string_width = 0;
  for (size_t i = r.begin; i < r.end; ++i)
  {
    string_width += m_text[i].advance;
  }
  return string_width;
}

void text::warp(std::vector<ml::point_d> const & path, ml::text_options const & options)
{
  size_t line_num = 0;
  ml::point_d shift(options.offset());

  if (options.vertical_align() == align_top)
  {
    shift.y += options.ascender() - m_bounds.height();
  }
  else if (options.vertical_align() == align_center)
  {
    shift.y += options.ascender() - m_bounds.height() / 2;
  }
  else if (options.vertical_align() == align_bottom)
  {
    shift.y += options.ascender();
  }

  for (string_array_type::iterator line = m_string_array.begin(); line != m_string_array.end();
       ++line)
  {
    double line_width = string_width(*line);

    //Позиционирование относительно ломаной (Left,Center,Right)
    double line_shift_x = 0;
    if (options.horizontal_align() == align_left)
      line_shift_x = 0;
    else if (options.horizontal_align() == align_center)
      line_shift_x = (ml::length(path) - line_width) / 2;
    else if (options.horizontal_align() == align_right)
      line_shift_x = ml::length(path) - line_width;
    ml::point_d line_offset(shift.x + line_shift_x, shift.y + line_num * options.linegap());
    warp_text_line(*line, path, line_offset, options.flip_y());
    line_num++;
  }
}

void text::warp_text_line(text::string_range const & text_line,
                          std::vector<ml::point_d> const & path, ml::point_d & shift, bool flip)
{
  size_t segment_num = ml::segment(path, shift.x, &shift.x);

  double tt_angle = path.front().angle(path.back());

  ml::point_d seg_start = path[segment_num];
  ml::point_d seg_end = path[++segment_num];

  double angle = seg_start.angle(seg_end);
  ml::point_d cursor = seg_start.vector(shift.x, angle);

  for (size_t i = text_line.begin; i < text_line.end; ++i)
  {
    symbol_holder & symbol = m_text[i];
    symbol.pt = cursor;

    double symbol_width = symbol.advance;
    double distance_to_end = ml::distance(cursor, seg_end);
    angle = seg_start.angle(seg_end);
    if (distance_to_end < symbol_width)
    {
      do
      {
        if ((segment_num) > (path.size() - 2))
          break;
        seg_start = seg_end;
        seg_end = path[++segment_num];
        distance_to_end = ml::distance(cursor, seg_start);
        angle = cursor.angle(seg_start);
      } while (distance_to_end + ml::distance(seg_start, seg_end) < symbol_width);

      double next_segment_angle = seg_start.angle(seg_end);
      double val = (distance_to_end / symbol_width) * sin((3.1415 - angle) + next_segment_angle);
      angle = next_segment_angle + asin(val);
    }
    cursor = cursor.vector(symbol_width, angle);

    symbol.angle = angle;
    symbol.pt.x += shift.y * sin(tt_angle) * (flip ? 1 : -1);
    symbol.pt.y -= shift.y * cos(tt_angle) * (flip ? 1 : -1);
  }
}

void text::apply_font(ml::face & face, double symbol_space)
{
  // preload glyphs & calc basic dimensions
  double max_width = 0;
  for (string_array_type::iterator line = m_string_array.begin(); line != m_string_array.end();
       ++line)
  {
    double line_width = 0;
    for (size_t i = (*line).begin; i < (*line).end; ++i)
    {
      m_text[i].pt.x = line_width;
      m_text[i].pt.y = 0;
      line_width +=
          (m_text[i].advance =
               (((double)face.glyph(m_text[i].charcode)->advance.x) / 65535) + symbol_space);
    }
    if (max_width < line_width)
      max_width = line_width;
  }
  m_bounds.set(0, 0, max_width, m_string_array.size() * face.height());
}

ml::rect_d const & text::calc_bounds(ml::face & face, symbol_holder & sym, double outlinewidth)
{
  FT_Error error;
  FT_Glyph img;
  FT_Glyph_Copy(face.glyph(sym.charcode), &img);

  if (outlinewidth > 0)
  {
    FT_Stroker stroker;
    FT_Stroker_New(img->library, &stroker);
    FT_Stroker_Set(stroker, (FT_Fixed)(outlinewidth * 64), FT_STROKER_LINECAP_ROUND,
                   FT_STROKER_LINEJOIN_ROUND, 0);
    FT_Glyph_StrokeBorder(&img, stroker, 0, 1);
    FT_Stroker_Done(stroker);
  }

  FT_Vector pen = {(FT_Pos)floor(sym.pt.x * 64), (FT_Pos)floor(sym.pt.y * 64)};

  FT_Matrix matrix;
  matrix.xx = (FT_Fixed)(cos(sym.angle) * 0x10000L);
  matrix.xy = (FT_Fixed)((face.flip_y() ? -1 : 1) * sin(sym.angle) * 0x10000L);
  matrix.yx = (FT_Fixed)(sin(sym.angle) * 0x10000L);
  matrix.yy = (FT_Fixed)((face.flip_y() ? 1 : -1) * cos(sym.angle) * 0x10000L);
  if ((error = FT_Glyph_Transform(img, &matrix, &pen)) != 0)
  {
    std::stringstream ss;
    ss << "Transform failed, code: " << std::hex << error << std::dec << std::endl;
    ss << "\tGlyph: " << face.glyph(sym.charcode)
       << " format: " << (char *)(&face.glyph(sym.charcode)->format) << std::endl;
    ss << "\twith angle: " << sym.angle << " coord: " << sym.pt << " pen: " << pen.x << ", "
       << pen.y;
    throw ml::text_engine_exception(ss.str());
  }

  FT_BBox bbox;
  FT_Glyph_Get_CBox(img, FT_GLYPH_BBOX_PIXELS, &bbox);
  //        FT_Glyph_Get_CBox(img,FT_GLYPH_BBOX_SUBPIXELS,&bbox);

  sym.m_bounds.set(bbox.xMin, bbox.yMin, bbox.xMax, bbox.yMax);
  //		std::cout << sym.pt << std::endl;
  //		std::cerr << m_bounds << std::endl;
  FT_Done_Glyph(img);
  return sym.m_bounds;
}

void text::render(ml::face & face, symbol_holder const & sym, text_renderer & renderer) const
{
  FT_Error error;
  FT_Glyph img;
  FT_Glyph_Copy(face.glyph(sym.charcode), &img);

  if (renderer.outline())
  {
    FT_Stroker stroker;
    FT_Stroker_New(img->library, &stroker);
    FT_Stroker_Set(stroker, (FT_Fixed)(renderer.outlinewidth() * 64), FT_STROKER_LINECAP_ROUND,
                   FT_STROKER_LINEJOIN_ROUND, 0);
    FT_Glyph_StrokeBorder(&img, stroker, 0, 1);
    FT_Stroker_Done(stroker);
  }

  FT_Matrix matrix;
  FT_Vector pen = {(FT_Pos)floor(sym.pt.x * 64), (FT_Pos)floor(sym.pt.y * 64)};
  matrix.xx = (FT_Fixed)(cos(sym.angle) * 0x10000L);
  matrix.xy = (FT_Fixed)((face.flip_y() ? -1 : 1) * sin(sym.angle) * 0x10000L);
  matrix.yx = (FT_Fixed)(sin(sym.angle) * 0x10000L);
  matrix.yy = (FT_Fixed)((face.flip_y() ? 1 : -1) * cos(sym.angle) * 0x10000L);
  if ((error = FT_Glyph_Transform(img, &matrix, &pen)) != 0)
  {
    std::stringstream ss;
    ss << "Transform failed, code: " << std::hex << error << std::dec << std::endl;
    ss << "\tGlyph: " << face.glyph(sym.charcode)
       << " format: " << (char *)(&face.glyph(sym.charcode)->format) << std::endl;
    ss << "\twith angle: " << sym.angle << " coord: " << sym.pt << " pen: " << pen.x << ", "
       << pen.y;
    throw ml::text_engine_exception(ss.str());
  }

  if ((error = FT_Glyph_To_Bitmap(&img, FT_RENDER_MODE_NORMAL, 0, 1)) != 0)
  {
    std::stringstream ss;
    ss << "FT_Glyph_To_Bitmap failed, code: " << std::hex << error << std::dec << std::endl;
    ss << "\tGlyph: " << face.glyph(sym.charcode)
       << " format: " << (char *)(&face.glyph(sym.charcode)->format) << std::endl;
    ss << "\twith angle: " << sym.angle << " coord: " << sym.pt << " pen: " << pen.x << ", "
       << pen.y;
    throw ml::text_engine_exception(ss.str());
  }

  FT_BitmapGlyph bitmap = (FT_BitmapGlyph)img;

  if (bitmap && bitmap->bitmap.width)
  {
    renderer(ml::point_d(bitmap->left, bitmap->top - bitmap->bitmap.rows), bitmap->bitmap.width,
             bitmap->bitmap.rows, bitmap->bitmap.buffer);
  }

  FT_Done_Glyph(img);
}

text_engine & text_engine::load_face(std::string const & face_name, std::string const & face_path)
{
  face_map_type::iterator face = m_faces.find(face_name);
  if (face != m_faces.end())
  {
    FT_Done_Face(m_faces[face_name]);
  }

  FT_Error error = FT_New_Face(m_library, face_path.c_str(), 0, &m_faces[face_name]);
  if (error)
    throw ml::text_engine_exception("Can't load font face from path");
  m_current_face = m_faces[face_name];
  return *this;
}

text_engine & text_engine::load_face(std::string const & face_name, char const * face_data,
                                     size_t face_data_size)
{
  face_map_type::iterator face = m_faces.find(face_name);
  if (face != m_faces.end())
  {
    FT_Done_Face(m_faces[face_name]);
  }
  FT_Error error =
      FT_New_Memory_Face(m_library, (const FT_Byte *)&face_data[0], /* first byte in memory */
                         face_data_size,                            /* size in bytes        */
                         0,                                         /* face_index           */
                         &m_faces[face_name]);
  if (error)
    throw ml::text_engine_exception("Can't load font face from memory");
  m_current_face = m_faces[face_name];
  return *this;
}

text_engine & text_engine::select_face(std::string const & face_name)
{
  face_map_type::iterator face = m_faces.find(face_name);
  if (face != m_faces.end())
  {
    m_current_face = m_faces[face_name];
  }
  else
  {
    try
    {
      load_face(face_name, face_name);
    }
    catch (std::exception const & e)
    {
      //				LOG(warning) << "Font face: " << face_name << " not found.
      //Using default." << std::endl;
      m_current_face = m_faces["default"];
    }
  }
  return *this;
}

face::face()
    : m_height(0),
      m_ascender(0),
      m_descender(0),
      m_flip_y(false),
      m_engine((*(ml::text_engine *)NULL))
{
  throw ml::text_engine_exception("No cache entry");
}

face::face(ml::text_engine & e, std::string const & name, size_t size)
    : m_face_name(name), m_face_size(size), m_flip_y(false), m_engine(e)
{
  m_face = e.current_face();
  FT_Size_Metrics metrics = m_face->size->metrics;
  double ss = (double)m_face->units_per_EM / (double)metrics.y_ppem;

  m_ascender = m_face->ascender / ss;
  m_descender = m_face->descender / ss;
  m_height = m_face->height / ss;
}

FT_Glyph face::glyph(unsigned int code, unsigned int prev_code, ml::point_d * kerning)
{
  FT_UInt charcode = FT_Get_Char_Index(m_face, code);

  glyph_cache_type::iterator it = m_glyph_cache.find(charcode);
  if (it == m_glyph_cache.end())
  {
    FT_Error error;

    FT_Glyph glyph;

    FT_Bool use_kerning = FT_HAS_KERNING(m_face);

    if (kerning && use_kerning && prev_code && charcode)
    {
      FT_Vector kern;
      FT_UInt previous = FT_Get_Char_Index(m_face, prev_code);
      error = FT_Get_Kerning(m_face, previous, charcode, FT_KERNING_DEFAULT, &kern);
      //				std::cout << (int)error << " Kerning: " << kern.x << " " <<
      //kern.y << std::endl;
    }

    error = FT_Load_Glyph(m_face, charcode, /* FT_LOAD_DEFAULT */ FT_LOAD_NO_HINTING);
    if (error)
    {
      std::stringstream ss_err;
      ss_err << "Can't load glyph, error: " << error;
      throw ml::text_engine_exception(ss_err.str());
    }

    error = FT_Get_Glyph(m_face->glyph, &glyph);
    if (error)
    {
      throw ml::text_engine_exception("Can't get glyph");
    }
    //			std::cerr << "\tGlyph: " << glyph << " format: " << (char *)(&(glyph->format)) <<
    //std::endl;
    m_glyph_cache[charcode] = glyph;
    return glyph;
  }
  return it->second;
}

ml::face & text_engine::get_face(size_t font_key, std::string const & name, size_t size)
{
  pair<size_t, size_t> key(font_key, size);
  face_cache_type::iterator entry = m_cache.find(key);
  if (entry == m_cache.end())
  {
    select_face(name).set_size(size);
    m_cache.insert(std::make_pair(key, ml::face(*this, name, size)));
    return m_cache[key];
  }
  else
  {
    select_face(entry->second.face_name()).set_size(entry->second.face_size());
    return entry->second;
  }
}

text_engine & text_engine::set_size(size_t height, size_t width)
{
  FT_Error error = FT_Set_Pixel_Sizes(m_current_face,                   /* handle to face object */
                                      static_cast<unsigned int>(width), /* pixel_width           */
                                      static_cast<unsigned int>(height)); /* pixel_height */
  if (error)
    throw ml::text_engine_exception("Can't set face size");
  return *this;
}

text_engine::text_engine()
{
  FT_Error error = FT_Init_FreeType(&m_library);
  if (error)
    throw ml::text_engine_exception("Can't init Freetype library");

  load_face("default", default_font_data, sizeof(default_font_data));
  //        face("default",16);
}
}
