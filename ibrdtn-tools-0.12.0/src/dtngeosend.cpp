/*
 * dtngeosend.cpp
 *
 */

#include "config.h"
#include <ibrdtn/api/Client.h>
#include <ibrdtn/data/GeoRoutingBlock.h>
#include <ibrcommon/net/socket.h>
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/Logger.h>

#include <iostream>

void print_help()
{
	cout << "-- dtngeosend (IBR-DTN) --" << endl;
	cout << "Syntax: dtngeosend [options] <dst> <filename>"  << endl;
	cout << " <dst>            Set the destination eid (e.g. dtn://node/filetransfer)" << endl;
	cout << " <filename>       The file to transfer" << endl << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help        Display this text" << endl;
	cout << " --src <name>     Set the source application name (e.g. filetransfer)" << endl;
	cout << " -p <0..2>        Set the bundle priority (0 = low, 1 = normal, 2 = high)" << endl;
	cout << " -g               Receiver is a destination group" << endl;
	cout << " --lifetime <seconds>" << endl;
	cout << "                  Set the lifetime of outgoing bundles; default: 3600" << endl;
	cout << " -U <socket>      Connect to UNIX domain socket API" << endl;
	cout << " -n <copies>      Create <copies> bundle copies" << endl;
	cout << " --encrypt        Request encryption on the bundle layer" << endl;
	cout << " --sign           Request signature on the bundle layer" << endl;
	cout << " --custody        Request custody transfer of the bundle" << endl;
	cout << " --compression    Request compression of the payload" << endl;

}

int main(int argc, char *argv[])
{
	bool error = false;
	string file_destination = "dtn://local/filetransfer";
	string file_source = "";
	unsigned int lifetime = 3600;
	bool use_stdin = false;
	std::string filename;
	ibrcommon::File unixdomain;
	int priority = 1;
	int copies = 1;
	bool bundle_encryption = false;
	bool bundle_signed = false;
	bool bundle_custody = false;
	bool bundle_compression = false;
	bool bundle_group = false;

//	ibrcommon::Logger::setVerbosity(99);
//	ibrcommon::Logger::addStream(std::cout, ibrcommon::Logger::LOGGER_ALL, ibrcommon::Logger::LOG_DATETIME | ibrcommon::Logger::LOG_LEVEL);

	std::list<std::string> arglist;

	for (int i = 0; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			std::string arg = argv[i];

			// print help if requested
			if (arg == "-h" || arg == "--help")
			{
				print_help();
				return 0;
			}
			else if (arg == "--encrypt")
			{
				bundle_encryption = true;
			}
			else if (arg == "--sign")
			{
				bundle_signed = true;
			}
			else if (arg == "--custody")
			{
				bundle_custody = true;
			}
			else if (arg == "--compression")
			{
				bundle_compression = true;
			}
			else if (arg == "--src" && argc > i)
			{
				if (++i > argc)
				{
					std::cout << "argument missing!" << std::endl;
					return -1;
				}

				file_source = argv[i];
			}
			else if (arg == "--lifetime" && argc > i)
			{
				if (++i > argc)
				{
					std::cout << "argument missing!" << std::endl;
					return -1;
				}

				stringstream data; data << argv[i];
				data >> lifetime;
			}
			else if (arg == "-p" && argc > i)
			{
				if (++i > argc)
				{
					std::cout << "argument missing!" << std::endl;
					return -1;
				}
				stringstream data; data << argv[i];
				data >> priority;
			}
			else if (arg == "-U" && argc > i)
			{
				if (++i > argc)
				{
					std::cout << "argument missing!" << std::endl;
					return -1;
				}

				unixdomain = ibrcommon::File(argv[i]);
			}
			else if (arg == "-n" && argc > i)
			{
				if (++i > argc)
				{
					std::cout << "argument missing!" << std::endl;
					return -1;
				}

				stringstream data; data << argv[i];
				data >> copies;

				if( copies < 1 ) {
					std::cout << "invalid number of bundle copies!" << std::endl;
					return -1;
				}
			}
			else if (arg == "-g")
			{
				bundle_group = true;
			}
			else
			{
				std::cout << "invalid argument " << arg << std::endl;
				return -1;
			}
		}
		else
		{
			arglist.push_back(argv[i]);
		}
	}

	if (arglist.size() <= 1)
	{
		print_help();
		return -1;
	} else if (arglist.size() == 2)
	{
		std::list<std::string>::iterator iter = arglist.begin(); ++iter;

		// the first parameter is the destination
		file_destination = (*iter);

		use_stdin = true;
	}
	else if (arglist.size() > 2)
	{
		std::list<std::string>::iterator iter = arglist.begin(); ++iter;

		// the first parameter is the destination
		file_destination = (*iter); ++iter;

		// the second parameter is the filename
		filename = (*iter);
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
			ibrcommon::vaddress addr("localhost", 4550);

			// connect to the standard local api port
			sock = new ibrcommon::tcpsocket(addr);
		}

		ibrcommon::socketstream conn(sock);

		try {
			// Initiate a client for synchronous receiving
			dtn::api::Client client(file_source, conn, dtn::api::Client::MODE_SENDONLY);

			// Connect to the server. Actually, this function initiate the
			// stream protocol by starting the thread and sending the contact header.
			client.connect();

			// target address
			EID addr = EID(file_destination);

			try {
				if (use_stdin)
				{
					cout << "Transfer stdin to " << addr.getString() << endl;

					// create an empty BLOB
					ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();

					// copy cin to a BLOB
					(*ref.iostream()) << cin.rdbuf();

					for(int u=0; u<copies; ++u){
						dtn::data::Bundle b;

						// set the destination
						b.destination = file_destination;

						// add payload block with the reference
						b.push_back(ref);

						// set destination address to non-singleton
						if (bundle_group) b.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, false);

						// enable encryption if requested
						if (bundle_encryption) b.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_ENCRYPT, true);

						// enable signature if requested
						if (bundle_signed) b.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_SIGN, true);

						// enable custody transfer if requested
						if (bundle_custody) {
							b.set(dtn::data::PrimaryBlock::CUSTODY_REQUESTED, true);
							b.custodian = dtn::data::EID("api:me");
						}

						// enable compression
						if (bundle_compression) b.set(dtn::data::PrimaryBlock::IBRDTN_REQUEST_COMPRESSION, true);

						// set the lifetime
						b.lifetime = lifetime;

						// set the bundles priority
						b.setPriority(dtn::data::PrimaryBlock::PRIORITY(priority));

						// put the GeoRouting block on
						// for now everything is hard-coded
						dtn::data::GeoRoutingBlock &grb = b.push_back<dtn::data::GeoRoutingBlock>();
						grb.append(22.2, 33.3, 256);
						grb.append(11.1, 44.4, 256);
						grb.append(12.3, 45.6, 256);

						// send the bundle
						client << b;

						if (copies > 1)
						{
							std::cout << "sent copy #" << (u+1) << std::endl;
						}
					}
				}
				else
				{
					cout << "Transfer file \"" << filename << "\" to " << addr.getString() << endl;

					// open file as read-only BLOB
					ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::open(filename);

					for(int u=0; u<copies; ++u){
						// create a bundle from the file
						dtn::data::Bundle b;

						// set the destination
						b.destination = file_destination;

						// add payload block with the reference
						b.push_back(ref);

						// set destination address to non-singleton
						if (bundle_group) b.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, false);

						// enable encryption if requested
						if (bundle_encryption) b.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_ENCRYPT, true);

						// enable signature if requested
						if (bundle_signed) b.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_SIGN, true);

						// enable custody transfer if requested
						if (bundle_custody) {
							b.set(dtn::data::PrimaryBlock::CUSTODY_REQUESTED, true);
							b.custodian = dtn::data::EID("api:me");
						}

						// enable compression
						if (bundle_compression) b.set(dtn::data::PrimaryBlock::IBRDTN_REQUEST_COMPRESSION, true);

						// set the lifetime
						b.lifetime = lifetime;

						// set the bundles priority
						b.setPriority(dtn::data::PrimaryBlock::PRIORITY(priority));

						// put the GeoRouting block on
						// for now everything is hard-coded
						dtn::data::GeoRoutingBlock &grb = b.push_back<dtn::data::GeoRoutingBlock>();
						grb.append(22.2, 33.3, 256);
						grb.append(11.1, 44.4, 256);
						grb.append(12.3, 45.6, 256);

						// send the bundle
						client << b;

						if (copies > 1)
						{
							std::cout << "sent copy #" << (u+1) << std::endl;
						}
					}
				}

				// flush the buffers
				client.flush();
			} catch (const ibrcommon::IOException &ex) {
				std::cerr << "Error while sending bundle." << std::endl;
				std::cerr << "\t" << ex.what() << std::endl;
				error = true;
			}

			// Shutdown the client connection.
			client.close();

		} catch (const ibrcommon::IOException &ex) {
			cout << "Error: " << ex.what() << endl;
			error = true;
		} catch (const dtn::api::ConnectionException&) {
			// connection already closed, the daemon was faster
		}

		// close the tcpstream
		conn.close();
	} catch (const std::exception &ex) {
		cout << "Error: " << ex.what() << endl;
		error = true;
	}

	if (error) return -1;

	return 0;
}
