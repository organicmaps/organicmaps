/*
    Copyright 2008 Intel Corporation
 
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_TRANSFORM_DETAIL_HPP
#define BOOST_POLYGON_TRANSFORM_DETAIL_HPP

namespace boost { namespace polygon{
  // inline std::ostream& operator<< (std::ostream& o, const axis_transformation& r) {
  //   o << r.atr_;
  //   return o;
  // }

  // inline std::istream& operator>> (std::istream& i, axis_transformation& r) {
  //   int tmp;
  //   i >> tmp;
  //   r = axis_transformation((axis_transformation::ATR)tmp);
  //   return i;
  // }

  // template <typename scale_factor_type>
  // inline std::ostream& operator<< (std::ostream& o, const anisotropic_scale_factor<scale_factor_type>& sc) {
  //   o << sc.scale_[0] << BOOST_POLYGON_SEP << sc.scale_[1] << GTL_SEP << sc.scale_[2];
  //   return o;
  // }

  // template <typename scale_factor_type>
  // inline std::istream& operator>> (std::istream& i, anisotropic_scale_factor<scale_factor_type>& sc) {
  //   i >> sc.scale_[0] >> sc.scale_[1] >> sc.scale_[2];
  //   return i;
  // }

  // template <typename coordinate_type>
  // inline std::ostream& operator<< (std::ostream& o, const transformation& tr) {
  //   o << tr.atr_ << BOOST_POLYGON_SEP << tr.p_;
  //   return o;
  // }

  // template <typename coordinate_type>
  // inline std::istream& operator>> (std::istream& i, transformation& tr) {
  //   i >> tr.atr_ >> tr.p_;
  //   return i;
  // }


  inline axis_transformation::axis_transformation(const orientation_3d& orient) : atr_(NULL_TRANSFORM) {
    const ATR tmp[3] = {
      UP_EAST_NORTH, //sort by x, then z, then y
      EAST_UP_NORTH, //sort by y, then z, then x
      EAST_NORTH_UP  //sort by z, then y, then x
    };
    atr_ = tmp[orient.to_int()];
  }
  
  inline axis_transformation::axis_transformation(const orientation_2d& orient) : atr_(NULL_TRANSFORM) {
    const ATR tmp[3] = {
      NORTH_EAST_UP, //sort by z, then x, then y
      EAST_NORTH_UP  //sort by z, then y, then x
    };
    atr_ = tmp[orient.to_int()];
  }
  
  inline axis_transformation::axis_transformation(const direction_3d& dir) : atr_(NULL_TRANSFORM) {
    const ATR tmp[6] = {
      DOWN_EAST_NORTH, //sort by -x, then z, then y
      UP_EAST_NORTH,   //sort by x, then z, then y
      EAST_DOWN_NORTH, //sort by -y, then z, then x
      EAST_UP_NORTH,   //sort by y, then z, then x
      EAST_NORTH_DOWN, //sort by -z, then y, then x
      EAST_NORTH_UP    //sort by z, then y, then x
    };
    atr_ = tmp[dir.to_int()];
  }
  
  inline axis_transformation::axis_transformation(const direction_2d& dir) : atr_(NULL_TRANSFORM) {
    const ATR tmp[4] = {
      SOUTH_EAST_UP, //sort by z, then x, then y
      NORTH_EAST_UP, //sort by z, then x, then y
      EAST_SOUTH_UP, //sort by z, then y, then x
      EAST_NORTH_UP  //sort by z, then y, then x
    };
    atr_ = tmp[dir.to_int()];
  }
  
  inline axis_transformation& axis_transformation::operator=(const axis_transformation& a) {
    atr_ = a.atr_;
    return *this;
  }

  inline axis_transformation& axis_transformation::operator=(const ATR& atr) {
    atr_ = atr;
    return *this;
  }

  inline bool axis_transformation::operator==(const axis_transformation& a) const {
    return atr_ == a.atr_;
  }

  inline bool axis_transformation::operator!=(const axis_transformation& a) const {
    return !(*this == a);
  }

  inline bool axis_transformation::operator<(const axis_transformation& a) const {
    return atr_ < a.atr_;
  }

  inline axis_transformation& axis_transformation::operator+=(const axis_transformation& a){
    bool abit5 = (a.atr_ & 32) != 0;
    bool abit4 = (a.atr_ & 16) != 0;
    bool abit3 = (a.atr_ & 8) != 0;
    bool abit2 = (a.atr_ & 4) != 0;
    bool abit1 = (a.atr_ & 2) != 0;
    bool abit0 = (a.atr_ & 1) != 0;      
    bool bit5 = (atr_ & 32) != 0;
    bool bit4 = (atr_ & 16) != 0;
    bool bit3 = (atr_ & 8) != 0;
    bool bit2 = (atr_ & 4) != 0;
    bool bit1 = (atr_ & 2) != 0;
    bool bit0 = (atr_ & 1) != 0;      
    int indexes[2][3] = {
      {
        ((int)((bit5 & bit2) | (bit4 & !bit2)) << 1) +
        (int)(bit2 & !bit5),
        ((int)((bit4 & bit2) | (bit5 & !bit2)) << 1) +
        (int)(!bit5 & !bit2),
        ((int)(!bit4 & !bit5) << 1) +
        (int)(bit5) 
      },
      {
        ((int)((abit5 & abit2) | (abit4 & !abit2)) << 1) +
        (int)(abit2 & !abit5),
        ((int)((abit4 & abit2) | (abit5 & !abit2)) << 1) +
        (int)(!abit5 & !abit2),
        ((int)(!abit4 & !abit5) << 1) +
        (int)(abit5) 
      }
    };
    int zero_bits[2][3] = {
      {bit0, bit1, bit3},
      {abit0, abit1, abit3}
    };
    int nbit3 = zero_bits[0][2] ^ zero_bits[1][indexes[0][2]];
    int nbit1 = zero_bits[0][1] ^ zero_bits[1][indexes[0][1]];
    int nbit0 = zero_bits[0][0] ^ zero_bits[1][indexes[0][0]];
    indexes[0][0] = indexes[1][indexes[0][0]];
    indexes[0][1] = indexes[1][indexes[0][1]];
    indexes[0][2] = indexes[1][indexes[0][2]];
    int nbit5 = (indexes[0][2] == 1);
    int nbit4 = (indexes[0][2] == 0);
    int nbit2 = (!(nbit5 | nbit4) & (bool)(indexes[0][0] & 1)) | //swap xy
      (nbit5 & ((indexes[0][0] & 2) >> 1)) | //z->y x->z
      (nbit4 & ((indexes[0][1] & 2) >> 1));  //z->x y->z
    atr_ = (ATR)((nbit5 << 5) + 
                 (nbit4 << 4) + 
                 (nbit3 << 3) + 
                 (nbit2 << 2) + 
                 (nbit1 << 1) + nbit0);
    return *this;
  }
  
  inline axis_transformation axis_transformation::operator+(const axis_transformation& a) const {
    axis_transformation retval(*this);
    return retval+=a;
  }
  
  // populate_axis_array writes the three INDIVIDUAL_AXIS values that the
  // ATR enum value of 'this' represent into axis_array
  inline void axis_transformation::populate_axis_array(INDIVIDUAL_AXIS axis_array[]) const {
    bool bit5 = (atr_ & 32) != 0;
    bool bit4 = (atr_ & 16) != 0;
    bool bit3 = (atr_ & 8) != 0;
    bool bit2 = (atr_ & 4) != 0;
    bool bit1 = (atr_ & 2) != 0;
    bool bit0 = (atr_ & 1) != 0;      
    axis_array[2] = 
      (INDIVIDUAL_AXIS)((((int)(!bit4 & !bit5)) << 2) +
                        ((int)(bit5) << 1) + 
                        bit3);
    axis_array[1] = 
      (INDIVIDUAL_AXIS)((((int)((bit4 & bit2) | (bit5 & !bit2))) << 2)+
                        ((int)(!bit5 & !bit2) << 1) + 
                        bit1);
    axis_array[0] = 
      (INDIVIDUAL_AXIS)((((int)((bit5 & bit2) | (bit4 & !bit2))) << 2) +
                        ((int)(bit2 & !bit5) << 1) + 
                        bit0);
  }
  
  // combine_axis_arrays concatenates this_array and that_array overwriting
  // the result into this_array
  inline void 
  axis_transformation::combine_axis_arrays (INDIVIDUAL_AXIS this_array[],
                                            const INDIVIDUAL_AXIS that_array[]){
    int indexes[3] = {this_array[0] >> 1,
                      this_array[1] >> 1,
                      this_array[2] >> 1};
    int zero_bits[2][3] = {
      {this_array[0] & 1, this_array[1] & 1, this_array[2] & 1},
      {that_array[0] & 1, that_array[1] & 1, that_array[2] & 1}
    };
    this_array[0] = that_array[indexes[0]];
    this_array[0] = (INDIVIDUAL_AXIS)((int)this_array[0] & (int)((int)PZ+(int)PY));
    this_array[0] = (INDIVIDUAL_AXIS)((int)this_array[0] | 
                                      ((int)zero_bits[0][0] ^ 
                                       (int)zero_bits[1][indexes[0]]));
    this_array[1] = that_array[indexes[1]];
    this_array[1] = (INDIVIDUAL_AXIS)((int)this_array[1] & (int)((int)PZ+(int)PY));
    this_array[1] = (INDIVIDUAL_AXIS)((int)this_array[1] | 
                                      ((int)zero_bits[0][1] ^ 
                                       (int)zero_bits[1][indexes[1]]));
    this_array[2] = that_array[indexes[2]];
    this_array[2] = (INDIVIDUAL_AXIS)((int)this_array[2] & (int)((int)PZ+(int)PY));
    this_array[2] = (INDIVIDUAL_AXIS)((int)this_array[2] | 
                                      ((int)zero_bits[0][2] ^ 
                                       (int)zero_bits[1][indexes[2]]));
  }
  
  // write_back_axis_array converts an array of three INDIVIDUAL_AXIS values
  // to the ATR enum value and sets 'this' to that value
  inline void axis_transformation::write_back_axis_array(const INDIVIDUAL_AXIS this_array[]) {
    int bit5 = ((int)this_array[2] & 2) != 0;
    int bit4 = !((((int)this_array[2] & 4) != 0) | (((int)this_array[2] & 2) != 0));
    int bit3 = ((int)this_array[2] & 1) != 0;
    //bit 2 is the tricky bit
    int bit2 = ((!(bit5 | bit4)) & (((int)this_array[0] & 2) != 0)) | //swap xy
      (bit5 & (((int)this_array[0] & 4) >> 2)) | //z->y x->z
      (bit4 & (((int)this_array[1] & 4) >> 2));  //z->x y->z
    int bit1 = ((int)this_array[1] & 1);
    int bit0 = ((int)this_array[0] & 1);
    atr_ = ATR((bit5 << 5) + 
               (bit4 << 4) + 
               (bit3 << 3) + 
               (bit2 << 2) + 
               (bit1 << 1) + bit0);
  }
  
  // behavior is deterministic but undefined in the case where illegal
  // combinations of directions are passed in. 
  inline axis_transformation& 
  axis_transformation::set_directions(const direction_2d& horizontalDir,
                                      const direction_2d& verticalDir){
    int bit2 = (static_cast<orientation_2d>(horizontalDir).to_int()) != 0;
    int bit1 = !(verticalDir.to_int() & 1);
    int bit0 = !(horizontalDir.to_int() & 1);
    atr_ = ATR((bit2 << 2) + (bit1 << 1) + bit0);
    return *this;
  }
  
  // behavior is deterministic but undefined in the case where illegal
  // combinations of directions are passed in.
  inline axis_transformation& axis_transformation::set_directions(const direction_3d& horizontalDir,
                                                                  const direction_3d& verticalDir,
                                                                  const direction_3d& proximalDir){
    int this_array[3] = {horizontalDir.to_int(),
                         verticalDir.to_int(),
                         proximalDir.to_int()};
    int bit5 = (this_array[2] & 2) != 0;
    int bit4 = !(((this_array[2] & 4) != 0) | ((this_array[2] & 2) != 0));
    int bit3 = !((this_array[2] & 1) != 0);
    //bit 2 is the tricky bit
    int bit2 = (!(bit5 | bit4) & ((this_array[0] & 2) != 0 )) | //swap xy
      (bit5 & ((this_array[0] & 4) >> 2)) | //z->y x->z
      (bit4 & ((this_array[1] & 4) >> 2));  //z->x y->z
    int bit1 = !(this_array[1] & 1);
    int bit0 = !(this_array[0] & 1);
    atr_ = ATR((bit5 << 5) + 
               (bit4 << 4) + 
               (bit3 << 3) + 
               (bit2 << 2) + 
               (bit1 << 1) + bit0);
    return *this;
  }
  
  template <typename coordinate_type_2>
  inline void axis_transformation::transform(coordinate_type_2& x, coordinate_type_2& y) const {
    int bit2 = (atr_ & 4) != 0;
    int bit1 = (atr_ & 2) != 0;
    int bit0 = (atr_ & 1) != 0;
    x *= -((bit0 << 1) - 1);
    y *= -((bit1 << 1) - 1);    
    predicated_swap(bit2 != 0,x,y);
  }
  
  template <typename coordinate_type_2>
  inline void axis_transformation::transform(coordinate_type_2& x, coordinate_type_2& y, coordinate_type_2& z) const {
    int bit5 = (atr_ & 32) != 0;
    int bit4 = (atr_ & 16) != 0;
    int bit3 = (atr_ & 8) != 0;
    int bit2 = (atr_ & 4) != 0;
    int bit1 = (atr_ & 2) != 0;
    int bit0 = (atr_ & 1) != 0;
    x *= -((bit0 << 1) - 1);
    y *= -((bit1 << 1) - 1);    
    z *= -((bit3 << 1) - 1);
    predicated_swap(bit2 != 0, x, y);
    predicated_swap(bit5 != 0, y, z);
    predicated_swap(bit4 != 0, x, z);
  }
  
  inline axis_transformation& axis_transformation::invert_2d() {
    int bit2 = ((atr_ & 4) != 0);
    int bit1 = ((atr_ & 2) != 0);
    int bit0 = ((atr_ & 1) != 0);
    //swap bit 0 and bit 1 if bit2 is 1
    predicated_swap(bit2 != 0, bit0, bit1);
    bit1 = bit1 << 1;
    atr_ = (ATR)(atr_ & (32+16+8+4)); //mask away bit0 and bit1
    atr_ = (ATR)(atr_ | bit0 | bit1);
    return *this;
  }
  
  inline axis_transformation axis_transformation::inverse_2d() const {
    axis_transformation retval(*this);
    return retval.invert_2d();
  }
  
  inline axis_transformation& axis_transformation::invert() {
    int bit5 = ((atr_ & 32) != 0);
    int bit4 = ((atr_ & 16) != 0);    
    int bit3 = ((atr_ & 8) != 0);
    int bit2 = ((atr_ & 4) != 0);
    int bit1 = ((atr_ & 2) != 0);
    int bit0 = ((atr_ & 1) != 0);
    predicated_swap(bit2 != 0, bit4, bit5);
    predicated_swap(bit4 != 0, bit0, bit3);
    predicated_swap(bit5 != 0, bit1, bit3);
    predicated_swap(bit2 != 0, bit0, bit1);
    atr_ = (ATR)((bit5 << 5) + 
                 (bit4 << 4) + 
                 (bit3 << 3) + 
                 (bit2 << 2) + 
                 (bit1 << 1) + bit0);
    return *this;
  }
  
  inline axis_transformation axis_transformation::inverse() const {
    axis_transformation retval(*this);
    return retval.invert();
  }
  
  template <typename scale_factor_type>
  inline scale_factor_type anisotropic_scale_factor<scale_factor_type>::get(orientation_3d orient) const {
    return scale_[orient.to_int()];
  }
  
  template <typename scale_factor_type>
  inline void anisotropic_scale_factor<scale_factor_type>::set(orientation_3d orient, scale_factor_type value) {
    scale_[orient.to_int()] = value;
  }

  template <typename scale_factor_type>
  inline scale_factor_type anisotropic_scale_factor<scale_factor_type>::x() const { return scale_[HORIZONTAL]; }
  template <typename scale_factor_type>
  inline scale_factor_type anisotropic_scale_factor<scale_factor_type>::y() const { return scale_[VERTICAL]; }
  template <typename scale_factor_type>
  inline scale_factor_type anisotropic_scale_factor<scale_factor_type>::z() const { return scale_[PROXIMAL]; }
  template <typename scale_factor_type>
  inline void anisotropic_scale_factor<scale_factor_type>::x(scale_factor_type value) { scale_[HORIZONTAL] = value; }
  template <typename scale_factor_type>
  inline void anisotropic_scale_factor<scale_factor_type>::y(scale_factor_type value) { scale_[VERTICAL] = value; }
  template <typename scale_factor_type>
  inline void anisotropic_scale_factor<scale_factor_type>::z(scale_factor_type value) { scale_[PROXIMAL] = value; }
  
  //concatenation operator (convolve scale factors)
  template <typename scale_factor_type>
  inline anisotropic_scale_factor<scale_factor_type> anisotropic_scale_factor<scale_factor_type>::operator+(const anisotropic_scale_factor<scale_factor_type>& s) const {
    anisotropic_scale_factor<scale_factor_type> retval(*this);
    return retval+=s;
  }
  
  //concatenate this with that
  template <typename scale_factor_type>
  inline const anisotropic_scale_factor<scale_factor_type>& anisotropic_scale_factor<scale_factor_type>::operator+=(const anisotropic_scale_factor<scale_factor_type>& s){
    scale_[0] *= s.scale_[0];
    scale_[1] *= s.scale_[1];
    scale_[2] *= s.scale_[2];
    return *this;
  }
  
  //transform
  template <typename scale_factor_type>
  inline anisotropic_scale_factor<scale_factor_type>& anisotropic_scale_factor<scale_factor_type>::transform(axis_transformation atr){
    direction_3d dirs[3];
    atr.get_directions(dirs[0],dirs[1],dirs[2]);
    scale_factor_type tmp[3] = {scale_[0], scale_[1], scale_[2]};
    for(int i = 0; i < 3; ++i){
      scale_[orientation_3d(dirs[i]).to_int()] = tmp[i];
    }
    return *this;
  }

  template <typename scale_factor_type>
  template <typename coordinate_type_2>
  inline void anisotropic_scale_factor<scale_factor_type>::scale(coordinate_type_2& x, coordinate_type_2& y) const {
    x = scaling_policy<coordinate_type_2>::round((scale_factor_type)x * get(HORIZONTAL));
    y = scaling_policy<coordinate_type_2>::round((scale_factor_type)y * get(HORIZONTAL));
  }

  template <typename scale_factor_type>
  template <typename coordinate_type_2>
  inline void anisotropic_scale_factor<scale_factor_type>::scale(coordinate_type_2& x, coordinate_type_2& y, coordinate_type_2& z) const {
    scale(x, y);
    z = scaling_policy<coordinate_type_2>::round((scale_factor_type)z * get(HORIZONTAL));
  }

  template <typename scale_factor_type>
  inline anisotropic_scale_factor<scale_factor_type>& anisotropic_scale_factor<scale_factor_type>::invert() {
    x(1/x());
    y(1/y());
    z(1/z());
    return *this;
  }


  template <typename coordinate_type>
  inline transformation<coordinate_type>::transformation() : atr_(), p_(0, 0, 0) {;}

  template <typename coordinate_type>
  inline transformation<coordinate_type>::transformation(axis_transformation atr) : atr_(atr), p_(0, 0, 0){;}

  template <typename coordinate_type>
  inline transformation<coordinate_type>::transformation(axis_transformation::ATR atr) : atr_(atr), p_(0, 0, 0){;}

  template <typename coordinate_type>
  template <typename point_type>
  inline transformation<coordinate_type>::transformation(const point_type& p) : atr_(), p_(0, 0, 0) {
    set_translation(p);
  }

  template <typename coordinate_type>
  template <typename point_type>
  inline transformation<coordinate_type>::transformation(axis_transformation atr, const point_type& p) :
    atr_(atr), p_(0, 0, 0) {
    set_translation(p);
  }

  template <typename coordinate_type>
  template <typename point_type>
  inline transformation<coordinate_type>::transformation(axis_transformation atr, const point_type& referencePt, const point_type& destinationPt) : atr_(), p_(0, 0, 0) {
    transformation<coordinate_type> tmp(referencePt);
    transformation<coordinate_type> rotRef(atr);
    transformation<coordinate_type> tmpInverse = tmp.inverse();
    point_type decon(referencePt);
    deconvolve(decon, destinationPt);
    transformation<coordinate_type> displacement(decon);
    tmp += rotRef;
    tmp += tmpInverse;
    tmp += displacement;
    (*this) = tmp;
  }

  template <typename coordinate_type>
  inline transformation<coordinate_type>::transformation(const transformation<coordinate_type>& tr) : 
    atr_(tr.atr_), p_(tr.p_) {;}
  
  template <typename coordinate_type>
  inline bool transformation<coordinate_type>::operator==(const transformation<coordinate_type>& tr) const {
    return atr_ == tr.atr_ && p_ == tr.p_;
  }
  
  template <typename coordinate_type>
  inline bool transformation<coordinate_type>::operator!=(const transformation<coordinate_type>& tr) const {
    return !(*this == tr);
  }
  
  template <typename coordinate_type>
  inline bool transformation<coordinate_type>::operator<(const transformation<coordinate_type>& tr) const {
    return atr_ < tr.atr_ || atr_ == tr.atr_ && p_ < tr.p_;
  }
  
  template <typename coordinate_type>
  inline transformation<coordinate_type> transformation<coordinate_type>::operator+(const transformation<coordinate_type>& tr) const {
    transformation<coordinate_type> retval(*this);
    return retval+=tr;
  }
  
  template <typename coordinate_type>
  inline const transformation<coordinate_type>& transformation<coordinate_type>::operator+=(const transformation<coordinate_type>& tr){
    //apply the inverse transformation of this to the translation point of that
    //and convolve it with this translation point
    coordinate_type x, y, z;
    transformation<coordinate_type> inv = inverse();
    inv.transform(x, y, z);
    p_.set(HORIZONTAL, p_.get(HORIZONTAL) + x);
    p_.set(VERTICAL, p_.get(VERTICAL) + y);
    p_.set(PROXIMAL, p_.get(PROXIMAL) + z);
    //concatenate axis transforms
    atr_ += tr.atr_;
    return *this;
  }
  
  template <typename coordinate_type>
  inline void transformation<coordinate_type>::set_axis_transformation(const axis_transformation& atr) {
    atr_ = atr;
  }
  
  template <typename coordinate_type>
  template <typename point_type>
  inline void transformation<coordinate_type>::get_translation(point_type& p) const {
    assign(p, p_);
  }
  
  template <typename coordinate_type>
  template <typename point_type>
  inline void transformation<coordinate_type>::set_translation(const point_type& p) {
    assign(p_, p);
  }
  
  template <typename coordinate_type>
  inline void transformation<coordinate_type>::transform(coordinate_type& x, coordinate_type& y) const {
    //subtract each component of new origin point
    y -= p_.get(VERTICAL);
    x -= p_.get(HORIZONTAL);
    atr_.transform(x, y);
  }

  template <typename coordinate_type>
  inline void transformation<coordinate_type>::transform(coordinate_type& x, coordinate_type& y, coordinate_type& z) const {
    //subtract each component of new origin point
    z -= p_.get(PROXIMAL);
    y -= p_.get(VERTICAL);
    x -= p_.get(HORIZONTAL);
    atr_.transform(x,y,z);
  }
  
  // sets the axis_transform portion to its inverse
  // transforms the tranlastion portion by that inverse axis_transform
  // multiplies the translation portion by -1 to reverse it
  template <typename coordinate_type>
  inline transformation<coordinate_type>& transformation<coordinate_type>::invert() {
    coordinate_type x = p_.get(HORIZONTAL), y = p_.get(VERTICAL), z = p_.get(PROXIMAL);
    atr_.transform(x, y, z);
    x *= -1;
    y *= -1;
    z *= -1;
    p_ = point_3d_data<coordinate_type>(x, y, z);
    atr_.invert();
    return *this;
  }
  
  template <typename coordinate_type>
  inline transformation<coordinate_type> transformation<coordinate_type>::inverse() const {
    transformation<coordinate_type> retval(*this);
    return retval.invert();
  }
}
}
#endif


