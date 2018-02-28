#include <pthread.h>
#include <unistd.h>
#include <cassert>
#include <string>
#include <iostream>
#include "cppzmq/zmq.hpp"
#include <boost/program_options.hpp>
#include <netinet/in.h>
#include <proto/protocol.h>

#include "cppzmq/zmq.hpp"

#include "proto/safekiddo.pb.h"
#include "proto/protocol.h"

namespace po = boost::program_options;

inline uint64_t getTime()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts))
	{
		perror("clock_gettime()");
		exit(-1);
	}
	return static_cast<uint64_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
}

void *worker_routine (void *arg)
{
	zmq::context_t *context = static_cast<zmq::context_t *>(arg);

	zmq::socket_t socket(*context, ZMQ_REP);
	socket.connect ("inproc://workers");

	uint64_t totalWait = 0;

//	std::cout << "starting worker thread" << std::endl;
	while (true)
	{
		uint64_t const startTime = getTime();
		//  Wait for next request from client
		zmq::message_t request;
		socket.recv(&request);
		uint64_t const endTime = getTime();
		totalWait += (endTime - startTime);

		protocol::header_t *hdr = static_cast<protocol::header_t*>(request.data());
//		std::cout << "got request: " << *hdr << std::endl;

		uint32_t reqId = ntohl(hdr->type);
		hdr->size = ntohl(hdr->size);
#ifndef NDEBUG
		size_t maxDataSize = 4096;
		assert(hdr->size <= maxDataSize);
#endif

		protocol::messages::Request rq;
		protocol::messages::Response rs;

		rq.ParseFromArray(hdr->data, hdr->size - sizeof(hdr->type));

		if (rq.url().size() % 2 == 0)
			rs.set_result(protocol::messages::Response::RES_OK);
		else
			rs.set_result(protocol::messages::Response::RES_CATEGORY_BLOCKED_DEFAULT);
//
		size_t wr_size = rs.ByteSize();
		assert(wr_size <= maxDataSize);

		zmq::message_t reply(wr_size + sizeof(hdr->size) + sizeof(hdr->type));
		hdr = static_cast<protocol::header_t*>(reply.data());

//
		rs.SerializeWithCachedSizesToArray(hdr->data);
//		wr_size += sizeof(hdr->type);
		hdr->size = htonl(wr_size + sizeof(hdr->type));
		hdr->type = htonl(reqId);

//		std::cout << "sending response: " << *hdr << std::endl;

		//  Send reply back to client
		socket.send(reply);
	}
	return (NULL);
}

int main (int argc, char **argv)
{
	po::options_description desc("Command line options");
	size_t thread_num = 0;
	size_t io_num = 0;
	std::string serverUrl;

	desc.add_options()
		("help,h", "Produce help message")
		("threads,t", boost::program_options::value<size_t>(&thread_num)->default_value(16),
				"Number of worker threads")
		("io,i", boost::program_options::value<size_t>(&io_num)->default_value(1),
				"Number of io threads")
		("address,a", boost::program_options::value<std::string>(&serverUrl)->default_value("tcp://*:7777"),
				"Server URL")
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
	zmq::socket_t clients (context, ZMQ_ROUTER);
	int hwm = 1000000;
	clients.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
	clients.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
	clients.bind(serverUrl.c_str());
	zmq::socket_t workers (context, ZMQ_DEALER);
	workers.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
	workers.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
	workers.bind("inproc://workers");

	//  Launch pool of worker threads
	for (int thread_nbr = 0; thread_nbr < (int)thread_num; thread_nbr++) {
		pthread_t worker;
		pthread_create (&worker, NULL, worker_routine, (void *) &context);
	}
	//  Connect work threads to client threads via a queue
	zmq::proxy (clients, workers, NULL);
	return 0;
}
