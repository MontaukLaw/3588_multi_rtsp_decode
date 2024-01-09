

#ifndef RK3588_DEMO_NN_MEDIA_H
#define RK3588_DEMO_NN_MEDIA_H

#include "types/error.h"
#include "types/media_dtype.h"

class NNMedia
{

public:
    nn_error_e SetInputType(nn_media_type_e type);
    nn_error_e SetOutputType(nn_media_type_e type);

    nn_error_e SetInputRTSP(const char *url);
    nn_error_e SetOutputRTSP(const char *url);

    nn_error_e SetInputFile(const char *path);

    nn_error_e GetSourceData();
    nn_error_e PushOutputData();

private:
};

#endif // RK3588_DEMO_NN_MEDIA_H
