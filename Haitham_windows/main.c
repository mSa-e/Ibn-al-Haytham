#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN   /* must precede any Windows header (avoids the `interface` macro etc.) */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include "Networking/Nodes/HTTPServer.h"
#include "Portal/Portal.h"

static struct HTTPServer *g_server;

static BOOL WINAPI on_console_event(DWORD type)
{
    (void)type;
    if (g_server) http_server_stop(g_server);   /* makes launch() return so cleanup below runs */
    return TRUE;
}

int main(void)
{
    WSADATA w;
    struct HTTPServer s;

    if (WSAStartup(MAKEWORD(2, 2), &w) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    s = http_server_constructor();
    g_server = &s;
    SetConsoleCtrlHandler(on_console_event, TRUE);

    if (!portal_register(&s)) {
        fprintf(stderr, "Could not initialise the portal\n");
        http_server_destructor(&s);
        WSACleanup();
        return 1;
    }

    printf("Student portal running at http://localhost:8080/  (Ctrl+C to stop)\n");
    s.launch(&s);

    g_server = NULL;
    http_server_destructor(&s);
    portal_shutdown();
    WSACleanup();
    return 0;
}
