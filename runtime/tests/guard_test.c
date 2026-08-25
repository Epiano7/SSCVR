#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>

int main(void)
{
    HMODULE proxy = LoadLibraryA("opengl32.dll");
    if (!proxy) { puts("FAIL could not load runtime proxy"); return 2; }
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 3;
    SOCKET socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_handle == INVALID_SOCKET) return 4;
    Sleep(300);

    /* Ordinary send (Winsock ordinal 19) must remain untouched. An unconnected
       datagram socket should report that it has no destination. */
    int plain_send_result = send(socket_handle, "x", 1, 0);
    int plain_send_error = WSAGetLastError();

    struct sockaddr_in remote = {0};
    remote.sin_family = AF_INET;
    remote.sin_port = htons(11235);
    inet_pton(AF_INET, "192.0.2.1", &remote.sin_addr);
    int remote_result = sendto(socket_handle, "x", 1, 0,
                               (const struct sockaddr *)&remote, sizeof(remote));
    int remote_error = WSAGetLastError();

    struct sockaddr_in loopback = {0};
    loopback.sin_family = AF_INET;
    loopback.sin_port = htons(11235);
    inet_pton(AF_INET, "127.0.0.1", &loopback.sin_addr);
    int loopback_result = sendto(socket_handle, "x", 1, 0,
                                 (const struct sockaddr *)&loopback, sizeof(loopback));
    int loopback_error = WSAGetLastError();

    closesocket(socket_handle);
    WSACleanup();
    printf("send=%d error=%d remote=%d error=%d loopback=%d error=%d\n",
           plain_send_result, plain_send_error, remote_result, remote_error,
           loopback_result, loopback_error);
    if (plain_send_result != SOCKET_ERROR ||
        (plain_send_error != WSAEDESTADDRREQ && plain_send_error != WSAENOTCONN)) return 7;
    /* SSCVR must not block the game's ordinary network traffic. UDP sendto
       may report success without a listener because delivery is asynchronous. */
    if (remote_result == SOCKET_ERROR) return 5;
    if (loopback_result == SOCKET_ERROR) return 6;
    puts("PASS normal network traffic remains available");
    return 0;
}
