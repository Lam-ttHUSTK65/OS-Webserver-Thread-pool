#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"  // Địa chỉ IP của server
#define SERVER_PORT 8080      // Cổng kết nối với server

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        exit(1);
    }

    // Bắt đầu đo thời gian
    clock_t start_time = clock();

    int sock = socket(AF_INET, SOCK_STREAM, 0);  // Tạo socket kết nối TCP
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));  // Khởi tạo địa chỉ server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    // Kết nối đến server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    // Gửi yêu cầu tải file
    char request[1024];
    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\nHost: localhost\r\n\r\n", argv[1]);
    if (send(sock, request, strlen(request), 0) < 0) {
        perror("send");
        exit(1);
    }

    // Nhận phản hồi từ server
    char buffer[1024];
    int total_recv = 0;
    while (1) {
        int num_recv = recv(sock, buffer, sizeof(buffer), 0);
        if (num_recv < 0) {
            perror("recv");
            exit(1);
        } else if (num_recv == 0) {
            break;
        }

        total_recv += num_recv;
        //fwrite(buffer, 1, num_recv, stdout);  // Xuất dữ liệu lên stdout
    }

    // Đóng socket kết nối
    close(sock);

    // Kết thúc đo thời gian và in ra kết quả
    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Elapsed time: %.6f seconds\n", elapsed_time);

    return 0;
}
