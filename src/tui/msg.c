#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "algo/raptor.h"
#include "tui/protocol.h"

void send_init_finished_message(int pipe_fd)
{
    ui_message *message = malloc(sizeof(ui_message));
    if (message)
    {
        message->type = UI_INIT_FINISHED;
        write(pipe_fd, &message, sizeof(ui_message *));
    }
}

void send_erase_ui_message(int pipe_fd)
{
    ui_message *message = malloc(sizeof(ui_message));
    if (message)
    {
        message->type = UI_ERASE;
        write(pipe_fd, &message, sizeof(ui_message *));
    }
}

void send_progress_ui_message(int pipe_fd, double progress, const char *fmt,
                              ...)
{
    va_list arglist;
    va_start(arglist, fmt);

    char *text;

    if (vasprintf(&text, fmt, arglist) != -1)
    {
        ui_message *message = malloc(sizeof(ui_message));
        if (message)
        {
            message->type = UI_MSG_PROGRESS;
            message->as.progress.text = text;
            message->as.progress.progress = progress;
            write(pipe_fd, &message, sizeof(ui_message *));
        }
        else
        {
            free(text);
        }
    }
    va_end(arglist);
}

void send_log_ui_message(int pipe_fd, const char *fmt, ...)
{
    va_list arglist;
    va_start(arglist, fmt);

    char *text;

    if (vasprintf(&text, fmt, arglist) != -1)
    {
        ui_message *message = malloc(sizeof(ui_message));
        if (message)
        {
            message->type = UI_MSG_LOG;
            message->as.log.text = text;
            write(pipe_fd, &message, sizeof(ui_message *));
        }
        else
        {
            free(text);
        }
    }
    va_end(arglist);
}

void send_search_res_ui(int pipe_fd, int request_seq, search_res *res)
{
    ui_message *message = malloc(sizeof(ui_message));
    if (message)
    {
        message->type = UI_STOP_RESULT_DATA;
        message->as.res = res;
        message->seq = request_seq;
        write(pipe_fd, &message, sizeof(ui_message *));
    }
}

void send_itinerary_res_ui(int pipe_fd, int request_seq, itinerary_res *res)
{
    ui_message *message = malloc(sizeof(ui_message));
    if (message)
    {
        message->type = UI_ITINERARY_RESULT_DATA;
        message->as.itinerary_req_res = res;
        message->seq = request_seq;
        write(pipe_fd, &message, sizeof(ui_message *));
    }
}

void send_shape_res_ui(int pipe_fd, int request_seq, shape_res *res)
{
    ui_message *message = malloc(sizeof(ui_message));
    if (message)
    {
        message->type = UI_SHAPE_RESULT_DATA;
        message->as.shapes = res;
        message->seq = request_seq;
        write(pipe_fd, &message, sizeof(ui_message *));
    }
}

void free_log_ui_message(ui_message *message)
{
    if (message != NULL)
    {
        free(message->as.log.text);
        free(message);
    }
}

void send_stop_req_logic(int pipe_fd, int request_seq, char *stop_name)
{
    logic_task_req *message = malloc(sizeof(logic_task_req));
    if (message)
    {
        message->type = LOGIC_TASK_STOP_REQ;
        message->stop_req.stop_name = stop_name;
        message->seq = request_seq;
        write(pipe_fd, &message, sizeof(logic_task_req *));
    }
}

void send_shape_req_logic(int pipe_fd, int request_seq, uint32_t included_modes)
{
    logic_task_req *message = malloc(sizeof(logic_task_req));
    if (message)
    {
        message->type = LOGIC_TASK_SHAPES_REQ;
        message->seq = request_seq;
        struct logic_task_shape_req shape_req = { 0 };
        shape_req.included_modes = included_modes;
        message->shape_req = shape_req;
        write(pipe_fd, &message, sizeof(logic_task_req *));
    }
}

void send_itinerary_shape_req_logic(int pipe_fd, int request_seq,
                                    struct raptor_itinerary *itinerary)
{
    logic_task_req *message = malloc(sizeof(logic_task_req));
    if (message)
    {
        message->type = LOGIC_TASK_ITINERARY_SHAPES_REQ;
        message->seq = request_seq;
        struct logic_task_itinerary_shape_req shape_req = { .itinerary =
                                                                itinerary };
        message->itinerary_shape_req = shape_req;
        write(pipe_fd, &message, sizeof(logic_task_req *));
    }
}

void send_itinerary_req_logic(int pipe_fd, int request_seq, char *dep_stop_id,
                              char *arr_stop_id, unsigned long dep_time)
{
    logic_task_req *message = malloc(sizeof(logic_task_req));
    if (message)
    {
        message->type = LOGIC_TASK_ITINERARY_REQ;
        message->itinerary_req.dep_id = dep_stop_id;
        message->itinerary_req.arr_id = arr_stop_id;
        message->itinerary_req.dep_time = dep_time;
        message->seq = request_seq;
        write(pipe_fd, &message, sizeof(logic_task_req *));
    }
}
