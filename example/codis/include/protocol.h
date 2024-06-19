// database protocol
#define N_BUCKETS 0xFF // 255
#define BUCKET_SIZE 0x1000 // page size

// first 8 bits in ipc buff is command number
#define COMMAND_IPC_IDX 0
// next 8 bits is bucket number
#define COMMAND_BUCKET_IDX 1
// the rest is the string input (if applicable)
#define COMMAND_DATA_START_IDX 2

// commands:
#define READ_CMD 0
#define APPEND_CMD 1
#define APPEND_CMD_MAX_PAYLOAD_LEN (BUCKET_SIZE - 2)

#define DELETE_CMD 2

// return:
// cmd 0: string occupy entire ipc buff
// cmd 1: first 16 bits indicate new length of the string in bucket, zero if the 
//        concatenation would cause the string to overflow the bucket.
// cmd 2: first 16 bits is a flag, non zero means the string was deleted successfully.
