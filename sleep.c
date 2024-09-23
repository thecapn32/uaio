// Copyright 2023 Vahid Mardani
/*
 * This file is part of uaio.
 *  uaio is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  uaio is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with uaio. If not, see <https://www.gnu.org/licenses/>.
 *
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 */
#include "esp_sleep.h"

#include "uaio.h"


static void
_callback(struct uaio_task *task) {
    if (task && (task->status == UAIO_WAITING)) {
        task->status = UAIO_RUNNING;
    }
    ESP_ERROR_CHECK(esp_timer_delete(task->sleep));
}


int
uaio_sleep(struct uaio_task *task, unsigned long us) {
    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = (void (*)(void *)) &_callback,
            .arg = (void*) task
    };
    task->status = UAIO_WAITING;
    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &task->sleep));
    ESP_ERROR_CHECK(esp_timer_start_once(task->sleep, us));
    return 0;
}
