#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define ETHER_FRAME_LEN 14
#define IP_HDR_LEN 20
#define UDP_HDR_LEN 8
#define IEX_HDR_LEN 40
#define IEX_MSG_HDR_LEN 2

typedef struct {
    uint32_t block_type;
    uint32_t block_total_len;
    uint32_t block_order_magic;
    uint16_t major_version;
    uint16_t minor_version;
    uint64_t section_length;
} pcapng_section_header;

typedef struct {
    uint32_t block_type;
    uint32_t block_total_len;
} pcapng_block_header;

typedef struct {
    uint16_t message_length;
    uint8_t data[];
} iex_message_header;

#pragma pack(1)
typedef struct {
    uint8_t message_type;
    uint8_t flags;
    uint64_t timestamp;
    char symbol[8];
    uint32_t size;
    uint64_t price;
    uint64_t trade_id;
} iex_trade_report_message;

int main(int argc, char *argv[]) {
    int opt, fd;
    FILE* outfd;
    uint64_t offset = 0;
    struct stat sb;

    while ((opt = getopt (argc, argv, "i:o:")) != -1)
    {
        switch (opt) {
            case 'i': fd = open(optarg, O_RDONLY); break;
            case 'o': outfd = fopen(optarg, "w"); break;
        }
    }

    fstat(fd, &sb);
    char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE|MAP_NORESERVE, fd, 0);

    pcapng_section_header* hdr = (pcapng_section_header*)addr;
    offset += hdr->block_total_len;

    int count = 0;
    char tradesbuf[256*8000] = {0};

    fprintf(outfd, "timestamp,symbol,price,size\n");

    while (offset < sb.st_size - 8) {
        pcapng_block_header *blk_hdr = (pcapng_block_header*)(addr+offset);
        switch (blk_hdr->block_type) {
            case 0x00000006: {
                iex_message_header* iex_msg = (iex_message_header*)(addr+offset+28+ETHER_FRAME_LEN+IP_HDR_LEN+UDP_HDR_LEN+IEX_HDR_LEN);

                switch (iex_msg->data[0]) {
                    case 0x54: { //Trade report message
                        iex_trade_report_message* trade = (iex_trade_report_message*)(iex_msg->data);
                        char symbol[9] = {}; strncpy(symbol, trade->symbol, 8);
                        for (int i = 0; i < 9; i++) { if (symbol[i] == 0x20) { symbol[i] = '\0'; break;} }

                        if (count < 256*7999) {
                            count += sprintf((tradesbuf+count), "%llu,%s,%llu,%u\n", trade->timestamp, symbol, trade->price, trade->size);
                        } else {
                            fwrite(tradesbuf, 1, count, outfd);
                            memset(tradesbuf, 0, sizeof(tradesbuf));
                            count = sprintf((tradesbuf), "%s,%llu,%u\n", symbol, trade->price, trade->size);
                        }
                    }
                    break;
                }
                break;
            }
        };
        offset += blk_hdr->block_total_len;
    }
    fwrite(tradesbuf, 1, count, outfd);
    fclose(outfd);

    return 0;
}
