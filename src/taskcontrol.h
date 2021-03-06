/*
 * Copyright (C) 2016-2019 Jussi Pakkanen.
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

#include <mutex>
#include <string>
#include <vector>

enum TaskState {
    TASK_NOT_STARTED,
    TASK_RUNNING,
    TASK_FINISHED,
};

class TaskControl final {
public:
    TaskControl();

    void reserve(size_t num_entries);
    TaskState state() const;
    void set_state(TaskState new_state);
    int successes() const;
    int failures() const;
    int total() const;

    void add_success(const std::string &msg);
    void add_failure(const std::string &msg);
    size_t finished() const;
    std::string entry(size_t i) const;

    void stop();
    bool should_stop() const;
    void throw_if_stopped() const;

private:
    mutable std::mutex m;
    TaskState cur_state;
    std::vector<std::string> results;
    int num_success;
    int num_failures;
    int total_tasks;
    bool stopped = false;
};
