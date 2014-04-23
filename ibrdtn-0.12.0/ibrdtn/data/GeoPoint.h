/*
 * GeoPoint.h
 */

#ifndef GEOPOINT_H_
#define GEOPOINT_H_

#include "ibrdtn/data/Number.h"
#include <stdint.h>

namespace dtn
{
	namespace data
	{
		class GeoPoint
		{
		public:
			GeoPoint();
			GeoPoint(float lat, float lon = 0);
			virtual ~GeoPoint();

			float getLongitude();
			float getLatitude();

			/**
			 * set the GeoPoint coords
			 */
			void set(float lat, float lon);

			Length getLength() const;

		private:
			friend std::ostream &operator<<(std::ostream &stream, const dtn::data::GeoPoint &obj);
			friend std::istream &operator>>(std::istream &stream, dtn::data::GeoPoint &obj);

			Number _latitude;
			Number _longitude;

			// we can't send floats as SDNV, so need to scale the gps coords up and encode as integers
			float _scale_factor;
		};
	}
}


#endif /* GEOPOINT_H_ */
