/*
 * Base64StreamTest.h
 *
 * Copyright (C) 2011 IBR, TU Braunschweig
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

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifndef BASE64STREAMTEST_H_
#define BASE64STREAMTEST_H_

class Base64StreamTest : public CppUnit::TestFixture {
	private:
		void compare(std::istream &s1, std::istream &s2);

	public:
		/*=== BEGIN tests for class 'Base64StreamTest' ===*/
		void testEncode();
		void testDecode();
		void testReader();
		void testFileReference();
		/*=== END   tests for class 'Base64StreamTest' ===*/

		void setUp();
		void tearDown();


		CPPUNIT_TEST_SUITE(Base64StreamTest);
		CPPUNIT_TEST(testEncode);
		CPPUNIT_TEST(testDecode);
		CPPUNIT_TEST(testReader);
		CPPUNIT_TEST(testFileReference);
		CPPUNIT_TEST_SUITE_END();
};

#endif /* BASE64STREAMTEST_H_ */
