#ifndef NET_UTIL_HPP
#define NET_UTIL_HPP

#include<cstdlib>
#include<string>

ssize_t readn(int fd, void* buff, size_t n);
ssize_t readn(int fd, std::string& inBuffer, bool& zero);
ssize_t readn(int fd, std::string& inBuffer);
ssize_t writen(int fd, void* buff, size_t n);
ssize_t writen(int fd, std::string & sbbuff);
void handleForSigpipe();
int setSocketNonBlocking(int fd);
void setSocketNodelay(int fd);
void setSocketNoLinger(int fd);
void shutDownWR(int fd);
int socketBindListen(int port);
#endif // UTIL_HPP
