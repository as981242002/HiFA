#include<stdio.h>
#include<assert.h>
#include<unistd.h>
#include<functional>
#include"LogFile.hpp"
#include"AsyncLogging.h"


AsyncLogging::AsyncLogging(const std::string basename, int flushInterval):
    flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    thread_(std::bind(&AsyncLogging::threadFunc,this), "logging"),
    mutex_(),
    cond_(mutex_),
    count_(1),
    currentBuff_(new Buffer),
    nextBuff_(new Buffer),
    buffers_()
{
    assert(basename_.size() > 1);
    currentBuff_->bzero();
    nextBuff_->bzero();
    buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len)
{
    MutexLockGuard lock(mutex_);
    if(currentBuff_->avail() > len)
    {
        currentBuff_->append(logline, len);
    }
    else
    {
        buffers_.push_back(currentBuff_);
        currentBuff_->reset();
        if(nextBuff_)
            currentBuff_ = std::move(nextBuff_);
        else
            currentBuff_.reset(new Buffer);
        currentBuff_->append(logline, len);
        cond_.notify();
    }
}

void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    count_.countDown();
    LogFile output(basename_);
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferPtrVector buffersToWrite;
    buffersToWrite.reserve(16);

    while(running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            MutexLockGuard lock(mutex_);
            if(buffers_.empty())
            {
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(currentBuff_);
            currentBuff_.reset();

            currentBuff_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if(!nextBuff_)
            {
                nextBuff_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());

        if(buffersToWrite.size() > 25)
        {
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }

        for(size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }

        if(buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2);
        }

        if(!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if(!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }

    output.flush();
}
