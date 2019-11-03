#include <linux/ioctl.h>
#include"pro.h"

#define MAGIC_NUMBER 				'K'
#define HT_IOCTL_BASE				0x00



#define	HT_530_READ_KEY							_IOWR(MAGIC_NUMBER, HT_IOCTL_BASE+1, int /*struct _HT_530_READ_KEY*/)
#define DUMP_IOCTL									_IOWR(MAGIC_NUMBER, HT_IOCTL_BASE+2, struct _DUMP_ARG)


	////////////////////////IOCTL STRUCTURE DEFINITION////////////////////////
/*typedef struct _HT_530_READ_KEY
{
	int RetVal;
	struct
	{
		int in_key;
	}in;
}SHT_530_READ_KEY, *PSHT_530_READ_KEY;
*/
typedef struct _DUMP_ARG
{
	int RetVal;
	struct
	{
		int in_n;
	}in;
	struct
	{
		ht_obj_t out_object_array[8];
	}out;
}dump_arg, *Pdump_arg;
	
