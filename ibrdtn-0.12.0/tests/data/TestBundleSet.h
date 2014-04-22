/*
 * TestBundleSet.h
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
#include <ibrdtn/data/BundleSet.h>

#ifndef TESTBUNDLESET_H_
#define TESTBUNDLESET_H_

class TestBundleSet : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (TestBundleSet);
	CPPUNIT_TEST (orderTest);
	CPPUNIT_TEST (containTest);
	CPPUNIT_TEST_SUITE_END ();

public:
	void setUp (void);
	void tearDown (void);

protected:
	void orderTest(void);
	void containTest(void);

private:
	class ExpiredBundleCounter : public dtn::data::BundleSet::Listener
	{
	public:
		ExpiredBundleCounter();
		virtual ~ExpiredBundleCounter();

		void eventBundleExpired(const dtn::data::MetaBundle &b) throw ();
		int counter;
	};

	void genbundles(dtn::data::BundleSet &l, int number, int offset, int max);
};

#endif /* TESTBUNDLESET_H_ */
