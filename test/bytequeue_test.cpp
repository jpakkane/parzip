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

#include <algorithm>
#include <array>
#include <bytequeue.hpp>
#include <cstdint>
#include <smalltest.hpp>
#include <vector>

void simple_data_test() {
    ByteQueue bq1024(1024);
    std::string input_data("Hello world!");

    bq1024.push(input_data.c_str(), input_data.length());
    bq1024.shutdown();
    auto res = bq1024.pop();
    std::string output_data(res.begin(), res.end());
    ST_ASSERT(input_data == output_data);
}

void split_data_test() {
    ByteQueue bq2(2);

    bq2.push("AB", 2);
    auto res = bq2.pop();
    bq2.push("CD", 2);
    auto res2 = bq2.pop();
    res.insert(res.end(), res2.begin(), res2.end());
    std::string output_data(res.begin(), res.end());
    ST_ASSERT(output_data == "ABCD");
}

void big_buf_test() {
    const int queue_size = 1024;
    const int test_size = queue_size * queue_size;
    ByteQueue bq(queue_size);
    int bytes_received = 0;
    int bytes_inserted = 0;

    auto push_future = std::async(std::launch::async, [&bq, &bytes_inserted, &test_size] {
        const int insert_size = 1000;
        char some_buf[insert_size];
        while (bytes_inserted < test_size) {
            int current_push_size;
            if (bytes_inserted + insert_size > test_size) {
                current_push_size = test_size - bytes_inserted;
                ST_ASSERT(current_push_size < insert_size);
            } else {
                current_push_size = insert_size;
            }
            bq.push(some_buf, current_push_size);
            bytes_inserted += current_push_size;
        }
        bq.shutdown();
    });

    auto pop_future = std::async(std::launch::async, [&bq, &bytes_received] {
        while (true) {
            bq.wait_until_full_or_shutdown();
            bytes_received += (int)bq.pop().size();
            QueueState cur_state = bq.state();
            if (cur_state == QueueState::SHUTDOWN) {
                // Grab the last bits, if any.
                bytes_received += (int)bq.pop().size();
                return;
            }
        }
    });

    push_future.get();
    pop_future.get();

    ST_ASSERT(bytes_received == test_size);
    ST_ASSERT(bytes_inserted == test_size);
}

void forwarder(ByteQueue *in, ByteQueue *out) {
    while (true) {
        in->wait_until_full_or_shutdown();
        auto data = in->pop();
        try {
            out->push(data.data(), data.size());
        } catch (...) {
            in->shutdown();
            return;
        }
        if (in->state() == QueueState::SHUTDOWN) {
            auto final_data = in->pop();
            try {
                if (!final_data.empty()) {
                    out->push(final_data.data(), final_data.size());
                }
            } catch (...) {
            }
            out->shutdown();
            return;
        }
    }
}

std::string get_all(ByteQueue *in) {
    std::string result;
    do {
        in->wait_until_full_or_shutdown();
        auto data = in->pop();
        result.insert(result.end(), data.begin(), data.end());
    } while (in->state() != QueueState::SHUTDOWN);
    auto final_data = in->pop();
    if (!final_data.empty()) {
        result.insert(result.end(), final_data.begin(), final_data.end());
    }
    return result;
}

void data_pusher(ByteQueue *bq, const std::string &data) {
    bq->push(data.c_str(), data.size());
    bq->shutdown();
}

std::string run_multibuf_test(const std::string &inmsg) {
    ByteQueue bq1(1);
    ByteQueue bq2(2);
    ByteQueue bq3(3);
    ByteQueue bq5(5);
    ByteQueue bq7(7);
    ByteQueue bq11(11);
    ByteQueue bq13(13);
    // Can't put non-movable things in an array. Gotta use pointers.
    std::array<ByteQueue *, 7> queues{&bq1, &bq2, &bq3, &bq5, &bq7, &bq11, &bq13};

    std::vector<std::future<void>> operations;
    operations.reserve(queues.size());
    for (size_t i = 0; i < queues.size() - 1; ++i) {
        operations.emplace_back(
            std::async(std::launch::async, forwarder, queues[i + 1], queues[i]));
    }
    auto result_getter = std::async(std::launch::async, get_all, queues[0]);
    auto push_result = std::async(std::launch::async, data_pusher, queues[6], inmsg);
    std::string outmsg = result_getter.get();
    std::for_each(operations.begin(), operations.end(), [](auto &f) { f.get(); });
    push_result.get();
    return outmsg;
}

void multibuf_test() {
    std::string inmsg("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    inmsg += inmsg;
    inmsg += inmsg;
    inmsg += "0123456789";

    // Run in own function to ensure all objects related to the pipeline
    // have been destroyed (and run their asserts) before looking at
    // the result.
    std::string outmsg = run_multibuf_test(inmsg);
    ST_ASSERT(inmsg == outmsg);
}

int main(int, char **) {
    ST_TEST(simple_data_test);
    ST_TEST(split_data_test);
    ST_TEST(big_buf_test);
    ST_TEST(multibuf_test);
    return 0;
}
