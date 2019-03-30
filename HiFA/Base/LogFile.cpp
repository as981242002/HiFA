#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "FileUtil.hpp"
#include "LogFile.hpp"

using namespace std;



LogFile::LogFile(const string &basename, int flushEveryN):
    basename_(basename), flushEveryN_(flushEveryN), count_(0), mutex_(new MutexLock())
{
    file_.reset(new AppendFile(basename));
}


void LogFile::append(const char *logline, int len)
{
    MutexLockGugrd lock(*mutex_);
    appendUnlocked(logline, len);
}

void LogFile::flush()
{
    MutexLockGugrd lock(*mutex_);
    file_->flush();
}

bool LogFile::rollFile()
{

}

void LogFile::appendUnlocked(const char *logline, int len)
{
    file_->append(logline, len);
    count_++;
    if(count_ >= flushEveryN_)
    {
        count_ = 0;
        file_->flush();
    }
}
