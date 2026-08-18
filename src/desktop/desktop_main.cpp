#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <sys/socket.h>
/*
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
*/
static volatile int running = 1;
int main()
{
    printf("DESKTOP APP");
}
/*
static void signal_handler(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    int dev_id;
    int sock;

    signal(SIGINT, signal_handler);


    dev_id = hci_get_route(NULL);

    if (dev_id < 0) {
        fprintf(stderr, "No Bluetooth adapter found\n");
        return 1;
    }

    printf("Using hci%d\n", dev_id);


    sock = hci_open_dev(dev_id);

    if (sock < 0) {
        perror("hci_open_dev");
        return 1;
    }


    uint8_t scan_type = 0x01;
    uint16_t interval = htobs(0x0010);
    uint16_t window   = htobs(0x0010);
    uint8_t own_addr_type = 0x00;
    uint8_t filter_policy = 0x00;

    if (hci_le_set_scan_parameters(
            sock,
            scan_type,
            interval,
            window,
            own_addr_type,
            filter_policy,
            1000) < 0) {

        perror("hci_le_set_scan_parameters");
        close(sock);
        return 1;
    }


    if (hci_le_set_scan_enable(
            sock,
            0x01,
            0x00,
            1000) < 0) {

        perror("hci_le_set_scan_enable");
        close(sock);
        return 1;
    }

    printf("Scanning for BLE devices...\n");
    printf("Press Ctrl+C to stop.\n\n");
    while (running) {
        unsigned char buf[HCI_MAX_EVENT_SIZE];

        int len = read(sock, buf, sizeof(buf));

        if (len < 0) {
            if (errno == EINTR)
                continue;

            perror("read");
            break;
        }

        if (len < 3)
            continue;

        uint8_t event = buf[0];

        if (event != EVT_LE_META_EVENT)
            continue;

        uint8_t subevent = buf[2];

        if (subevent != EVT_LE_ADVERTISING_REPORT)
            continue;


        uint8_t num_reports = buf[3];

        int offset = 4;

        for (int i = 0; i < num_reports; i++) {

            if (offset + 10 > len)
                break;


            uint8_t event_type = buf[offset];

            uint8_t addr_type = buf[offset + 1];

            bdaddr_t addr;

            memcpy(
                &addr,
                &buf[offset + 2],
                sizeof(bdaddr_t)
            );

            uint8_t data_len = buf[offset + 8];

            if (offset + 9 + data_len >= len)
                break;

            int8_t rssi =
                (int8_t)buf[offset + 9 + data_len];

            char addr_str[18];

            ba2str(&addr, addr_str);

            printf(
                "%s  RSSI=%d  event=0x%02x  addr_type=%d\n",
                addr_str,
                rssi,
                event_type,
                addr_type
            );

            uint8_t *data = &buf[offset + 9];

            printf("    AD:");

            for (int j = 0; j < data_len; j++)
                printf(" %02x", data[j]);

            printf("\n");

            offset += 10 + data_len;
        }
    }


    hci_le_set_scan_enable(
        sock,
        0x00,
        0x00,
        1000
    );

    close(sock);

    printf("\nScan stopped.\n");

    return 0;
}*/
