#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include "replace.h"
#include "interface.h"
#include "protocol.h"

extern "C" {

// this is not declared in qurt headers
void HAP_debug(const char *msg, int level, const char *filename, int line);
    
void HAP_printf(const char *file, int line, const char *format, ...)
{
	va_list ap;
        char buf[300];
        
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
    HAP_debug(buf, 0, file, line);
    //usleep(20000);
}
    
/**
   QURT doesn't have strnlen()
**/
size_t strnlen(const char *s, size_t max)
{
        size_t len;
  
        for (len = 0; len < max; len++) {
                if (s[len] == '\0') {
                        break;
                }
        }
        return len;  
}

int vasprintf(char **ptr, const char *format, va_list ap)
{
	int ret;
	va_list ap2;

	va_copy(ap2, ap);
	ret = vsnprintf(nullptr, 0, format, ap2);
	va_end(ap2);
	if (ret < 0) return ret;

	(*ptr) = (char *)malloc(ret+1);
	if (!*ptr) return -1;

	va_copy(ap2, ap);
	ret = vsnprintf(*ptr, ret+1, format, ap2);
	va_end(ap2);

	return ret;
}

int asprintf(char **ptr, const char *format, ...)
{
	va_list ap;
	int ret;
	
	*ptr = nullptr;
	va_start(ap, format);
	ret = vasprintf(ptr, format, ap);
	va_end(ap);

	return ret;
}

void *memmem(const void *haystack, size_t haystacklen,
                    const void *needle, size_t needlelen)
{
	if (needlelen == 0) {
        return const_cast<void*>(haystack);
	}
	while (haystacklen >= needlelen) {
		char *p = (char *)memchr(haystack, *(const char *)needle,
                                 haystacklen-(needlelen-1));
		if (!p) return NULL;
		if (memcmp(p, needle, needlelen) == 0) {
			return p;
		}
		haystack = p+1;
		haystacklen -= (p - (const char *)haystack) + 1;
	}
	return NULL;
}

char *strndup(const char *s, size_t n)
{
	char *ret;
	
    n = strnlen(s, n);
    ret = (char*)malloc(n+1);
    if (!ret) {
        return NULL;
    }
	memcpy(ret, s, n);
	ret[n] = 0;

	return ret;
}

int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *attr)
{
       return 0;
}

// INVESTIGATE: What is this needed on QURT?
int apfs_rename(const char *oldpath, const char *newpath)
{
       return 0;
}

// INVESTIGATE: How to enable
void lua_abort() {}
const char* lua_get_modules_path() {return NULL;}
int lua_get_current_ref() {return 0;}

// INVESTIGATE: Seems important :-)
int ArduPilot_main(int argc, const char *argv[])
{
       return 0;
}

}

extern "C" int qurt_arducopter_main(int argc, char* const argv[]);

int slpi_link_client_init(void)
{
	HAP_PRINTF("About to call qurt_arducopter_main %p", &qurt_arducopter_main);

	qurt_arducopter_main(0, NULL);

    HAP_PRINTF("qurt_arducopter_main RETURNED");

    return 0;
}

typedef void (*mavlink_data_callback_t)(const uint8_t *data, int len, void* p);
mavlink_data_callback_t mav_cb = NULL;
void *mav_cb_ptr = nullptr;
void register_mavlink_data_callback(mavlink_data_callback_t func, void *p) {
	mav_cb = func;
	mav_cb_ptr = p;
}

int slpi_link_client_receive(const uint8_t *data, int data_len_in_bytes)
{
    // HAP_PRINTF("slpi_link_client_receive: %d bytes", data_len_in_bytes);

	if (data_len_in_bytes != 0) {
		uint8_t msg_id = data[0];
		switch (msg_id) {
		case QURT_MSG_ID_TEST_MSG:
		{
			HAP_PRINTF("Got test message");
			qurt_test_msg msg;
			memcpy((void*) &msg, (void*) data, data_len_in_bytes);
			HAP_PRINTF("Parsing struct test_msg");
			HAP_PRINTF("msg_id = 0x%x", msg.msg_id);
			HAP_PRINTF("byte_field = 0x%x", msg.byte_field);
			HAP_PRINTF("word16_field = 0x%x", msg.word16_field);
			HAP_PRINTF("word32_field = 0x%lx", msg.word32_field);
			HAP_PRINTF("word64_field = 0x%llx", msg.word64_field);
			HAP_PRINTF("float_field = %f", msg.float_field);
			HAP_PRINTF("double_field = %f", msg.double_field);
			break;
		}
		case QURT_MSG_ID_MAVLINK_MSG:
		{
			// HAP_PRINTF("Got mavlink message");
			// HAP_PRINTF("0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x",
			// 		data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12]);
			if (mav_cb) mav_cb(&data[1], data_len_in_bytes - 1, mav_cb_ptr);
			break;
		}
		default:
			HAP_PRINTF("Got unknown message id %d", msg_id);
			break;
		}
	}

    return 0;
}

int __wrap_printf(const char *fmt, ...)
{
   va_list ap;

   char buf[300];
   va_start(ap, fmt);
   vsnprintf(buf, sizeof(buf), fmt, ap);
   va_end(ap);
   HAP_PRINTF(buf);
   qurt_timer_sleep(5000);

   return 0;
}
