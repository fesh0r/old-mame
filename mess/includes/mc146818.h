
typedef enum {
	MC146818_STANDARD,
	MC146818_IGNORE_CENTURY, // century is NOT set, for systems having other usage of this byte
	MC146818_ENHANCED 
} MC146818_TYPE;

void mc146818_init(MC146818_TYPE type);
void mc146818_close(void);

READ_HANDLER(mc146818_port_r);
WRITE_HANDLER(mc146818_port_w);
