//
// metamod-p - engine mock for unit testing
//
// engine_mock.h
//

#ifndef ENGINE_MOCK_H
#define ENGINE_MOCK_H

#define MOCK_MAX_MSGS 64
#define MOCK_MSG_LEN 1024

void mock_reset(void);

// Captured ALERT messages (from pfnAlertMessage)
int mock_get_alert_count(void);
const char *mock_get_alert_msg(int index);

// Captured SERVER_PRINT messages
int mock_get_server_print_count(void);
const char *mock_get_server_print_msg(int index);

// Captured CLIENT_PRINTF messages
int mock_get_client_print_count(void);
const char *mock_get_client_print_msg(int index);

// Settable CMD_ARGV/ARGC/ARGS
void mock_set_cmd_args(int argc, const char **argv, const char *args);

// Settable CVarGetFloat return value (keyed by cvar name)
void mock_set_cvar_float(const char *name, float value);

// Settable IndexOfEdict return value
void mock_set_index_of_edict(int index);

// Settable GET_GAME_DIR return value
void mock_set_gamedir(const char *dir);

// Settable LOCALINFO key/value pairs (pfnInfoKeyValue)
void mock_set_localinfo(const char *key, const char *value);

#endif // ENGINE_MOCK_H
