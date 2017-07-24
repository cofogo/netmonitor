#define NOT_READY -1
#define FILLED 0
#define TAKEN 1

struct Sh_mem_data {
    int status;
    char e_link_data[1024];
    char e_addr_data[1024];
    char w_link_data[1024];
    char w_addr_data[1024];
};
