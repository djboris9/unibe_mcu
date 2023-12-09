#ifndef COMM_H
#define COMM_H

enum locsvc_fifo_type {
    LOCSVC_FIFO_TYPE_GPS,
    LOCSVC_FIFO_TYPE_MAG,
};

struct locsvc_fifo_t {
    void *fifo_reserved; /* 1st word reserved for use by FIFO */
    enum locsvc_fifo_type type;
    void *data;
    int data_len;
};

#endif // COMM_H