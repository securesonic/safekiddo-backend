/*
 * direct.cpp
 *
 *  Created on: Feb 1, 2014
 *      Author: zytka
 */

#include <pthread.h>
#include <unistd.h>
#include <cassert>
#include <string>
#include <iostream>
#include "cppzmq/zmq.hpp"
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <netinet/in.h>
#include <proto/protocol.h>

#include "cppzmq/zmq.hpp"

#include "proto/safekiddo.pb.h"

namespace po = boost::program_options;

void worker_routine (void *arg, std::string url, size_t port)
{
	zmq::context_t *context = static_cast<zmq::context_t *>(arg);

	zmq::socket_t socket(*context, ZMQ_REP);
	std::ostringstream os;
	os << url << ":" << port;
	std::string u = os.str();
	std::cout << u << std::endl;
	socket.bind (u.c_str());

	std::cout << "starting worker thread" << std::endl;
	while (true)
	{
		//  Wait for next request from client
		zmq::message_t request;
		socket.recv(&request);

		protocol::header_t *hdr = static_cast<protocol::header_t*>(request.data());
//		std::cout << "got request: " << *hdr << std::endl;

		hdr->size = ntohl(hdr->size);
#ifndef NDEBUG
		size_t maxDataSize = 4096;
		assert(hdr->size <= maxDataSize);
#endif

		protocol::messages::Request rq;
		protocol::messages::Response rs;

		rq.ParseFromArray(hdr->data, hdr->size - sizeof(hdr->type));
//
		rs.set_result(protocol::messages::Response::RES_OK);
//
		size_t wr_size = rs.ByteSize();
		assert(wr_size <= maxDataSize);

		zmq::message_t reply(wr_size + sizeof(hdr->size) + sizeof(hdr->type));
		hdr = static_cast<protocol::header_t*>(reply.data());

//
		rs.SerializeWithCachedSizesToArray(hdr->data);
//		wr_size += sizeof(hdr->type);
		hdr->size = htonl(wr_size + sizeof(hdr->type));
		hdr->type = htonl(protocol::MSG_RESPONSE);

//		std::cout << "sending response: " << *hdr << std::endl;

		//  Send reply back to client
		socket.send(reply);
	}
}

int main (int argc, char **argv)
{
	po::options_description desc("Command line options");
	size_t thread_num = 0;
	size_t io_num = 0;
	size_t basePort = 0;
	std::string serverUrl;

	desc.add_options()
		("help,h", "Produce help message")
		("threads,t", boost::program_options::value<size_t>(&thread_num)->default_value(16),
				"Number of worker threads")
		("io,i", boost::program_options::value<size_t>(&io_num)->default_value(1),
				"Number of io threads")
		("address,a", boost::program_options::value<std::string>(&serverUrl)->default_value("tcp://127.0.0.1"),
				"Server URL")
		("base,b", boost::program_options::value<size_t>(&basePort)->default_value(12345),
				"Base port")
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cerr << desc << std::endl;
		return 1;
	}

	//  Prepare our context and sockets
	zmq::context_t context (io_num);
//	zmq::socket_t clients (context, ZMQ_ROUTER);
//	clients.bind(serverUrl.c_str());
//	zmq::socket_t workers (context, ZMQ_DEALER);
//	workers.bind("inproc://workers");
	std::vector<boost::shared_ptr<boost::thread> > threads;

	//  Launch pool of worker threads
	for (int thread_nbr = 0; thread_nbr < (int)thread_num; thread_nbr++) {
		threads.push_back(boost::shared_ptr<boost::thread>(
			new boost::thread(worker_routine, &context, serverUrl, basePort + thread_nbr))
		);
	}
	for (size_t i = 0; i < threads.size(); ++i)
		threads[i]->join();
	//  Connect work threads to client threads via a queue
//	zmq::proxy (clients, workers, NULL);
	return 0;
}



