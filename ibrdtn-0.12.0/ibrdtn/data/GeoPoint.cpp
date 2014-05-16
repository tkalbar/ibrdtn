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
		 : _scale_factor(1048576), _latitude(0), _longitude(0)
		{
			set(0.0, 0.0);
		}

		GeoPoint::GeoPoint(float lat, float lon)
		 : _scale_factor(1048576)
		{
			// we can't encode negatives as SDNV, so add 360 to any negative coords
			// they're still technically the correct coordinate
			if (lat < 0.0) lat += 360.0;
			if (lon < 0.0) lon += 360.0;
			_latitude = (int)(lat*_scale_factor);
			_longitude = (int)(lon*_scale_factor);
		}

		GeoPoint::~GeoPoint()
		{
		}

		void GeoPoint::set(float lat, float lon)
		{
			if (lat < 0.0) lat += 360.0;
			if (lon < 0.0) lon += 360.0;
			_latitude = (int)(lat*_scale_factor);
			_longitude = (int)(lon*_scale_factor);
		}

		Length GeoPoint::getLength() const
		{
			return _latitude.getLength() + _longitude.getLength();
		}

		float GeoPoint::getLongitude() const
		{
			float lon = _longitude.get()/_scale_factor;
			// represent longitude between -180 to 180 degrees
			if (lon > 180.0) lon -= 360.0;;
			return lon;
		}

		float GeoPoint::getLatitude() const
		{
			float lat = _latitude.get()/_scale_factor;
			// represent longitude between -90 to 90 degrees
			if (lat > 90.0) lat -= 360.0;
			return lat;
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

		const std::string GeoPoint::toString() const
		{
			std::stringstream ss;
			ss << "(" << getLatitude() << " , " << getLongitude() << ")";
			return ss.str();
		}

	}
}
