#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ps5/kernel.h>

typedef struct notify_request {
    char useless1[45];
    char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int, notify_request_t*, size_t, int);

static void notify(const char *msg) {
    notify_request_t req;
    bzero(&req, sizeof(req));
    strncpy(req.message, msg, sizeof(req.message));
    sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}

#define DDD_GET_STATUS      0xC0284406

struct ddd_ioctl {
    uint8_t  pad[4];
    int32_t  in_size;
    uint8_t *in_buf;
    int32_t  out_size;
    uint8_t *out_buf;
    uint32_t status;
    uint32_t pad2;
};

static uint32_t ddd_call(int fd, unsigned long code, uint8_t *in, int in_size,
                         uint8_t *out, int out_size) {
    struct ddd_ioctl arg;
    memset(&arg, 0, sizeof(arg));
    arg.in_size = in_size;
    arg.in_buf = in;
    arg.out_size = out_size;
    arg.out_buf = out;

    int ret = ioctl(fd, code, &arg);
    if (ret != 0) {
        printf("  ioctl failed: %s (errno=%d)\n", strerror(errno), errno);
        return 0xFFFFFFFF;
    }
    return arg.status;
}



int main(void) {
    pid_t mypid = getpid();
    kernel_set_ucred_authid(mypid, 0x4800000000000010ULL);
    uint8_t caps[16];
    kernel_get_ucred_caps(mypid, caps);
    caps[7] |= 0x40;
    kernel_set_ucred_caps(mypid, caps);
    setuid(0);

    int ddd_fd = open("/dev/ddd", O_RDWR);
    if (ddd_fd < 0) {
        printf("/dev/ddd: %s\n", strerror(errno));
        return 1;
    }

    uint8_t status_buf[16];
    memset(status_buf, 0, sizeof(status_buf));

    uint32_t st = ddd_call(ddd_fd, DDD_GET_STATUS, NULL, 0, status_buf, 4);
    printf("status: 0x%08x\n", st);
    
    uint32_t ps = *(uint32_t *)status_buf;
    const char *names[] = {
        "PAIRED", "PAIRED_TO_ANOTHER", "NOT_PAIRED",
        "REGISTER_INCOMPLETE", "DELETE_INCOMPLETE",
        "FACTORY_PAIRED", "FACTORY_PAIRED_TO_ANOTHER"
    };
    FILE *fptr = fopen("/mnt/usb0/ddd.txt", "wb");
    if (st) {
        notify("digital (no drive)");
    }
    else{
        
        char *msg = NULL;
        if (ps < 7) asprintf(&msg, "DDD Status: %u (%s)",ps, names[ps]);
        notify(msg);
        
        fwrite(msg, sizeof(msg), 1, fptr);
        printf("%s\n",msg);
        free(msg);
        msg = NULL;
    }
    fclose(fptr);
    close(ddd_fd);
    return 0;
}
