#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include <google/protobuf/message.h>


#include "tcpSocket.h"
#include "proto/safekiddo.pb.h"

using utils::TcpSocket;
using utils::IpAddr;

inline uint64_t getTimeUs()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts))
	{
		perror("clock_gettime()");
		exit(-1);
	}
	return static_cast<uint64_t>(ts.tv_sec) * 1000000 + ts.tv_nsec/1000;
}

struct LatencyStats {
	uint64_t avg,
			worst,
			best;
};

void thread_func(IpAddr ip, size_t queries, LatencyStats &latencyStats)
{
	TcpSocket sock;
	if (sock.connect(ip))
	{
		std::cerr << "Cannot connect to server\n";
		return;
	}

	sock.setNodelay();

	std::vector<char> buf(4096);

	uint64_t latencySum = 0, worst = 0, best = -1;

	for (size_t i = 0; i < queries; ++i)
	{

		std::string const request = (i % 2
			? "get / http/1.1\r\nRequest: siatka.org\r\n\r\n"
			: "get / http/1.1\r\nRequest: onet.pl\r\n\r\n");
		uint64_t reqStart = getTimeUs();

		sock.writeAll(request.data(), request.size());
		ssize_t ret = sock.read(buf.data(), buf.size());
		if (ret <= 0) {
			perror("sock.read()");
			abort();
		}
		assert(ret >= 4);
		buf[ret] = 0;
		if (memcmp(buf.data()+ret-4, "\r\n\r\n", 4)) {
			std::cerr << "Invalid response: " << std::endl << buf.data() << std::endl;
			for (uint32_t i = 0; i < 4; i++)
			{
				uint32_t c = buf[ret -4 + i];
				std::cerr <<  i << ": " << c << std::endl;
			}
			abort();
		}

		uint64_t reqLatency = getTimeUs() - reqStart;

		if (worst < reqLatency)
			worst = reqLatency;

		if (best > reqLatency)
			best = reqLatency;

		latencySum += reqLatency;
	}

	latencyStats.avg = latencySum/queries;
	latencyStats.best = best;
	latencyStats.worst = worst;
}


namespace po = boost::program_options;


int main(int argc, char **argv)
{

	po::options_description desc("Command line options");
	size_t queries = 0, thread_num = 0;
	std::string address;
	unsigned port;

	desc.add_options()
		("help,h", "Produce help message")
		("queries,q", boost::program_options::value<size_t>(&queries)->default_value(1000),
				"Number of queries to generate")
		("threads,t", boost::program_options::value<size_t>(&thread_num)->default_value(100),
				"Number of client threads")
		("address,a", boost::program_options::value<std::string>(&address)->default_value("127.0.0.1"),
				"Server IP")
		("port,p", boost::program_options::value<unsigned>(&port)->default_value(8080),
				"Server port");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cerr << desc << std::endl;
		return 1;
	}

	IpAddr ip(address, port);

	std::vector<boost::shared_ptr<boost::thread> > threads;
	std::vector<LatencyStats> latencies(thread_num);

	for (size_t i = 0; i < thread_num; ++i)
		threads.push_back(boost::shared_ptr<boost::thread>(
				new boost::thread(thread_func, ip, queries, boost::ref(latencies[i]))));

	for (size_t i = 0; i < threads.size(); ++i)
		threads[i]->join();

	uint64_t sum = 0, worst = 0, best = -1;
	for (size_t i = 0; i < latencies.size(); ++i) {
		sum += latencies[i].avg;
		if (latencies[i].best < best)
			best = latencies[i].best;
		if (latencies[i].worst > worst)
			worst = latencies[i].worst;
	}

	std::cerr << "Average latency : " << sum/latencies.size() << std::endl;
	std::cerr << "Worst latency : " << worst << std::endl;
	std::cerr << "Best latency : " << best << std::endl;

	return 0;
}

