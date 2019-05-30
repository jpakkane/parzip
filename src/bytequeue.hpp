/*
 * Copyright (C) 2019 Jussi Pakkanen.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 3, or (at your option) any later version,
 * of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cassert>
#include <condition_variable>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

enum class QueueState { EMPTY, HAS_DATA, FULL, SHUTDOWN };

// Single producer
// Single consumer
class ByteQueue final {
public:
    explicit ByteQueue(const int64_t bsize) : buffer_size(bsize) { buffer.reserve(buffer_size); }

    ~ByteQueue() {
        assert(buffer.empty()); // Guard against data loss.
    }

    void push(const char *data, int64_t inbuf_size) {
        if (inbuf_size < 0) {
            throw std::logic_error("Negative value used for input buffer size.");
        }
        std::unique_lock<std::mutex> l(m);
        if (st == QueueState::SHUTDOWN) {
            throw std::logic_error("Tried to push data to a closed queue.");
        }
        if ((int64_t)buffer.size() + inbuf_size < buffer_size) {
            buffer.insert(buffer.end(), data, data + inbuf_size);
            // Buffer did not get full here.
            if (st == QueueState::EMPTY) {
                set_state(l, QueueState::HAS_DATA);
            }
            return;
        }
        push_internal(l, data, inbuf_size);
    }

    std::vector<char> pop() {
        std::vector<char> rv;
        {
            std::unique_lock<std::mutex> l(m);
            rv.reserve(buffer_size);
            rv.swap(buffer);
            if (st != QueueState::SHUTDOWN) {
                set_state(l, QueueState::EMPTY);
            }
        }
        return rv;
    }

    void wait_until_full_or_shutdown() const {
        std::unique_lock<std::mutex> l(m);
        while (!(st == QueueState::FULL || st == QueueState::SHUTDOWN)) {
            cv.wait(l);
        }
    }

    QueueState state(std::unique_lock<std::mutex> &) const { return st; }

    QueueState state() const {
        std::unique_lock<std::mutex> l(m);
        return state(l);
    }

    int64_t queue_size() const { return buffer_size; }

    void shutdown() {
        std::unique_lock<std::mutex> l(m);
        set_state(l, QueueState::SHUTDOWN);
    }

private:
    void push_internal(std::unique_lock<std::mutex> &l, const char *data, int64_t inbuf_size) {
        int64_t pushed_so_far = 0;
        while (pushed_so_far < inbuf_size) {
            auto this_round_size =
                std::min(buffer_size - (int64_t)buffer.size(), inbuf_size - pushed_so_far);
            auto start_point = data + pushed_so_far;
            auto end_point = start_point + this_round_size;
            buffer.insert(buffer.end(), start_point, end_point);
            pushed_so_far += this_round_size;
            if ((int64_t)buffer.size() == queue_size()) {
                set_state(l, QueueState::FULL);
                if (pushed_so_far == inbuf_size) {
                    // Everything is in but we don't need to write any more
                    // stuff. Return rather than blocking.
                    return;
                }
                // Wait until someone grabs buffer contents.
                while (st == QueueState::FULL) {
                    cv.wait(l);
                }
                if (st == QueueState::SHUTDOWN) {
                    return;
                }
            } else {
                set_state(l, QueueState::HAS_DATA);
            }
        }
    }

    // The first argument is only to ensure that this is only called with a held
    // lock.
    void set_state(std::unique_lock<std::mutex> &, const QueueState new_state) {
        const bool should_notify = new_state != st;
        if (st == QueueState::SHUTDOWN) {
            throw std::runtime_error("Trying to change the state of a closed queue.");
        }
        st = new_state;
        if (should_notify) {
            cv.notify_one();
        }
    }

    mutable std::mutex m;
    mutable std::condition_variable cv;
    std::vector<char> buffer;
    const int64_t buffer_size;
    QueueState st = QueueState::EMPTY;
};
