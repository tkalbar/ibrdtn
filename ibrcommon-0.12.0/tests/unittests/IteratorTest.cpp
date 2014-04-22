/*
 * IteratorTest.cpp
 *
 * Copyright (C) 2013 IBR, TU Braunschweig
 *
 * Written-by: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "IteratorTest.h"
#include <ibrcommon/Iterator.h>
#include <list>
#include <string>

CPPUNIT_TEST_SUITE_REGISTRATION(IteratorTest);

typedef ibrcommon::find_iterator<std::list<std::string>::iterator, std::string> list_find;

void IteratorTest::setUp() {
}

void IteratorTest::tearDown() {
}

void IteratorTest::listTest() {
	std::list<std::string> l;
	l.push_back("0");
	l.push_back("2");
	l.push_back("3");
	l.push_back("0");
	l.push_back("5");
	l.push_back("6");
	l.push_back("0");
	l.push_back("0");

	std::string token("0");

	list_find lf(l.begin(), token);

	int data_count = 0;

	while (lf.next(l.end()))
	{
		std::string &data = (*lf);
		CPPUNIT_ASSERT_EQUAL(token, data);

		l.erase(lf);
		data_count++;
	}

	CPPUNIT_ASSERT_EQUAL(4, data_count);
}
