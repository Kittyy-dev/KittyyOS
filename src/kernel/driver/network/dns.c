
#include <stdint.h>
#include <stddef.h>
#include <net.h>
#include <net_config.h>
#include <kprint.h>

#define DNS_PORT 53
#define DNS_LOCAL_PORT 49152
#define DNS_TXID 0x4B59

static uint8_t resolved_ip[4];
static int dns_ready = 0;
static int dns_pending = 0;

static uint16_t be16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static int ip_equal(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < 4; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

static int skip_name(const uint8_t *data, uint16_t length, uint16_t *offset) {
    uint16_t pos = *offset;

    for (uint16_t steps = 0; steps < length; steps++) {
        if (pos >= length) {
            return 0;
        }

        uint8_t n = data[pos];

        if ((n & 0xC0) == 0xC0) {
            if (pos + 1 >= length) {
                return 0;
            }
            *offset = pos + 2;
            return 1;
        }

        if (n & 0xC0) {
            return 0;
        }

        pos++;

        if (n == 0) {
            *offset = pos;
            return 1;
        }

        if (n > 63 || pos + n > length) {
            return 0;
        }

        pos += n;
    }

    return 0;
}

int dns_resolve(const char *hostname) {
    if (!hostname || !hostname[0]) {
        return 0;
    }

    const struct ipv4_config *cfg = net_config_get();

    if (!cfg) {
        return 0;
    }

    uint8_t query[512] = {0};
    uint16_t pos = 12;

    put16(&query[0], DNS_TXID);
    put16(&query[2], 0x0100);
    put16(&query[4], 1);

    const char *p = hostname;

    while (*p) {
        uint16_t len_pos = pos++;
        uint8_t label_len = 0;

        while (*p && *p != '.') {
            if (label_len >= 63 || pos >= 505) {
                return 0;
            }

            query[pos++] = (uint8_t)*p++;
            label_len++;
        }

        if (label_len == 0)
            return 0;

        query[len_pos] = label_len;

        if (*p == '.') {
            p++;
        }
    }

    query[pos++] = 0;
    put16(&query[pos], 1); // QTYPE A
    pos += 2;
    put16(&query[pos], 1); // QCLASS IN
    pos += 2;

    dns_ready = 0;
    dns_pending = 1;

    if (!udp_send(cfg->dns, DNS_LOCAL_PORT, DNS_PORT, query, pos)) {
        dns_pending = 0;
        return 0;
    }

    log_kernel("DNS: Query sent for %s\n", hostname);
    return 1;
}

void dns_receive(const uint8_t *data, uint16_t length, const uint8_t src_ip[4], uint16_t src_port, uint16_t dst_port) {
    if (!dns_pending || !data || !src_ip || length < 12) {
        return;
    }

    const struct ipv4_config *cfg = net_config_get();

    if (!cfg || !ip_equal(src_ip, cfg->dns) || src_port != DNS_PORT || dst_port != DNS_LOCAL_PORT) {
        return;
    }

    if (be16(&data[0]) != DNS_TXID) {
        return;
    }

    uint16_t flags = be16(&data[2]);

    if (!(flags & 0x8000) || (flags & 0x7800) || (flags & 0x0200) || (flags & 0x000F)) {
        return;
    }

    uint16_t questions = be16(&data[4]);
    uint16_t answers = be16(&data[6]);
    uint16_t offset = 12;

    for (uint16_t i = 0; i < questions; i++) {
        if (!skip_name(data, length, &offset) || offset > length || length - offset < 4) {
            return;
        }

        offset += 4;
    }

    for (uint16_t i = 0; i < answers; i++) {
        if (!skip_name(data, length, &offset) || offset > length || length - offset < 10) {
            return;
        }

        uint16_t type = be16(&data[offset]);
        uint16_t klass = be16(&data[offset + 2]);
        uint16_t rdlen = be16(&data[offset + 8]);
        offset += 10;

        if (rdlen > length - offset) {
            return;
        }

        if (type == 1 && klass == 1 && rdlen == 4) {
            for (int j = 0; j < 4; j++) {
                resolved_ip[j] = data[offset + j];
            }

            dns_ready = 1;
            dns_pending = 0;

            log_kernel("DNS: Resolved to %u.%u.%u.%u\n", (unsigned int)resolved_ip[0], (unsigned int)resolved_ip[1], (unsigned int)resolved_ip[2], (unsigned int)resolved_ip[3]);
            return;
        }

        offset += rdlen;
    }
}

int dns_get_result(uint8_t ip[4]) {
    if (!dns_ready || !ip) {
        return 0;
    }

    for (int i = 0; i < 4; i++) {
        ip[i] = resolved_ip[i];
    }

    return 1;
}