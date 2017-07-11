/****************************************************************************
Most of this source has been taken from the website: https://github.com/pornel/dssim
and adapted to fit the purpose of the HWC Validation Team.

File Name:      SSIMUtils.cpp

Description:    Set of utilities in order to evaluate the SSIM index of two images.

Environment:

Notes:

****************************************************************************/

#define CHANS 3             //number of color channels
#define REGULAR_BLUR_RAY 1  // ray of the regular blur
#define TRANS_BLUR_RAY 4    // ray of the transposing blur
#define BYTES_PER_PIXEL 4   // self-explained
#define SIGMA 3             // sigma ~ gaussian radius
                            // the gaussian kernel length is (6*sigma-1)

static double gamma_lut[256];
static const double D65x = 0.9505, D65y = 1.0, D65z = 1.089;

typedef void rowcallback(float *, const int width);

typedef struct
{
    unsigned char r, g, b, a;
} dssim_rgba;

typedef struct
{
    float l, A, b, a;
} laba;

typedef struct
{
    int width, height;
    float *img1, *mu1, *sigma1_sq;
    float *img2, *mu2, *sigma2_sq, *sigma12;
} dssim_info_chan;

struct dssim_info
{
    dssim_info_chan chan[CHANS];
};

enum BlurType
{
    ebtLinear,
    ebtGaussian
};

void DoSSIMCalculations(dssim_info *inf,
                        dssim_rgba *Bufrow_pointers[],
                        dssim_rgba *Refrow_pointers[],
                        const int width,
                        const int height,
                        const int blur_type,
                        bool hasAlpha);

double GetSSIMIndex(dssim_info_chan *chan);
