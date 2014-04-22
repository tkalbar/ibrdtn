/*
 * GeoPoint.cpp
 *
 * represent lat and lon as integers so we can use SDNVs.
 * Represent them by multiplying by 1,000,000
 * The SDNV-based Float type is broken.
 *
 */

#include "ibrdtn/data/GeoPoint.h"
#include "ibrdtn/utils/Clock.h"
#include <sys/time.h>

namespace dtn
{
	namespace data
	{
		GeoPoint::GeoPoint()
		 : _latitude(0), _longitude(0)
		{
			set(0.0, 0.0);
		}

		GeoPoint::GeoPoint(float lat, float lon)
		 : _latitude(lat*1000000.0), _longitude(lon*1000000.0)
		{
		}

		GeoPoint::~GeoPoint()
		{
		}

		void GeoPoint::set(float lat, float lon)
		{
			_latitude = lat*1000000.0;
			_longitude = lon*1000000.0;
		}

		Length GeoPoint::getLength() const
		{
			return _latitude.getLength() + _longitude.getLength();
		}

		float GeoPoint::getLongitude()
		{
			float lon = _longitude.get();
			return lon/1000000.0;
		}

		float GeoPoint::getLatitude()
		{
			float lat = _latitude.get();
			return lat/1000000.0;
		}

		std::ostream& operator<<(std::ostream &stream, const dtn::data::GeoPoint &obj)
		{
			stream << obj._latitude << obj._longitude;
			return stream;
		}

		std::istream& operator>>(std::istream &stream, dtn::data::GeoPoint &obj)
		{
			stream >> obj._latitude;
			stream >> obj._longitude;
			return stream;
		}
	}
}
