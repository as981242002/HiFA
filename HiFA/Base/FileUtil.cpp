#include<assert.h>
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/stat.h>
#include "FileUtil.hpp"

using namespace std;

AppendFile::AppendFile(string filename):
    fp_(fopen(filename.c_str(), "ae"))
{
    setbuffer(fp_, buffer_, sizeof(buffer_));
}

AppendFile::~AppendFile()
{
    fclose(fp_);
}

void AppendFile::append(const char *message, const size_t len)
{
    size_t send = this->write(message, len);
    size_t remain = len - send;
    while(remain > 0)
    {
        size_t resend = this->write(message + send, remain);
        if(resend == 0)
        {
            int err = ferror(fp_);
            if(err)
                fprintf(stderr, "append file error : AppendFile::append \n");
            break;
        }
        send += resend;
        remain = len - send;
    }
}

void AppendFile::flush()
{
    fflush(fp_);
}

size_t AppendFile::write(const char *message, const size_t len)
{
    return fwrite_unlocked(message, 1, len, fp_);
}
