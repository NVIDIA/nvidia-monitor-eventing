/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

#include "common.hpp"

#include <semaphore>

/**
 * The compile-time minimum number of concurrent threads the semaphore must support.
 */
#ifndef THREADPOOL_SEMAPHORE_MIN_CAPABILITY
#define THREADPOOL_SEMAPHORE_MIN_CAPABILITY 32
#endif

/**
 * For deadlock prevention: the timeout after which threads that are
 * still waiting for the semaphore will go ahead and proceed without it.
 */
#ifndef THREADPOOL_QUEUED_THREAD_TIMEOUT
#define THREADPOOL_QUEUED_THREAD_TIMEOUT 60
#endif

class ThreadpoolGuard;

enum class AcquireState : int
{
    succ,
    error,
    timeout,  // currently unused, as timeout will now cancel thread creation
};

class ThreadpoolManager
{
  public:
    ThreadpoolManager(const int maxRunning, const int maxTotal) :
        _sem(maxRunning),
        _waiting(0),
        _maxTotal(maxTotal)
    {
        log_err("maxRunning %i, maxTotal %i\n", maxRunning, maxTotal);
        if (maxRunning > THREADPOOL_SEMAPHORE_MIN_CAPABILITY)
        {
            throw std::runtime_error("ThreadpoolManager: maxRunning cannot be greater than THREADPOOL_SEMAPHORE_MIN_CAPABILITY");
        }
        if (_maxTotal < maxRunning)
        {
            throw std::runtime_error("ThreadpoolManager: maxTotal cannot be less than maxRunning");
        }
    }

    ThreadpoolManager(ThreadpoolManager const&) = delete;
    ThreadpoolManager& operator=(ThreadpoolManager const&) = delete;

  private:
    AcquireState try_acquire()
    {
        log_wrn("entering try_acquire\n");
        // atomically increment and return the old value
        int existing_waiters = _waiting.fetch_add(1);
        if (existing_waiters >= _maxTotal)
        {
            _waiting--;
            log_err("thread cap reached: %i waiting, %i limit\n",
                _waiting.load(),
                _maxTotal);
            return AcquireState::error;
        }
        log_wrn("there are now %i threads waiting\n", _waiting.load());
        AcquireState rc = AcquireState::succ;
        // try_acquire_for returns true if acquired, false if timeout
        if (!_sem.try_acquire_for(std::chrono::seconds(THREADPOOL_QUEUED_THREAD_TIMEOUT)))
        {
            log_err("try_acquire_for reached timeout, not creating thread! %i waiting, %i limit\n",
                _waiting.load(),
                _maxTotal);
            _waiting--;
            return AcquireState::error;
        }
        _waiting--;
        log_wrn("created thread, %i still waiting\n", _waiting.load());
        return rc;
    }

    void release() {
        log_wrn("releasing semaphore, %i threads waiting\n", _waiting.load());
        _sem.release();
    }

    std::counting_semaphore<THREADPOOL_SEMAPHORE_MIN_CAPABILITY> _sem;
    std::atomic_int _waiting;
    int _maxTotal;

    friend class ThreadpoolGuard;
    // friend ThreadpoolGuard::ThreadpoolGuard(ThreadpoolManager&);
    // friend ThreadpoolGuard::~ThreadpoolGuard();
};


class ThreadpoolGuard
{
  public:
    ThreadpoolGuard(ThreadpoolManager* threadpool) :
        _threadpool(threadpool)
    {
        AcquireState rc = _threadpool->try_acquire();
        switch (rc)
        {
            case AcquireState::succ:
                _acquired = true;
                _success = true;
                break;
            case AcquireState::error:
                _acquired = false;
                _success = false;
                break;
            case AcquireState::timeout:
                _acquired = false;
                _success = true;
                break;
            default:
                log_err("unrecognized AcquireState value\n");
                _acquired = false;
                _success = false;
                break;
        }
    }

    ~ThreadpoolGuard()
    {
        // Only call release if we actually acquired the semaphore.
        // Even if _success is true, that may be due to timing out but continuing anyway.
        if (_acquired)
        {
            _threadpool->release();
        }
    }

    bool was_successful()
    {
        return _success;
    }

    ThreadpoolGuard(ThreadpoolGuard const&) = delete;
    ThreadpoolGuard& operator=(ThreadpoolGuard const&) = delete;

  private:
    ThreadpoolManager* _threadpool;
    bool _success;
    bool _acquired;
};
