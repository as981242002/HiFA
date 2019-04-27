#include <getopt.h>
#include <string>
#include "./Net/EventLoop.hpp"
#include "./Net/Server.hpp"
#include  "./Base/Logging.hpp"

int main(int argc, char* args[])
{
	int thread_size = 4;
	int port = 8080;
	std::string logPath = "./HiFA.log";

	int opt;
	const char* str = "t:l:p:";
	while((opt == getopt(argc, args, str)) != -1)
	{
		switch (opt)
		{
			case 't':
			{
				thread_size = atoi(optarg);
				break;
			}
			case 'l':
			{
				logPath = optarg;
				if(logPath.size() < 2 || optarg[0] != '/')
				{
						printf("logPath should start with \" /\"\n");
						abort();
				}
				break;
			}
			case 'p':
			{
				port = atoi(optarg);
				break;
			}
			default:
				break;
		}
	}

	Logger::setLogFileName(logPath);
#ifndef _THREADS
	LOG << "_THREADS not defined";
#endif
	EventLoop mainLoop;
	Server myHttpServer (&mainLoop, thread_size, port);
	myHttpServer.start();
	mainLoop.loop();
	return 0;
}
