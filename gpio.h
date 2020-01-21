#define BUFFER_SIZE 64

#define MODE_WRITE 1
#define MODE_READ 2

#define E_RISING 0
#define E_FALLING 1
#define E_BOTH 2
#define E_NONE 3


int gpio_init(int,int);
int gpio_close(int);
int gpio_read(int);
int gpio_write(int,int);
int gpio_edge_prepare(int,int);
int wait_for_edge(int);
int wait_for_edge_of(int);
