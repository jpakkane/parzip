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
#pragma once

#include<string>
#include<vector>
#include<mutex>

enum TaskState {
    TASK_NOT_STARTED,
    TASK_RUNNING,
    TASK_FINISHED,
};

class TaskControl final {
public:

    TaskControl();

    TaskState state() const;
    int successes() const;
    int failures() const;

private:
    mutable std::mutex m;
    TaskState cur_state;
    std::vector<std::string> results;
    int num_success;
    int num_failures;
};
