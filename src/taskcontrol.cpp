/*
 * Copyright (C) 2016 Jussi Pakkanen.
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

#include"taskcontrol.h"
#include<stdexcept>

TaskControl::TaskControl() : cur_state(TASK_NOT_STARTED),
    num_success(0),
    num_failures(0),
    total_tasks(0) {
}

void TaskControl::reserve(size_t num_entries) {
    if(cur_state != TASK_NOT_STARTED) {
        throw std::logic_error("Called reserve after task has started.");
    }
    total_tasks = (int) num_entries;
    results.reserve(total_tasks);
}

TaskState TaskControl::state() const {
    std::lock_guard<std::mutex> l(m);
    return cur_state;
}

void TaskControl::set_state(TaskState new_state) {
    // FIXME check that we are only going forwards.
    cur_state = new_state;
}

int TaskControl::successes() const {
    std::lock_guard<std::mutex> l(m);
    return num_success;
}

int TaskControl::failures() const {
    std::lock_guard<std::mutex> l(m);
    return num_failures;
}

int TaskControl::total() const {
    std::lock_guard<std::mutex> l(m);
    return total_tasks;
}

size_t TaskControl::finished() const {
    std::lock_guard<std::mutex> l(m);
    return results.size();
}

std::string TaskControl::entry(size_t i) const {
    std::lock_guard<std::mutex> l(m);
    // Return a copy because returning a pointer will crash if
    // the array gets resized before the result is used.
    return results.at(i);
}

void TaskControl::add_success(const std::string &msg) {
    std::lock_guard<std::mutex> l(m);
    results.push_back(msg);
    num_success++;
}

void TaskControl::add_failure(const std::string &msg) {
    std::lock_guard<std::mutex> l(m);
    results.push_back(msg);
    num_failures++;
}

void TaskControl::stop() {
    std::lock_guard<std::mutex> l(m);
    should_stop = true;
}

void TaskControl::check_for_stopping() {
    std::lock_guard<std::mutex> l(m);
    if(should_stop) {
        throw std::runtime_error("Stopping task evaluation.");
    }
}
