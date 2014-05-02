/*
 * dtnrecv.cpp
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

#include "config.h"
#include <ibrdtn/api/Client.h>
#include <ibrcommon/net/socket.h>
#include <ibrcommon/net/socketstream.h>
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/thread/SignalHandler.h>
#include <ibrcommon/Logger.h>
#include <ibrdtn/data/GeoRoutingBlock.h>
#include <ibrdtn/data/TrackingBlock.h>
#include <ibrdtn/data/BundleString.h>


#include <sys/types.h>
#include <iostream>

void print_help()
{
	cout << "-- dtnrecv (IBR-DTN) --" << endl;
	cout << "Syntax: dtnrecv [options]"  << endl << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help        Display this text" << endl;
	cout << " --file <path>    Write the incoming data to the a file instead of the" << endl;
	cout << "                  standard output" << endl;
	cout << " --name <name>    Set the application name (e.g. filetransfer)" << endl;
	cout << " --timeout <seconds>" << endl;
	cout << "                  Receive timeout in seconds" << endl;
	cout << " --count <n>      Receive that many bundles" << endl;
	cout << " --group <group>  Join a group" << endl;
	cout << " -U <socket>      Connect to UNIX domain socket API" << endl;
}

dtn::api::Client *_client = NULL;
ibrcommon::socketstream *_conn = NULL;

int h = 0;
bool _stdout = true;

void term(int signal)
{
	if (!_stdout)
	{
		std::cout << h << " bundles received." << std::endl;
	}

	if (signal >= 1)
	{
		if (_client != NULL)
		{
			_client->close();
			_conn->close();
		}
	}
}

int main(int argc, char *argv[])
{
	// logging options
	const unsigned char logopts = ibrcommon::Logger::LOG_DATETIME | ibrcommon::Logger::LOG_LEVEL;

	// error filter
	unsigned char loglevel = 0;

	// create signal handler
	ibrcommon::SignalHandler sighandler(term);
	sighandler.handle(SIGINT);
	sighandler.handle(SIGTERM);
	sighandler.initialize();

	int ret = EXIT_SUCCESS;
	string filename = "";
	string name = "filetransfer";
	dtn::data::EID group;
	int timeout = 0;
	int count   = 1;
	ibrcommon::File unixdomain;

	for (int i = 0; i < argc; ++i)
	{
		string arg = argv[i];

		// print help if requested
		if (arg == "-h" || arg == "--help")
		{
			print_help();
			return ret;
		}

		if (arg == "--logging")
		{
			loglevel |= ibrcommon::Logger::LOGGER_ALL ^ ibrcommon::Logger::LOGGER_DEBUG;
		}

		if (arg == "--debug")
		{
			loglevel |= ibrcommon::Logger::LOGGER_DEBUG;
		}

		if (arg == "--name" && argc > i)
		{
			name = argv[i + 1];
		}

		if (arg == "--file" && argc > i)
		{
			filename = argv[i + 1];
			_stdout = false;
		}

		if (arg == "--timeout" && argc > i)
		{
			timeout = atoi(argv[i + 1]);
		}

		if (arg == "--group" && argc > i)
		{
			group = std::string(argv[i + 1]);
		}

		if (arg == "--count" && argc > i) 
		{
			count = atoi(argv[i + 1]);
		}

		if (arg == "-U" && argc > i)
		{
			if (++i > argc)
			{
				std::cout << "argument missing!" << std::endl;
				return -1;
			}

			unixdomain = ibrcommon::File(argv[i]);
		}
	}

	if (loglevel > 0)
	{
		// add logging to the cerr
		ibrcommon::Logger::addStream(std::cerr, loglevel, logopts);
	}

	try {
		// Create a stream to the server using TCP.
		ibrcommon::clientsocket *sock = NULL;

		// check if the unixdomain socket exists
		if (unixdomain.exists())
		{
			// connect to the unix domain socket
			sock = new ibrcommon::filesocket(unixdomain);
		}
		else
		{
			// connect to the standard local api port
			ibrcommon::vaddress addr("localhost", 4550);
			sock = new ibrcommon::tcpsocket(addr);
		}

    	ibrcommon::socketstream conn(sock);

		// Initiate a client for synchronous receiving
		dtn::api::Client client(name, group, conn);

		// export objects for the signal handler
		_conn = &conn;
		_client = &client;

		// Connect to the server. Actually, this function initiate the
		// stream protocol by starting the thread and sending the contact header.
		client.connect();

		std::fstream file;

		if (!_stdout)
		{
			std::cout << "Wait for incoming bundle... " << std::endl;
			file.open(filename.c_str(), ios::in|ios::out|ios::binary|ios::trunc);
			file.exceptions(std::ios::badbit | std::ios::eofbit);
		}

		for(h = 0; h < count; ++h)
		{
			// receive the bundle
			dtn::data::Bundle b = client.getBundle(timeout);

			// get the reference to the blob
			ibrcommon::BLOB::Reference ref = b.find<dtn::data::PayloadBlock>().getBLOB();

            try {
                dtn::data::GeoRoutingBlock gb = b.find<dtn::data::GeoRoutingBlock>();
                std::cout << "Received geo routing block:"<< endl;
                for(dtn::data::GeoRoutingBlock::tracking_list::const_iterator i = gb.getRoute().begin();i!=gb.getRoute().end();i++) {

                    if (i->getFlag(GeoRoutingBlock::GeoRoutingEntry::EID_REQUIRED)) {
                        dtn::data::BundleString endpoint(i->eid.getString());
                        std::cout << endpoint << endl;
                    }
                    if (i->getFlag(GeoRoutingBlock::GeoRoutingEntry::GEO_REQUIRED)) {
                        std::cout << i->geopoint.getLatitude() << " " << i->geopoint.getLongitude() << endl;
                    }
                                                
                }
            } catch(const dtn::data::Bundle::NoSuchBlockFoundException &) {
            }
            try {
                dtn::data::TrackingBlock tb = b.find<dtn::data::TrackingBlock>();
                std::cout << "Received tracking block:" << endl;
                for(dtn::data::TrackingBlock::tracking_list::const_iterator i = tb.getTrack().begin();i!=tb.getTrack().end();i++) {
                    if (i->entry_type == TrackingBlock::TrackingEntry::HOPDATA) {
                        dtn::data::BundleString endpoint(i->endpoint.getString());
                        std::cout << endpoint << endl;
                    }
                    if (i->entry_type == TrackingBlock::TrackingEntry::GEODATA) {
                        std::cout << i->geopoint.getLatitude() << " " << i->geopoint.getLongitude() << endl;
                    }
                 
                }
            } catch(const dtn::data::Bundle::NoSuchBlockFoundException&) {
            }

			// write the data to output
			if (_stdout)
			{
				std::cout << ref.iostream()->rdbuf() << std::flush;
			}
			else
			{
				// write data to temporary file
				try {
					std::cout << "Bundle received (" << (h + 1) << ")." << endl;

					file << ref.iostream()->rdbuf();
				} catch (const ios_base::failure&) {

				}
			}
		}

		if (!_stdout)
		{
			file.close();
			std::cout << "done." << std::endl;
		}

		// Shutdown the client connection.
		client.close();

		// close the tcp connection
		conn.close();
	} catch (const dtn::api::ConnectionTimeoutException&) {
		std::cerr << "Timeout." << std::endl;
		ret = EXIT_FAILURE;
	} catch (const dtn::api::ConnectionAbortedException&) {
		std::cerr << "Aborted." << std::endl;
		ret = EXIT_FAILURE;
	} catch (const dtn::api::ConnectionException&) {
	} catch (const std::exception &ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
		ret = EXIT_FAILURE;
	}

	return ret;
}
