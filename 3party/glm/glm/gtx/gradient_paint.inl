///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2009-03-06
// Updated : 2013-04-09
// Licence : This source is under MIT License
// File    : glm/gtx/gradient_paint.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T radialGradient
	(
		detail::tvec2<T, P> const & Center,
		T const & Radius,
		detail::tvec2<T, P> const & Focal,
		detail::tvec2<T, P> const & Position
	)
	{
		detail::tvec2<T, P> F = Focal - Center;
		detail::tvec2<T, P> D = Position - Focal;
		T Radius2 = pow2(Radius);
		T Fx2 = pow2(F.x);
		T Fy2 = pow2(F.y);

		T Numerator = (D.x * F.x + D.y * F.y) + sqrt(Radius2 * (pow2(D.x) + pow2(D.y)) - pow2(D.x * F.y - D.y * F.x));
		T Denominator = Radius2 - (Fx2 + Fy2);
		return Numerator / Denominator;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T linearGradient
	(
		detail::tvec2<T, P> const & Point0,
		detail::tvec2<T, P> const & Point1,
		detail::tvec2<T, P> const & Position
	)
	{
		detail::tvec2<T, P> Dist = Point1 - Point0;
		return (Dist.x * (Position.x - Point0.x) + Dist.y * (Position.y - Point0.y)) / glm::dot(Dist, Dist);
	}
}//namespace glm
