///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2005-12-21
// Updated : 2007-08-14
// Licence : This source is under MIT License
// File    : glm/gtx/euler_angles.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleX
	(
		T const & angleX
	)
	{
		T cosX = glm::cos(angleX);
		T sinX = glm::sin(angleX);
	
		return detail::tmat4x4<T, defaultp>(
			T(1), T(0), T(0), T(0),
			T(0), cosX, sinX, T(0),
			T(0),-sinX, cosX, T(0),
			T(0), T(0), T(0), T(1));
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleY
	(
		T const & angleY
	)
	{
		T cosY = glm::cos(angleY);
		T sinY = glm::sin(angleY);

		return detail::tmat4x4<T, defaultp>(
			cosY,	T(0),	-sinY,	T(0),
			T(0),	T(1),	T(0),	T(0),
			sinY,	T(0),	cosY,	T(0),
			T(0),	T(0),	T(0),	T(1));
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleZ
	(
		T const & angleZ
	)
	{
		T cosZ = glm::cos(angleZ);
		T sinZ = glm::sin(angleZ);

		return detail::tmat4x4<T, defaultp>(
			cosZ,	sinZ,	T(0), T(0),
			-sinZ,	cosZ,	T(0), T(0),
			T(0),	T(0),	T(1), T(0),
			T(0),	T(0),	T(0), T(1));
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleXY
	(
		T const & angleX,
		T const & angleY
	)
	{
		T cosX = glm::cos(angleX);
		T sinX = glm::sin(angleX);
		T cosY = glm::cos(angleY);
		T sinY = glm::sin(angleY);

		return detail::tmat4x4<T, defaultp>(
			cosY,   -sinX * -sinY,  cosX * -sinY,   T(0),
			T(0),   cosX,           sinX,           T(0),
			sinY,   -sinX * cosY,   cosX * cosY,    T(0),
			T(0),   T(0),           T(0),           T(1));
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleYX
	(
		T const & angleY,
		T const & angleX
	)
	{
		T cosX = glm::cos(angleX);
		T sinX = glm::sin(angleX);
		T cosY = glm::cos(angleY);
		T sinY = glm::sin(angleY);

		return detail::tmat4x4<T, defaultp>(
			cosY,          0,      -sinY,    T(0),
			sinY * sinX,  cosX, cosY * sinX, T(0),
			sinY * cosX, -sinX, cosY * cosX, T(0),
			T(0),         T(0),     T(0),    T(1));
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleXZ
	(
		T const & angleX,
		T const & angleZ
	)
	{
		return eulerAngleX(angleX) * eulerAngleZ(angleZ);
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleZX
	(
		T const & angleZ,
		T const & angleX
	)
	{
		return eulerAngleZ(angleZ) * eulerAngleX(angleX);
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleYZ
	(
		T const & angleY,
		T const & angleZ
	)
	{
		return eulerAngleY(angleY) * eulerAngleZ(angleZ);
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleZY
	(
		T const & angleZ,
		T const & angleY
	)
	{
		return eulerAngleZ(angleZ) * eulerAngleY(angleY);
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> eulerAngleYXZ
	(
		T const & yaw,
		T const & pitch,
		T const & roll
	)
	{
		T tmp_ch = glm::cos(yaw);
		T tmp_sh = glm::sin(yaw);
		T tmp_cp = glm::cos(pitch);
		T tmp_sp = glm::sin(pitch);
		T tmp_cb = glm::cos(roll);
		T tmp_sb = glm::sin(roll);

		detail::tmat4x4<T, defaultp> Result;
		Result[0][0] = tmp_ch * tmp_cb + tmp_sh * tmp_sp * tmp_sb;
		Result[0][1] = tmp_sb * tmp_cp;
		Result[0][2] = -tmp_sh * tmp_cb + tmp_ch * tmp_sp * tmp_sb;
		Result[0][3] = static_cast<T>(0);
		Result[1][0] = -tmp_ch * tmp_sb + tmp_sh * tmp_sp * tmp_cb;
		Result[1][1] = tmp_cb * tmp_cp;
		Result[1][2] = tmp_sb * tmp_sh + tmp_ch * tmp_sp * tmp_cb;
		Result[1][3] = static_cast<T>(0);
		Result[2][0] = tmp_sh * tmp_cp;
		Result[2][1] = -tmp_sp;
		Result[2][2] = tmp_ch * tmp_cp;
		Result[2][3] = static_cast<T>(0);
		Result[3][0] = static_cast<T>(0);
		Result[3][1] = static_cast<T>(0);
		Result[3][2] = static_cast<T>(0);
		Result[3][3] = static_cast<T>(1);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> yawPitchRoll
	(
		T const & yaw,
		T const & pitch,
		T const & roll
	)
	{
		T tmp_ch = glm::cos(yaw);
		T tmp_sh = glm::sin(yaw);
		T tmp_cp = glm::cos(pitch);
		T tmp_sp = glm::sin(pitch);
		T tmp_cb = glm::cos(roll);
		T tmp_sb = glm::sin(roll);

		detail::tmat4x4<T, defaultp> Result;
		Result[0][0] = tmp_ch * tmp_cb + tmp_sh * tmp_sp * tmp_sb;
		Result[0][1] = tmp_sb * tmp_cp;
		Result[0][2] = -tmp_sh * tmp_cb + tmp_ch * tmp_sp * tmp_sb;
		Result[0][3] = static_cast<T>(0);
		Result[1][0] = -tmp_ch * tmp_sb + tmp_sh * tmp_sp * tmp_cb;
		Result[1][1] = tmp_cb * tmp_cp;
		Result[1][2] = tmp_sb * tmp_sh + tmp_ch * tmp_sp * tmp_cb;
		Result[1][3] = static_cast<T>(0);
		Result[2][0] = tmp_sh * tmp_cp;
		Result[2][1] = -tmp_sp;
		Result[2][2] = tmp_ch * tmp_cp;
		Result[2][3] = static_cast<T>(0);
		Result[3][0] = static_cast<T>(0);
		Result[3][1] = static_cast<T>(0);
		Result[3][2] = static_cast<T>(0);
		Result[3][3] = static_cast<T>(1);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat2x2<T, defaultp> orientate2
	(
		T const & angle
	)
	{
		T c = glm::cos(angle);
		T s = glm::sin(angle);

		detail::tmat2x2<T, defaultp> Result;
		Result[0][0] = c;
		Result[0][1] = s;
		Result[1][0] = -s;
		Result[1][1] = c;
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat3x3<T, defaultp> orientate3
	(
		T const & angle
	)
	{
		T c = glm::cos(angle);
		T s = glm::sin(angle);

		detail::tmat3x3<T, defaultp> Result;
		Result[0][0] = c;
		Result[0][1] = s;
		Result[0][2] = 0.0f;
		Result[1][0] = -s;
		Result[1][1] = c;
		Result[1][2] = 0.0f;
		Result[2][0] = 0.0f;
		Result[2][1] = 0.0f;
		Result[2][2] = 1.0f;
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat3x3<T, P> orientate3
	(
		detail::tvec3<T, P> const & angles
	)
	{
		return detail::tmat3x3<T, P>(yawPitchRoll(angles.z, angles.x, angles.y));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> orientate4
	(
		detail::tvec3<T, P> const & angles
	)
	{
		return yawPitchRoll(angles.z, angles.x, angles.y);
	}
}//namespace glm
