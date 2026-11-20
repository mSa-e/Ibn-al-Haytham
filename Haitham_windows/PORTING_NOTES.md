# libeom Windows port

This is a Windows-oriented port of the 20-file dependency set identified for the original Networking server modules. The 20 library files are the `.h` and `.c` files under `DataStructures/`, `Networking/`, and `Systems/`; `main.c` is an extra example entry point.

## Windows changes
- POSIX sockets replaced with Winsock2 (`SOCKET`, `INVALID_SOCKET`, `accept`, `recv`, `send`, `closesocket`).
- Winsock initialization is performed once in `main()` using `WSAStartup`; cleanup belongs after the server stops.
- The pthread thread pool was replaced with Win32 threads, critical sections, and condition variables.
- Accepted client addresses are stored separately from the listening socket's bound address.
- HTTP request parsing is bounded and initializes all dictionaries, including empty bodies.
- Route records own their method list; the URI key is copied by the dictionary.
- Linked-list/tree/dictionary destruction handles empty structures safely.

## Fixes applied in the review
- `Server.interface` renamed `iface` (`interface` is a macro in Windows headers) and `WIN32_LEAN_AND_MEAN` is set in main.c.
- `enum HTTPMethods` values prefixed `HTTP_` (`DELETE` is a macro in winnt.h); use `HTTP_GET` etc.
- Routing ignores the query string/fragment; malformed or incomplete requests get 400, overlong URIs 414.
- 10 s receive timeout per connection; `shutdown(SD_SEND)` before `closesocket`.
- `accept()` failures on a closed listener no longer spin forever; `http_server_stop()` added.
- Thread pool: no 64-handle wait limit, `add_work` reports rejection (caller closes the socket instead of leaking it).
- `SO_EXCLUSIVEADDRUSE` set on the listener; LinkedList has a tail pointer (O(1) queue push) and an O(n^2) sort.

## Important limitations / follow-up work
- This remains a learning server, not a production HTTP implementation. It handles one request per connection, closes the connection, and does not implement chunked transfer encoding, persistent connections, TLS, multipart parsing, or full URL decoding.
Route callbacks return a complete HTTP response as a `malloc()`ed string, which the server sends and then `free()`s (return NULL to send nothing); there is no streaming.
- Each dictionary keeps its own key comparator (stored per entry), so dictionaries with different comparators can coexist.
- Request bodies: `Content-Length` bodies (max ~30 KB) are read; only `application/x-www-form-urlencoded` is parsed (into `HTTPRequest.body`, URL-decoded). Chunked/multipart bodies are ignored. Header names are matched case-insensitively.
- `server_constructor()` calls `exit()` if socket/bind/listen fails.
- The `ThreadPool` is Windows-specific and uses Win32 APIs. `launch()` blocks until `http_server_stop()` is called (main.c does this from a Ctrl+C handler); it then drains queued connections and joins the workers.
- Compile with MSYS2 UCRT64 GCC and link `-lws2_32`.

## Student portal (Portal/)
- `/` redirects to `/login`; `/login` (GET form, POST submit), `/home` (needs a session), `/logout` (POST), `/style.css`.
- Login accepts only the student in `Portal/StudentData.c` (name `Haitham`, password = student ID `25010000`). Name is case-insensitive; the password comparison is constant-time.
- Sessions: 128-bit random token (`rand_s`) in an `HttpOnly; SameSite=Strict` cookie, 30-minute sliding expiry, 64 slots, in memory only (restarting the server logs everyone out).
- Pages send a strict Content-Security-Policy (no inline scripts/styles). All data is HTML-escaped.
- This is plain HTTP: do not expose it beyond localhost, and do not reuse real passwords.
