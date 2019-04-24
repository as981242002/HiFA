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
