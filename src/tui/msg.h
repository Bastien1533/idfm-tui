#ifndef TUI_MSG_H
#define TUI_MSG_H

#include "tui/protocol.h"

void send_erase_ui_message(int pipe_fd);
void send_init_finished_message(int pipe_fd);
void send_log_ui_message(int pipe_fd, const char *fmt, ...);
void send_progress_ui_message(int pipe_fd, double progress, const char *fmt,
                              ...);

void send_search_res_ui(int pipe_fd, int request_seq, search_res *res);
void send_itinerary_res_ui(int pipe_fd, int request_seq, itinerary_res *res);
void send_shape_res_ui(int pipe_fd, int request_seq, shape_res *res);

void send_stop_req_logic(int pipe_fd, int request_seq, char *stop_name);
void send_itinerary_req_logic(int pipe_fd, int request_seq, char *dep_stop_id,
                              char *arr_stop_id, unsigned long dep_time);
void send_shape_req_logic(int pipe_fd, int request_seq,
                          uint32_t included_modes);
void send_itinerary_shape_req_logic(int pipe_fd, int request_seq,
                                    struct raptor_itinerary *itinerary);

void free_log_ui_message(ui_message *message);

#endif
