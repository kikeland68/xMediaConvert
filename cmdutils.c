/*
 * Various utilities for command line tools
 * Copyright (c) 2000-2003 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

/* Include only the enabled headers since some compilers (namely, Sun
   Studio) will not omit unused inline functions and create undefined
   references to libraries that are not being built. */

#include "config.h"
#include "libavformat/avformat.h"
#ifdef CONFIG_AVFILTER
#include "libavfilter/avfilter.h"
#endif
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#ifdef CONFIG_POSTPROC
#include "libpostproc/postprocess.h"
#endif
#include "libavutil/avstring.h"
#include "libavcodec/opt.h"
#include "cmdutils.h"
#ifdef CONFIG_NETWORK
#include "libavformat/network.h"
#endif

#undef exit

const char **opt_names;
static int opt_name_count;
AVCodecContext *avctx_opts[CODEC_TYPE_NB];
AVFormatContext *avformat_opts;
struct SwsContext *sws_opts;

double parse_number_or_die(const char *context, const char *numstr, int type, double min, double max)
{
    char *tail;
    const char *error;
    double d = strtod(numstr, &tail);
    if (*tail)
        error= "Expected number for %s but found: %s\n";
    else if (d < min || d > max)
        error= "The value for %s was %s which is not within %f - %f\n";
    else if(type == OPT_INT64 && (int64_t)d != d)
        error= "Expected int64 for %s but found %s\n";
    else
        return d;
    fprintf(stderr, error, context, numstr, min, max);
    exit(1);
}

int64_t parse_time_or_die(const char *context, const char *timestr, int is_duration)
{
    int64_t us = parse_date(timestr, is_duration);
    if (us == INT64_MIN) {
        fprintf(stderr, "Invalid %s specification for %s: %s\n",
                is_duration ? "duration" : "date", context, timestr);
        exit(1);
    }
    return us;
}

void show_help_options(const OptionDef *options, const char *msg, int mask, int value)
{
    const OptionDef *po;
    int first;

    first = 1;
    for(po = options; po->name != NULL; po++) {
        char buf[64];
        if ((po->flags & mask) == value) {
            if (first) {
                printf("%s", msg);
                first = 0;
            }
            av_strlcpy(buf, po->name, sizeof(buf));
            if (po->flags & HAS_ARG) {
                av_strlcat(buf, " ", sizeof(buf));
                av_strlcat(buf, po->argname, sizeof(buf));
            }
            printf("-%-17s  %s\n", buf, po->help);
        }
    }
}

static const OptionDef* find_option(const OptionDef *po, const char *name){
    while (po->name != NULL) {
        if (!strcmp(name, po->name))
            break;
        po++;
    }
    return po;
}

void parse_options(int argc, char **argv, const OptionDef *options,
                   void (* parse_arg_function)(const char*))
{
    const char *opt, *arg;
    int optindex, handleoptions=1;
    const OptionDef *po;

    /* parse options */
    optindex = 1;
    while (optindex < argc) {
        opt = argv[optindex++];

        if (handleoptions && opt[0] == '-' && opt[1] != '\0') {
          if (opt[1] == '-' && opt[2] == '\0') {
            handleoptions = 0;
            continue;
          }
            po= find_option(options, opt + 1);
            if (!po->name)
                po= find_option(options, "default");
            if (!po->name) {
unknown_opt:
                fprintf(stderr, "%s: unrecognized option '%s'\n", argv[0], opt);
                exit(1);
            }
            arg = NULL;
            if (po->flags & HAS_ARG) {
                arg = argv[optindex++];
                if (!arg) {
                    fprintf(stderr, "%s: missing argument for option '%s'\n", argv[0], opt);
                    exit(1);
                }
            }
            if (po->flags & OPT_STRING) {
                char *str;
                str = av_strdup(arg);
                *po->u.str_arg = str;
            } else if (po->flags & OPT_BOOL) {
                *po->u.int_arg = 1;
            } else if (po->flags & OPT_INT) {
                *po->u.int_arg = parse_number_or_die(opt+1, arg, OPT_INT64, INT_MIN, INT_MAX);
            } else if (po->flags & OPT_INT64) {
                *po->u.int64_arg = parse_number_or_die(opt+1, arg, OPT_INT64, INT64_MIN, INT64_MAX);
            } else if (po->flags & OPT_FLOAT) {
                *po->u.float_arg = parse_number_or_die(opt+1, arg, OPT_FLOAT, -1.0/0.0, 1.0/0.0);
            } else if (po->flags & OPT_FUNC2) {
                if(po->u.func2_arg(opt+1, arg)<0)
                    goto unknown_opt;
            } else {
                po->u.func_arg(arg);
            }
            if(po->flags & OPT_EXIT)
                exit(0);
        } else {
            if (parse_arg_function)
                parse_arg_function(opt);
        }
    }
}

int opt_default(const char *opt, const char *arg){
    int type;
    const AVOption *o= NULL;
    int opt_types[]={AV_OPT_FLAG_VIDEO_PARAM, AV_OPT_FLAG_AUDIO_PARAM, 0, AV_OPT_FLAG_SUBTITLE_PARAM, 0};

    for(type=0; type<CODEC_TYPE_NB; type++){
        const AVOption *o2 = av_find_opt(avctx_opts[0], opt, NULL, opt_types[type], opt_types[type]);
        if(o2)
            o = av_set_string2(avctx_opts[type], opt, arg, 1);
    }
    if(!o)
        o = av_set_string2(avformat_opts, opt, arg, 1);
    if(!o)
        o = av_set_string2(sws_opts, opt, arg, 1);
    if(!o){
        if(opt[0] == 'a')
            o = av_set_string2(avctx_opts[CODEC_TYPE_AUDIO], opt+1, arg, 1);
        else if(opt[0] == 'v')
            o = av_set_string2(avctx_opts[CODEC_TYPE_VIDEO], opt+1, arg, 1);
        else if(opt[0] == 's')
            o = av_set_string2(avctx_opts[CODEC_TYPE_SUBTITLE], opt+1, arg, 1);
    }
    if(!o)
        return -1;

//    av_log(NULL, AV_LOG_ERROR, "%s:%s: %f 0x%0X\n", opt, arg, av_get_double(avctx_opts, opt, NULL), (int)av_get_int(avctx_opts, opt, NULL));

    //FIXME we should always use avctx_opts, ... for storing options so there will not be any need to keep track of what i set over this
    opt_names= av_realloc(opt_names, sizeof(void*)*(opt_name_count+1));
    opt_names[opt_name_count++]= o->name;

    if(avctx_opts[0]->debug || avformat_opts->debug)
        av_log_set_level(AV_LOG_DEBUG);
    return 0;
}

void set_context_opts(void *ctx, void *opts_ctx, int flags)
{
    int i;
    for(i=0; i<opt_name_count; i++){
        char buf[256];
        const AVOption *opt;
        const char *str= av_get_string(opts_ctx, opt_names[i], &opt, buf, sizeof(buf));
        /* if an option with name opt_names[i] is present in opts_ctx then str is non-NULL */
        if(str && ((opt->flags & flags) == flags))
            av_set_string2(ctx, opt_names[i], str, 1);
    }
}

void print_error(const char *filename, int err)
{
    switch(err) {
    case AVERROR_NUMEXPECTED:
        fprintf(stderr, "%s: Incorrect image filename syntax.\n"
                "Use '%%d' to specify the image number:\n"
                "  for img1.jpg, img2.jpg, ..., use 'img%%d.jpg';\n"
                "  for img001.jpg, img002.jpg, ..., use 'img%%03d.jpg'.\n",
                filename);
        break;
    case AVERROR_INVALIDDATA:
        fprintf(stderr, "%s: Error while parsing header\n", filename);
        break;
    case AVERROR_NOFMT:
        fprintf(stderr, "%s: Unknown format\n", filename);
        break;
    case AVERROR(EIO):
        fprintf(stderr, "%s: I/O error occurred\n"
                "Usually that means that input file is truncated and/or corrupted.\n",
                filename);
        break;
    case AVERROR(ENOMEM):
        fprintf(stderr, "%s: memory allocation error occurred\n", filename);
        break;
    case AVERROR(ENOENT):
        fprintf(stderr, "%s: no such file or directory\n", filename);
        break;
#ifdef CONFIG_NETWORK
    case AVERROR(FF_NETERROR(EPROTONOSUPPORT)):
        fprintf(stderr, "%s: Unsupported network protocol\n", filename);
        break;
#endif
    default:
        fprintf(stderr, "%s: Error while opening file\n", filename);
        break;
    }
}

#define PRINT_LIB_VERSION(outstream,libname,LIBNAME,indent) \
    version= libname##_version(); \
    fprintf(outstream, "%slib%-10s %2d.%2d.%2d / %2d.%2d.%2d\n", indent? "  " : "", #libname, \
            LIB##LIBNAME##_VERSION_MAJOR, LIB##LIBNAME##_VERSION_MINOR, LIB##LIBNAME##_VERSION_MICRO, \
            version >> 16, version >> 8 & 0xff, version & 0xff);

static void print_all_lib_versions(FILE* outstream, int indent)
{
    unsigned int version;
    PRINT_LIB_VERSION(outstream, avutil,   AVUTIL,   indent);
    PRINT_LIB_VERSION(outstream, avcodec,  AVCODEC,  indent);
    PRINT_LIB_VERSION(outstream, avformat, AVFORMAT, indent);
    PRINT_LIB_VERSION(outstream, avdevice, AVDEVICE, indent);
#ifdef CONFIG_AVFILTER
    PRINT_LIB_VERSION(outstream, avfilter, AVFILTER, indent);
#endif
#ifdef CONFIG_SWSCALE
    PRINT_LIB_VERSION(outstream, swscale,  SWSCALE,  indent);
#endif
#ifdef CONFIG_POSTPROC
    PRINT_LIB_VERSION(outstream, postproc, POSTPROC, indent);
#endif
}
