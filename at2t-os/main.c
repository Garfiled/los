#define PAGE_SIZE 4096

long user_stack [ PAGE_SIZE>>2 ] ;
struct {
	long * a;
	short b;
} stack_start = { & user_stack [4096>>2] , 0x10 };

int printk(const char *fmt,...)
{
    return 0;
}
int main()
{
	char* video_memory = (char*)0xb8000;
	*video_memory = 'X';
	return 0;
}
