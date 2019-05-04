#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Util.hpp"
#include "HttpData.hpp"

pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime;

const uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const int DEFAULT_EXPIRED_TIME = 2000;
const int DEFAULT_KEEP_TIME = 5 * 60 * 1000;

void MimeType::init()
{
    mime[".html"] = "text/html";
     mime[".avi"] = "video/x-msvideo";
     mime[".bmp"] = "image/bmp";
     mime[".c"] = "text/plain";
     mime[".doc"] = "application/msword";
     mime[".gif"] = "image/gif";
     mime[".gz"] = "application/x-gzip";
     mime[".htm"] = "text/html";
     mime[".ico"] = "image/x-icon";
     mime[".jpg"] = "image/jpeg";
     mime[".png"] = "image/png";
     mime[".txt"] = "text/plain";
     mime[".mp3"] = "audio/mp3";
     mime["default"] = "text/html";
}

string MimeType::getMime(const string &suffix)
{
    pthread_once(&once_control, MimeType::init);
    if(mime.count(suffix) == 0)
    {
        return  mime["default"];
    }

    return  mime[suffix];
}

HttpData::HttpData(EventLoop *loop, int connfd):
    loop_(loop),
    channel_(new Channel(loop, connfd)),
    fd_(connfd),
    error_(false),
    connectionState_(ConnectionState::H_CONNECTED),
    method_(HttpMethod::METHOD_GET),
    HTTPVersion_(HttpVersion::HTTP_11),
    nowReadPos_(0),
    state_(ProcessState::STATE_PARSE_URI),
    hState_(ParseState::H_START),
    keepAlive_(false)
{
    channel_->setReadHandler(bind(&HttpData::handleRead, this));
    channel_->setWriteHandler(bind(&HttpData::handleWrite,this));
    channel_->setConnHandler(bind(&HttpData::handleConn, this));
}

void HttpData::reset()
{
    fileName_.clear();
    path_.clear();
    headers_ .clear();
    nowReadPos_ = 0;
    hState_ = ParseState::H_START;

    if(timer_.lock())
    {
        shared_ptr<TimerNode> my_timer(timer_.lock());
        my_timer->clearReq();
        timer_.reset();
    }
}

void HttpData::seperateTimer()
{
    if(timer_.lock())
    {
        shared_ptr<TimerNode> my_timer(timer_.lock());
        my_timer->clearReq();
        timer_.reset();
    }
}

void HttpData::handleClose()
{
    connectionState_ = ConnectionState::H_DISCONNECTED;
    shared_ptr<HttpData> guard(shared_from_this());
    loop_->removeFromPoller(channel_);
}

void HttpData::newEvent()
{
    channel_->setEvents(DEFAULT_EVENT);
    loop_->addToPoller(channel_, DEFAULT_EXPIRED_TIME);
}

void HttpData::handleRead()
{
    uint32_t& events = channel_->getEvents();
    do
    {
        bool zero = false;
        int read_num = readn(fd_, inBuffer_, zero);
        LOG << "Request:" << inBuffer_;
        if(connectionState_ == ConnectionState::H_DISCONNECTING)
        {
            inBuffer_.clear();
            break;
        }

        if(read_num < 0)
        {
            perror("1");
            error_ = true;
            handleError(fd_, 400, "Bad Request");
            break;
        }
        else if(zero)
        {
            connectionState_ = ConnectionState::H_DISCONNECTING;
            if(read_num == 0)
            {
                break;
            }
        }

        if(state_ == ProcessState::STATE_PARSE_URI)
        {
            URIState flag  = this->parseURI();
            if(flag == URIState::PARSE_URI_AGAIN)
            {
                break;
            }
            else if(flag == URIState::PARSE_URI_ERROR)
            {
                perror("2");
                LOG << "fd = " << fd_ << "," << inBuffer_ << "*******";
                inBuffer_.clear();
                handleError(fd_, 400, "Bad Request");
                break;
            }
            else
                state_ = ProcessState::STATE_PARSE_HEADERS;
        }

        if(state_ == ProcessState::STATE_PARSE_HEADERS)
        {
            HeaderState flag = this->parseHeaders();
            if(flag == HeaderState::PARSE_HEADER_AGAIN)
                break;
            else if(flag == HeaderState::PARSE_HEADER_ERROR)
            {
                perror("3");
                error_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            }

            if(method_ == HttpMethod::METHOD_POST)
            {
                state_ = ProcessState::STATE_RECV_BODY;
            }
            else
            {
                state_ = ProcessState::STATE_ANALYSIS;
            }
        }

        if(state_ == ProcessState::STATE_RECV_BODY)
        {
            int conlen = -1;
            if(headers_.find("Content-length") != headers_.end())
            {
                conlen = stoi(headers_["Content-length"]);
            }
            else
            {
                error_ = true;
                handleError(fd_, 400, "Bad Request: Lack of argument(Content-length)");
                break;
            }
            if(static_cast<int>(inBuffer_.size() < conlen))
                break;
            state_ = ProcessState::STATE_ANALYSIS;
        }

        if(state_ == ProcessState::STATE_ANALYSIS)
        {
            AnalyusisState flag = this->analysisRequest();
            if(flag == AnalyusisState::ANALYSIS_SUCCESS)
            {
                state_ = ProcessState::STATE_FINSH;
                break;
            }
            else
            {
                error_ = true;
                break;
            }
        }

    } while(false);

    if(!error_)
    {
        if(outBuffer_.size() > 0)
        {
            handleWrite();
        }

        if(!error_ && state_ == ProcessState::STATE_FINSH)
        {
            this->reset();
            if(inBuffer_.size() > 0 && connectionState_ != ConnectionState::H_DISCONNECTING)
            {
                handleRead();
            }
        }
        else if(!error_ && connectionState_ != ConnectionState::H_DISCONNECTED)
        {
            events |= EPOLLIN;
        }
    }
}

void HttpData::handleWrite()
{
    if(!error_ && connectionState_ != ConnectionState::H_DISCONNECTED)
    {
        uint32_t& events = channel_->getEvents();
        if(writen(fd_, outBuffer_) < 0)
        {
            perror("writen");
            events = 0;
            error_ = true;
        }

        if(outBuffer_.size() > 0)
            events |= EPOLLOUT;
    }
}

void HttpData::handleConn()
{
    seperateTimer();
    uint32_t& events = channel_->getEvents();
    if(!error_ && connectionState_ == ConnectionState::H_CONNECTED)
    {
         if(events != 0)
         {
             int timeout = DEFAULT_EXPIRED_TIME;
             if(keepAlive_)
             {
                 timeout = DEFAULT_KEEP_TIME;
             }

             if((events & EPOLLIN) && (events & EPOLLOUT))
             {
                 events = static_cast<uint32_t>(0);
                 events |= EPOLLOUT;
             }

             events |= EPOLLOUT;
             loop_->updatePoller(channel_, timeout);
         }
         else if(keepAlive_ )
         {
             events |= (EPOLLIN | EPOLLOUT);
             int timeout = DEFAULT_KEEP_TIME;
             loop_->updatePoller(channel_, timeout);
         }
         else
         {
             events |= (EPOLLIN | EPOLLOUT);
             int timeout = (DEFAULT_KEEP_TIME >> 1);
             loop_->updatePoller(channel_, timeout);
         }
    }
    else if(!error_ && connectionState_ == ConnectionState::H_DISCONNECTING && (events & EPOLLOUT))
    {
        events = (EPOLLOUT | EPOLLET);
    }
    else
    {
        loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
    }

}


void HttpData::handleError(int fd, int err_num, string short_msg)
{
    short_msg = " " + short_msg ;
    char sendBuff[4 * 1024];
    string bodyBuff, headerBuff;
    bodyBuff += "<html><title>Aliens have been here, error....<title>";
    bodyBuff += "<body bgcolor=\"ffffff\">";
    bodyBuff += to_string(err_num) + short_msg;
    bodyBuff += "<hr><em> HiFA </em>\n</body></html>";

    headerBuff += "HTTP/1.1" + to_string(err_num) + short_msg + "\r\n";
    headerBuff += "Connect-Typ: text/html\r\n" ;
    headerBuff += "Connection: Close \r\n";
    headerBuff += "Content-Length: " + to_string(bodyBuff.size()) + "\r\n";
    headerBuff += "Server: HiFA\r\n";
    headerBuff += "\r\n";

    sprintf(sendBuff, "%s", headerBuff.c_str());
    writen(fd, sendBuff, strlen(sendBuff));
    sprintf(sendBuff, "%s", bodyBuff.c_str());
	writen(fd, sendBuff, strlen(sendBuff));
}

URIState HttpData::parseURI()
{
	string str = inBuffer_;
	string copy = str;

	size_t  pos  = str.find('\r', nowReadPos_);
	if(pos == string::npos)
	{
			return URIState::PARSE_URI_AGAIN;
	}

	string request_line = str.substr(0, pos);
	if(str.size() > pos + 1)
	{
		str = str.substr(pos + 1);
	}
	else
		str.clear();

	size_t posGet = request_line.find("GET");
	size_t posPost = request_line.find("POST");
	size_t posHead = request_line.find("HEAD");

	if(posGet  != string::npos)
	{
		pos = posGet;
		method_  = HttpMethod::METHOD_GET;
	}
	else if(posPost != string::npos)
	{
		pos = posPost;
		method_ = HttpMethod::METHOD_POST;
	}
	else if(posHead != string::npos)
	{
		pos = posHead;
		method_ = HttpMethod::METHOD_HEAD;
	}
	else
	{
		return URIState::PARSE_URI_ERROR ;
	}

	pos = request_line.find("/", pos);
	if(pos == string::npos)
	{
		fileName_ = "index.html";
		HTTPVersion_ = HttpVersion::HTTP_11;
		return URIState::PARSE_URI_SUCCESS ;
	}
	else
	{
		size_t space_pos = request_line.find(' ', pos);
		if(space_pos == string::npos)
		{
			return  URIState::PARSE_URI_ERROR;
		}
		else
		{
			if(space_pos - pos > 1)
			{
				fileName_ = request_line.substr(pos +  1, space_pos - pos - 1);
				size_t  question_pos = fileName_.find('?');
				if(question_pos != string::npos )
				{
					fileName_ = fileName_.substr(0, question_pos);
				}
			}
			else
			{
				fileName_ = "index.html";
			}
		}
		pos = space_pos;
	}

	pos = request_line.find("/", pos);
	if(pos == string::npos)
		return URIState::PARSE_URI_ERROR;
	else
	{
		if(request_line.size() - pos <= 3)
			return URIState::PARSE_URI_ERROR;
		else
		{
			string ver = request_line.substr(pos + 1, 3);
			if(ver == "1.0")
				HTTPVersion_ = HttpVersion::HTTP_10;
			else  if(ver == "1.1")
				HTTPVersion_ = HttpVersion::HTTP_11;
			else
				return  URIState::PARSE_URI_ERROR;
		}
	}

	return  URIState::PARSE_URI_SUCCESS;
}

HeaderState HttpData::parseHeaders()
{
	string& str = inBuffer_;
	int key_start = -1;
	int key_end = -1;
	int value_start = -1;
	int value_end = -1;
	int now_read_line_begin = 0;
	bool not_finish = true;
	size_t i = 0;
	for(; i < str.size() && not_finish; ++ i)
	{
		switch (hState_)
		{
			case ParseState::H_START :
			{
				if(str[i] == '\n' || str[i] == '\r')
				{
					break;
				}

				hState_ = ParseState::H_KEY;
				key_start = i ;
				now_read_line_begin = i;
				break;
			}
			case  ParseState::H_KEY:
			{
				if(str[i] == ':')
				{
					key_end = i;
					if(key_end - key_start <= 0)
					{
						return HeaderState::PARSE_HEADER_ERROR;
					}
					hState_ = ParseState::H_COLON;
				}
				else if(str[i] == '\n' || str[i] == '\r')
				{
					return HeaderState::PARSE_HEADER_ERROR;
				}
				break;
			}
			case ParseState::H_COLON:
			{
				if(str[i] == ' ')
				{
					hState_ = ParseState::H_SPACES_AFTER_COLON;
				}
				else
				{
					return HeaderState::PARSE_HEADER_ERROR;
				}
				break;
			}
			case ParseState::H_SPACES_AFTER_COLON:
			{
				hState_ = ParseState::H_VALUE;
				value_start = i;
				break;
			}
			case ParseState::H_VALUE:
			{
				if(str[i] == '\r')
				{
					hState_ = ParseState::H_CR;
					value_end = i;
					if(value_end - value_start <= 0)
						return HeaderState::PARSE_HEADER_ERROR;
				}
				else  if(i - value_start > 255)
					return HeaderState::PARSE_HEADER_ERROR;
				break;
			}
			case ParseState::H_CR:
			{
				if(str[i] == '\n')
				{
					hState_ = ParseState::H_LF;
					string key(str.begin() + key_start, str.begin() + key_end);
					string value(str.begin() + value_start, str.begin() + value_end);
					headers_[key] = value;
					now_read_line_begin = i;
				}
				else
					return HeaderState::PARSE_HEADER_ERROR;
				break;
			}
			case ParseState::H_LF:
			{
				if(str[i] == '\r')
				{
					hState_ = ParseState::H_END_CR;
				}
				else
				{
					key_start = i;
					hState_ = ParseState::H_KEY;
				}
				break;
			}
			case ParseState::H_END_CR:
			{
				if(str[i] == '\n')
				{
					hState_ = ParseState::H_END_LF;
				}
				else
					return HeaderState::PARSE_HEADER_ERROR;
				break;
			}
			case ParseState::H_END_LF:
			{
				not_finish = false;
				key_start = i;
				now_read_line_begin = i;
				break;
			}
		}
	}
	if(hState_ == ParseState::H_END_LF)
	{
		str = str.substr(i);
		return HeaderState::PARSE_HEADER_SUCCESS;
	}
	str = str.substr(now_read_line_begin);
	return HeaderState::PARSE_HEADER_AGAIN;
}

AnalyusisState HttpData::analysisRequest()
{
	if(method_ == HttpMethod::METHOD_POST)
	{

	}
	else if(method_ == HttpMethod::METHOD_GET || method_ == HttpMethod::METHOD_HEAD)
	{
		string header = "";
		header += "HTTP/1.1 200 OK \r\n";
		if(headers_.find("Connection") != headers_.end() && (headers_["Connection"] == "Keep-Alive" || headers_["Connection"] == "keep-alive"))
		{
			keepAlive_ = true;
			header += string("Connection:Keep-Alive\r\n") + "Keep-Alive: timeout" + to_string(DEFAULT_KEEP_TIME) + "\r\n";
		}
		int dot_pos = fileName_.find('.');
		string filetype = "";
		if(dot_pos < 0)
		{
			filetype = MimeType::getMime("default");
		}
		else
		{
			filetype = MimeType::getMime(fileName_.substr(dot_pos));
		}

		if(fileName_ == "hello")
		{
			outBuffer_ = "HTTP/1.1 200 OK \r\n Connect-type:text /plain \r\n\r Hello HiFA";
			return AnalyusisState::ANALYSIS_SUCCESS;
		}

		struct stat sbuf;
		if(stat(fileName_.c_str(), &sbuf) < 0)
		{
			header.clear();
			handleError(fd_, 404, "Not found");
			return AnalyusisState::ANALYSIS_ERROR;
		}
		header += "Content-type:" + filetype + "\r\n";
		header += "Content-type:" + to_string(sbuf.st_size) + "\r\n";
		header += "Server: HiFA \r\n";

		header += "\r\n";
		outBuffer_ += header;

		if(method_ == HttpMethod::METHOD_HEAD)
		{
			return AnalyusisState::ANALYSIS_SUCCESS;
		}

		int src_fd = open(fileName_.c_str(), O_RDONLY, 0);
		if(src_fd < 0)
		{
			outBuffer_.clear();
			handleError(fd_, 404, "not found");
			return  AnalyusisState::ANALYSIS_ERROR;
		}

		void* mmapRet = mmap(nullptr, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
		close(src_fd);
		if(mmapRet == (void*) -1)
		{
			munmap(mmapRet, sbuf.st_size);
			outBuffer_.clear();
			handleError(fd_, 404, "not found");
			return  AnalyusisState::ANALYSIS_ERROR;
		}

		char* src_addr = static_cast<char*>(mmapRet);
		outBuffer_ += string(src_addr, src_addr + sbuf.st_size);
		munmap(mmapRet, sbuf.st_size);
		return  AnalyusisState::ANALYSIS_SUCCESS;
	}
	return AnalyusisState::ANALYSIS_ERROR;
}

