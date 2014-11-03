///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///
/// @ref core
/// @file glm/core/func_matrix.inl
/// @date 2008-03-08 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "../geometric.hpp"
#include <limits>

namespace glm{
namespace detail
{
	template
	<
		template <class, precision> class vecTypeA,
		template <class, precision> class vecTypeB,
		typename T, precision P
	>
	struct compute_outerProduct{};

	template <typename T, precision P>
	struct compute_outerProduct<detail::tvec2, detail::tvec2, T, P>
	{
		GLM_FUNC_QUALIFIER static typename detail::outerProduct_trait<T, P, detail::tvec2, detail::tvec2>::type call(detail::tvec2<T, P> const & c, detail::tvec2<T, P> const & r)
		{
			detail::tmat2x2<T, P> m(detail::tmat2x2<T, P>::_null);
			m[0][0] = c[0] * r[0];
			m[0][1] = c[1] * r[0];
			m[1][0] = c[0] * r[1];
			m[1][1] = c[1] * r[1];
			return m;
		}
	};

	template <typename T, precision P>
	struct compute_outerProduct<detail::tvec3, detail::tvec3, T, P>
	{
		GLM_FUNC_QUALIFIER static typename detail::outerProduct_trait<T, P, detail::tvec3, detail::tvec3>::type call(detail::tvec3<T, P> const & c, detail::tvec3<T, P> const & r)
		{
			detail::tmat3x3<T, P> m(detail::tmat3x3<T, P>::_null);
			for(length_t i(0); i < m.length(); ++i)
				m[i] = c * r[i];
			return m;
		}
	};

	template <typename T, precision P>
	struct compute_outerProduct<detail::tvec4, detail::tvec4, T, P>
	{
		GLM_FUNC_QUALIFIER static typename detail::outerProduct_trait<T, P, detail::tvec4, detail::tvec4>::type call(detail::tvec4<T, P> const & c, detail::tvec4<T, P> const & r)
		{
			detail::tmat4x4<T, P> m(detail::tmat4x4<T, P>::_null);
			for(length_t i(0); i < m.length(); ++i)
				m[i] = c * r[i];
			return m;
		}
	};

	template <typename T, precision P>
	struct compute_outerProduct<detail::tvec3, detail::tvec2, T, P>
	{
		GLM_FUNC_QUALIFIER static typename detail::outerProduct_trait<T, P, detail::tvec3, detail::tvec2>::type call(detail::tvec3<T, P> const & c, detail::tvec2<T, P> const & r)
		{
			detail::tmat2x3<T, P> m(detail::tmat2x3<T, P>::_null);
			m[0][0] = c.x * r.x;
			m[0][1] = c.y * r.x;
			m[0][2] = c.z * r.x;
			m[1][0] = c.x * r.y;
			m[1][1] = c.y * r.y;
			m[1][2] = c.z * r.y;
			return m;
		}
	};

	template <typename T, precision P>
	struct compute_outerProduct<detail::tvec2, detail::tvec3, T, P>
	{
		GLM_FUNC_QUALIFIER static typename detail::outerProduct_trait<T, P, detail::tvec2, detail::tvec3>::type call(detail::tvec2<T, P> const & c, detail::tvec3<T, P> const & r)
		{
			detail::tmat3x2<T, P> m(detail::tmat3x2<T, P>::_null);
			m[0][0] = c.x * r.x;
			m[0][1] = c.y * r.x;
			m[1][0] = c.x * r.y;
			m[1][1] = c.y * r.y;
			m[2][0] = c.x * r.z;
			m[2][1] = c.y * r.z;
			return m;
		}
	};

	template <typename T, precision P>
	struct compute_outerProduct<detail::tvec4, detail::tvec2, T, P>
	{
		GLM_FUNC_QUALIFIER static typename detail::outerProduct_trait<T, P, detail::tvec4, detail::tvec2>::type call(detail::tvec4<T, P> const & c, detail::tvec2<T, P> const & r)
		{
			detail::tmat2x4<T, P> m(detail::tmat2x4<T, P>::_null);
			m[0][0] = c.x * r.x;
			m[0][1] = c.y * r.x;
			m[0][2] = c.z * r.x;
			m[0][3] = c.w * r.x;
			m[1][0] = c.x * r.y;
			m[1][1] = c.y * r.y;
			m[1][2] = c.z * r.y;
			m[1][3] = c.w * r.y;
			return m;
		}
	};

	template <typename T, precision P>
	struct compute_outerProduct<detail::tvec2, detail::tvec4, T, P>
	{
		GLM_FUNC_QUALIFIER static typename detail::outerProduct_trait<T, P, detail::tvec2, detail::tvec4>::type call(detail::tvec2<T, P> const & c, detail::tvec4<T, P> const & r)
		{
			detail::tmat4x2<T, P> m(detail::tmat4x2<T, P>::_null);
			m[0][0] = c.x * r.x;
			m[0][1] = c.y * r.x;
			m[1][0] = c.x * r.y;
			m[1][1] = c.y * r.y;
			m[2][0] = c.x * r.z;
			m[2][1] = c.y * r.z;
			m[3][0] = c.x * r.w;
			m[3][1] = c.y * r.w;
			return m;
		}
	};

	template <typename T, precision P>
	struct compute_outerProduct<detail::tvec4, detail::tvec3, T, P>
	{
		GLM_FUNC_QUALIFIER static typename detail::outerProduct_trait<T, P, detail::tvec4, detail::tvec3>::type call(detail::tvec4<T, P> const & c, detail::tvec3<T, P> const & r)
		{
			detail::tmat3x4<T, P> m(detail::tmat3x4<T, P>::_null);
			m[0][0] = c.x * r.x;
			m[0][1] = c.y * r.x;
			m[0][2] = c.z * r.x;
			m[0][3] = c.w * r.x;
			m[1][0] = c.x * r.y;
			m[1][1] = c.y * r.y;
			m[1][2] = c.z * r.y;
			m[1][3] = c.w * r.y;
			m[2][0] = c.x * r.z;
			m[2][1] = c.y * r.z;
			m[2][2] = c.z * r.z;
			m[2][3] = c.w * r.z;
			return m;
		}
	};

	template <typename T, precision P>
	struct compute_outerProduct<detail::tvec3, detail::tvec4, T, P>
	{
		GLM_FUNC_QUALIFIER static typename detail::outerProduct_trait<T, P, detail::tvec3, detail::tvec4>::type call(detail::tvec3<T, P> const & c, detail::tvec4<T, P> const & r)
		{
			detail::tmat4x3<T, P> m(detail::tmat4x3<T, P>::_null);
			m[0][0] = c.x * r.x;
			m[0][1] = c.y * r.x;
			m[0][2] = c.z * r.x;
			m[1][0] = c.x * r.y;
			m[1][1] = c.y * r.y;
			m[1][2] = c.z * r.y;
			m[2][0] = c.x * r.z;
			m[2][1] = c.y * r.z;
			m[2][2] = c.z * r.z;
			m[3][0] = c.x * r.w;
			m[3][1] = c.y * r.w;
			m[3][2] = c.z * r.w;
			return m;
		}
	};

	template <template <class, precision> class matType, typename T, precision P>
	struct compute_transpose{};

	template <typename T, precision P>
	struct compute_transpose<detail::tmat2x2, T, P>
	{
		GLM_FUNC_QUALIFIER static detail::tmat2x2<T, P> call(detail::tmat2x2<T, P> const & m)
		{
			detail::tmat2x2<T, P> result(detail::tmat2x2<T, P>::_null);
			result[0][0] = m[0][0];
			result[0][1] = m[1][0];
			result[1][0] = m[0][1];
			result[1][1] = m[1][1];
			return result;
		}
	};

	template <typename T, precision P>
	struct compute_transpose<detail::tmat2x3, T, P>
	{
		GLM_FUNC_QUALIFIER static detail::tmat3x2<T, P> call(detail::tmat2x3<T, P> const & m)
		{
			detail::tmat3x2<T, P> result(detail::tmat3x2<T, P>::_null);
			result[0][0] = m[0][0];
			result[0][1] = m[1][0];
			result[1][0] = m[0][1];
			result[1][1] = m[1][1];
			result[2][0] = m[0][2];
			result[2][1] = m[1][2];
			return result;
		}
	};

	template <typename T, precision P>
	struct compute_transpose<detail::tmat2x4, T, P>
	{
		GLM_FUNC_QUALIFIER static detail::tmat4x2<T, P> call(detail::tmat2x4<T, P> const & m)
		{
			detail::tmat4x2<T, P> result(detail::tmat4x2<T, P>::_null);
			result[0][0] = m[0][0];
			result[0][1] = m[1][0];
			result[1][0] = m[0][1];
			result[1][1] = m[1][1];
			result[2][0] = m[0][2];
			result[2][1] = m[1][2];
			result[3][0] = m[0][3];
			result[3][1] = m[1][3];
			return result;
		}
	};

	template <typename T, precision P>
	struct compute_transpose<detail::tmat3x2, T, P>
	{
		GLM_FUNC_QUALIFIER static detail::tmat2x3<T, P> call(detail::tmat3x2<T, P> const & m)
		{
			detail::tmat2x3<T, P> result(detail::tmat2x3<T, P>::_null);
			result[0][0] = m[0][0];
			result[0][1] = m[1][0];
			result[0][2] = m[2][0];
			result[1][0] = m[0][1];
			result[1][1] = m[1][1];
			result[1][2] = m[2][1];
			return result;
		}
	};

	template <typename T, precision P>
	struct compute_transpose<detail::tmat3x3, T, P>
	{
		GLM_FUNC_QUALIFIER static detail::tmat3x3<T, P> call(detail::tmat3x3<T, P> const & m)
		{
			detail::tmat3x3<T, P> result(detail::tmat3x3<T, P>::_null);
			result[0][0] = m[0][0];
			result[0][1] = m[1][0];
			result[0][2] = m[2][0];

			result[1][0] = m[0][1];
			result[1][1] = m[1][1];
			result[1][2] = m[2][1];

			result[2][0] = m[0][2];
			result[2][1] = m[1][2];
			result[2][2] = m[2][2];
			return result;
		}
	};

	template <typename T, precision P>
	struct compute_transpose<detail::tmat3x4, T, P>
	{
		GLM_FUNC_QUALIFIER static detail::tmat4x3<T, P> call(detail::tmat3x4<T, P> const & m)
		{
			detail::tmat4x3<T, P> result(detail::tmat4x3<T, P>::_null);
			result[0][0] = m[0][0];
			result[0][1] = m[1][0];
			result[0][2] = m[2][0];
			result[1][0] = m[0][1];
			result[1][1] = m[1][1];
			result[1][2] = m[2][1];
			result[2][0] = m[0][2];
			result[2][1] = m[1][2];
			result[2][2] = m[2][2];
			result[3][0] = m[0][3];
			result[3][1] = m[1][3];
			result[3][2] = m[2][3];
			return result;
		}
	};

	template <typename T, precision P>
	struct compute_transpose<detail::tmat4x2, T, P>
	{
		GLM_FUNC_QUALIFIER static detail::tmat2x4<T, P> call(detail::tmat4x2<T, P> const & m)
		{
			detail::tmat2x4<T, P> result(detail::tmat2x4<T, P>::_null);
			result[0][0] = m[0][0];
			result[0][1] = m[1][0];
			result[0][2] = m[2][0];
			result[0][3] = m[3][0];
			result[1][0] = m[0][1];
			result[1][1] = m[1][1];
			result[1][2] = m[2][1];
			result[1][3] = m[3][1];
			return result;
		}
	};

	template <typename T, precision P>
	struct compute_transpose<detail::tmat4x3, T, P>
	{
		GLM_FUNC_QUALIFIER static detail::tmat3x4<T, P> call(detail::tmat4x3<T, P> const & m)
		{
			detail::tmat3x4<T, P> result(detail::tmat3x4<T, P>::_null);
			result[0][0] = m[0][0];
			result[0][1] = m[1][0];
			result[0][2] = m[2][0];
			result[0][3] = m[3][0];
			result[1][0] = m[0][1];
			result[1][1] = m[1][1];
			result[1][2] = m[2][1];
			result[1][3] = m[3][1];
			result[2][0] = m[0][2];
			result[2][1] = m[1][2];
			result[2][2] = m[2][2];
			result[2][3] = m[3][2];
			return result;
		}
	};

	template <typename T, precision P>
	struct compute_transpose<detail::tmat4x4, T, P>
	{
		GLM_FUNC_QUALIFIER static detail::tmat4x4<T, P> call(detail::tmat4x4<T, P> const & m)
		{
			detail::tmat4x4<T, P> result(detail::tmat4x4<T, P>::_null);
			result[0][0] = m[0][0];
			result[0][1] = m[1][0];
			result[0][2] = m[2][0];
			result[0][3] = m[3][0];

			result[1][0] = m[0][1];
			result[1][1] = m[1][1];
			result[1][2] = m[2][1];
			result[1][3] = m[3][1];

			result[2][0] = m[0][2];
			result[2][1] = m[1][2];
			result[2][2] = m[2][2];
			result[2][3] = m[3][2];

			result[3][0] = m[0][3];
			result[3][1] = m[1][3];
			result[3][2] = m[2][3];
			result[3][3] = m[3][3];
			return result;
		}
	};

	template <template <class, precision> class matType, typename T, precision P>
	struct compute_determinant{};

	template <typename T, precision P>
	struct compute_determinant<detail::tmat2x2, T, P>
	{
		GLM_FUNC_QUALIFIER static T call(detail::tmat2x2<T, P> const & m)
		{
			return m[0][0] * m[1][1] - m[1][0] * m[0][1];
		}
	};

	template <typename T, precision P>
	struct compute_determinant<detail::tmat3x3, T, P>
	{
		GLM_FUNC_QUALIFIER static T call(detail::tmat3x3<T, P> const & m)
		{
			return
				+ m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
				- m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
				+ m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
		}
	};

	template <typename T, precision P>
	struct compute_determinant<detail::tmat4x4, T, P>
	{
		GLM_FUNC_QUALIFIER static T call(detail::tmat4x4<T, P> const & m)
		{
			T SubFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
			T SubFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
			T SubFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
			T SubFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
			T SubFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
			T SubFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];

			detail::tvec4<T, P> DetCof(
				+ (m[1][1] * SubFactor00 - m[1][2] * SubFactor01 + m[1][3] * SubFactor02),
				- (m[1][0] * SubFactor00 - m[1][2] * SubFactor03 + m[1][3] * SubFactor04),
				+ (m[1][0] * SubFactor01 - m[1][1] * SubFactor03 + m[1][3] * SubFactor05),
				- (m[1][0] * SubFactor02 - m[1][1] * SubFactor04 + m[1][2] * SubFactor05));

			return
				m[0][0] * DetCof[0] + m[0][1] * DetCof[1] +
				m[0][2] * DetCof[2] + m[0][3] * DetCof[3];
		}
	};
}//namespace detail

	template <typename T, precision P, template <typename, precision> class matType>
	GLM_FUNC_QUALIFIER matType<T, P> matrixCompMult(matType<T, P> const & x, matType<T, P> const & y)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'matrixCompMult' only accept floating-point inputs");

		matType<T, P> result(matType<T, P>::_null);
		for(length_t i = 0; i < result.length(); ++i)
			result[i] = x[i] * y[i];
		return result;
	}

	template<typename T, precision P, template <typename, precision> class vecTypeA, template <typename, precision> class vecTypeB>
	GLM_FUNC_QUALIFIER typename detail::outerProduct_trait<T, P, vecTypeA, vecTypeB>::type outerProduct(vecTypeA<T, P> const & c, vecTypeB<T, P> const & r)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'outerProduct' only accept floating-point inputs");
		return detail::compute_outerProduct<vecTypeA, vecTypeB, T, P>::call(c, r);
	}

	template <typename T, precision P, template <typename, precision> class matType>
	GLM_FUNC_QUALIFIER typename matType<T, P>::transpose_type transpose(matType<T, P> const & m)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'transpose' only accept floating-point inputs");
		return detail::compute_transpose<matType, T, P>::call(m);
	}

	template <typename T, precision P, template <typename, precision> class matType>
	GLM_FUNC_QUALIFIER T determinant(matType<T, P> const & m)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'determinant' only accept floating-point inputs");
		return detail::compute_determinant<matType, T, P>::call(m);
	}

	template <typename T, precision P, template <typename, precision> class matType>
	GLM_FUNC_QUALIFIER matType<T, P> inverse(matType<T, P> const & m)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'inverse' only accept floating-point inputs");
		return detail::compute_inverse<matType, T, P>::call(m);
	}

}//namespace glm
