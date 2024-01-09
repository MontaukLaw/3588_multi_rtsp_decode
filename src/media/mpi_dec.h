
#ifndef RK3588_DEMO_MPI_DEC_H
#define RK3588_DEMO_MPI_DEC_H

#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpp_frame.h"
#include "mpp_buffer_impl.h"
#include "mpp_frame_impl.h"

#define MPI_DEC_STREAM_SIZE (SZ_4K)
#define MPI_DEC_LOOP_COUNT 4
#define MAX_FILE_NAME_LENGTH 256

typedef struct
{
    MppCtx ctx;
    MppApi *mpi;
    RK_U32 eos;
    char *buf;

    MppBufferGroup frm_grp;
    MppBufferGroup pkt_grp;
    MppPacket packet;
    size_t packet_size;
    MppFrame frame;

    FILE *fp_input;
    FILE *fp_output;
    RK_S32 frame_count;
    RK_S32 frame_num;
    size_t max_usage;
} MpiDecLoopData;

typedef struct
{
    char file_input[MAX_FILE_NAME_LENGTH];
    char file_output[MAX_FILE_NAME_LENGTH];
    MppCodingType type;
    MppFrameFormat format;
    RK_U32 width;
    RK_U32 height;
    RK_U32 debug;

    RK_U32 have_input;
    RK_U32 have_output;

    RK_U32 simple;
    RK_S32 timeout;
    RK_S32 frame_num;
    size_t max_usage;
} MpiDecTestCmd;

#endif // RK3588_DEMO_MPI_DEC_H
