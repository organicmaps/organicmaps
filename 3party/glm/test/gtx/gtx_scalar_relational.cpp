///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2013-02-04
// Updated : 2013-02-04
// Licence : This source is under MIT licence
// File    : test/gtx/gtx_scalar_relational.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/scalar_relational.hpp>
#include <cstdio>

int test_lessThan()
{
	int Error(0);

	Error += glm::lessThan(0, 1) ? 0 : 1;
	Error += glm::lessThan(1, 0) ? 1 : 0;
	Error += glm::lessThan(0, 0) ? 1 : 0;
	Error += glm::lessThan(1, 1) ? 1 : 0;
	Error += glm::lessThan(0.0f, 1.0f) ? 0 : 1;
	Error += glm::lessThan(1.0f, 0.0f) ? 1 : 0;
	Error += glm::lessThan(0.0f, 0.0f) ? 1 : 0;
	Error += glm::lessThan(1.0f, 1.0f) ? 1 : 0;
	Error += glm::lessThan(0.0, 1.0) ? 0 : 1;
	Error += glm::lessThan(1.0, 0.0) ? 1 : 0;
	Error += glm::lessThan(0.0, 0.0) ? 1 : 0;
	Error += glm::lessThan(1.0, 1.0) ? 1 : 0;

	return Error;
}

int test_lessThanEqual()
{
	int Error(0);

	Error += glm::lessThanEqual(0, 1) ? 0 : 1;
	Error += glm::lessThanEqual(1, 0) ? 1 : 0;
	Error += glm::lessThanEqual(0, 0) ? 0 : 1;
	Error += glm::lessThanEqual(1, 1) ? 0 : 1;
	Error += glm::lessThanEqual(0.0f, 1.0f) ? 0 : 1;
	Error += glm::lessThanEqual(1.0f, 0.0f) ? 1 : 0;
	Error += glm::lessThanEqual(0.0f, 0.0f) ? 0 : 1;
	Error += glm::lessThanEqual(1.0f, 1.0f) ? 0 : 1;
	Error += glm::lessThanEqual(0.0, 1.0) ? 0 : 1;
	Error += glm::lessThanEqual(1.0, 0.0) ? 1 : 0;
	Error += glm::lessThanEqual(0.0, 0.0) ? 0 : 1;
	Error += glm::lessThanEqual(1.0, 1.0) ? 0 : 1;

	return Error;
}

int test_greaterThan()
{
	int Error(0);

	Error += glm::greaterThan(0, 1) ? 1 : 0;
	Error += glm::greaterThan(1, 0) ? 0 : 1;
	Error += glm::greaterThan(0, 0) ? 1 : 0;
	Error += glm::greaterThan(1, 1) ? 1 : 0;
	Error += glm::greaterThan(0.0f, 1.0f) ? 1 : 0;
	Error += glm::greaterThan(1.0f, 0.0f) ? 0 : 1;
	Error += glm::greaterThan(0.0f, 0.0f) ? 1 : 0;
	Error += glm::greaterThan(1.0f, 1.0f) ? 1 : 0;
	Error += glm::greaterThan(0.0, 1.0) ? 1 : 0;
	Error += glm::greaterThan(1.0, 0.0) ? 0 : 1;
	Error += glm::greaterThan(0.0, 0.0) ? 1 : 0;
	Error += glm::greaterThan(1.0, 1.0) ? 1 : 0;

	return Error;
}

int test_greaterThanEqual()
{
	int Error(0);

	Error += glm::greaterThanEqual(0, 1) ? 1 : 0;
	Error += glm::greaterThanEqual(1, 0) ? 0 : 1;
	Error += glm::greaterThanEqual(0, 0) ? 0 : 1;
	Error += glm::greaterThanEqual(1, 1) ? 0 : 1;
	Error += glm::greaterThanEqual(0.0f, 1.0f) ? 1 : 0;
	Error += glm::greaterThanEqual(1.0f, 0.0f) ? 0 : 1;
	Error += glm::greaterThanEqual(0.0f, 0.0f) ? 0 : 1;
	Error += glm::greaterThanEqual(1.0f, 1.0f) ? 0 : 1;
	Error += glm::greaterThanEqual(0.0, 1.0) ? 1 : 0;
	Error += glm::greaterThanEqual(1.0, 0.0) ? 0 : 1;
	Error += glm::greaterThanEqual(0.0, 0.0) ? 0 : 1;
	Error += glm::greaterThanEqual(1.0, 1.0) ? 0 : 1;

	return Error;
}

int test_equal()
{
	int Error(0);

	Error += glm::equal(0, 1) ? 1 : 0;
	Error += glm::equal(1, 0) ? 1 : 0;
	Error += glm::equal(0, 0) ? 0 : 1;
	Error += glm::equal(1, 1) ? 0 : 1;
	Error += glm::equal(0.0f, 1.0f) ? 1 : 0;
	Error += glm::equal(1.0f, 0.0f) ? 1 : 0;
	Error += glm::equal(0.0f, 0.0f) ? 0 : 1;
	Error += glm::equal(1.0f, 1.0f) ? 0 : 1;
	Error += glm::equal(0.0, 1.0) ? 1 : 0;
	Error += glm::equal(1.0, 0.0) ? 1 : 0;
	Error += glm::equal(0.0, 0.0) ? 0 : 1;
	Error += glm::equal(1.0, 1.0) ? 0 : 1;

	return Error;
}

int test_notEqual()
{
	int Error(0);

	Error += glm::notEqual(0, 1) ? 0 : 1;
	Error += glm::notEqual(1, 0) ? 0 : 1;
	Error += glm::notEqual(0, 0) ? 1 : 0;
	Error += glm::notEqual(1, 1) ? 1 : 0;
	Error += glm::notEqual(0.0f, 1.0f) ? 0 : 1;
	Error += glm::notEqual(1.0f, 0.0f) ? 0 : 1;
	Error += glm::notEqual(0.0f, 0.0f) ? 1 : 0;
	Error += glm::notEqual(1.0f, 1.0f) ? 1 : 0;
	Error += glm::notEqual(0.0, 1.0) ? 0 : 1;
	Error += glm::notEqual(1.0, 0.0) ? 0 : 1;
	Error += glm::notEqual(0.0, 0.0) ? 1 : 0;
	Error += glm::notEqual(1.0, 1.0) ? 1 : 0;

	return Error;
}

int test_any()
{
	int Error(0);

	Error += glm::any(true) ? 0 : 1;
	Error += glm::any(false) ? 1 : 0;

	return Error;
}

int test_all()
{
	int Error(0);

	Error += glm::all(true) ? 0 : 1;
	Error += glm::all(false) ? 1 : 0;

	return Error;
}

int test_not()
{
	int Error(0);

	Error += glm::not_(true) ? 1 : 0;
	Error += glm::not_(false) ? 0 : 1;

	return Error;
}

int main()
{
	int Error = 0;

	Error += test_lessThan();
	Error += test_lessThanEqual();
	Error += test_greaterThan();
	Error += test_greaterThanEqual();
	Error += test_equal();
	Error += test_notEqual();
	Error += test_any();
	Error += test_all();
	Error += test_not();

	return Error;
}
