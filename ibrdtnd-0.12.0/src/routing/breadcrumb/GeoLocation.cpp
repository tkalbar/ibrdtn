/*
 * AcknowledgementSet.cpp
 *
 *  Created on: 11.01.2013
 *      Author: morgenro
 */

#include "routing/breadcrumb/GeoLocation.h"
#include "core/BundleCore.h"
#include <ibrdtn/utils/Clock.h>

namespace dtn
{
	namespace routing
	{
		const dtn::data::Number GeoLocation::identifier = NodeHandshakeItem::GEO_LOCATION;

		GeoLocation::GeoLocation() : NeighborDataSetImpl(GeoLocation::identifier) {}

		GeoLocation::GeoLocation(const GeoLocation &other)
		 : NeighborDataSetImpl(GeoLocation::identifier), ibrcommon::Mutex(), _geopoint(other._geopoint){}

		GeoLocation::GeoLocation(float latitude, float longitude)
		 : NeighborDataSetImpl(GeoLocation::identifier)
		{
			_geopoint.set(latitude, longitude);
		}

		GeoLocation::~GeoLocation() {}

		/*void AcknowledgementSet::add(const dtn::data::MetaBundle &bundle) throw ()
		{
			try {
				// check if the bundle is valid
				dtn::core::BundleCore::getInstance().validate(bundle);

				// add the bundle to the set
				_bundles.add(bundle);
			} catch (dtn::data::Validator::RejectedException &ex) {
				// bundle rejected
			}
		}*/

		/*void AcknowledgementSet::expire(const dtn::data::Timestamp &timestamp) throw ()
		{
			_bundles.expire(timestamp);
		}*/

		/*bool AcknowledgementSet::has(const dtn::data::BundleID &id) const throw ()
		{
			dtn::data::BundleList::const_iterator iter = _bundles.find(dtn::data::MetaBundle::create(id));
			return !(iter == _bundles.end());
		}*/

		const dtn::data::Number& GeoLocation::getIdentifier() const
		{
			return identifier;
		}

		dtn::data::Length GeoLocation::getLength() const
		{
			std::stringstream ss;
			serialize(ss);
			return ss.str().length();
		}

		std::ostream& GeoLocation::serialize(std::ostream& stream) const
		{
			stream << (*this)._geopoint;
			return stream;
		}

		std::istream& GeoLocation::deserialize(std::istream& stream)
		{
			stream >> (*this)._geopoint;
			return stream;
		}

		void GeoLocation::toString(std::ostream &stream) const
		{
			std::ostringstream latStrs;
			float tmp = _geopoint.getLatitude();
			latStrs << tmp;
			std::string latStr = latStrs.str();

			std::ostringstream longStrs;
			tmp = _geopoint.getLongitude();
			longStrs << tmp;
			std::string longStr = longStrs.str();

			stream << "("+latStr+","+longStr+")";
		}

		// use this operator for outputting to string
		std::ostream& operator<<(std::ostream& stream, const GeoLocation &geo_loc)
		{
			geo_loc.toString(stream);
			return stream;
		}

	} /* namespace routing */
} /* namespace dtn */
