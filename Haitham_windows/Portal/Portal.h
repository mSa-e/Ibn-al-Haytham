#ifndef PORTAL_PORTAL_H
#define PORTAL_PORTAL_H
#include "../Networking/Nodes/HTTPServer.h"

/* Registers the portal routes (/, /login, /home, /logout, /style.css) and creates the session
 * store. Call once, from the main thread, before launch(). */
int portal_register(struct HTTPServer *server);
void portal_shutdown(void);     /* call after the server has stopped */
#endif
