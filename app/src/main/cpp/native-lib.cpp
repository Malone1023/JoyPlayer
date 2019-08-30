#include <jni.h>
#include <string>
#include <android/log.h>
#include <stdio.h>
#include <sys/stat.h>

extern "C" {
// 编解码
#include "libavcodec/avcodec.h"
// 封装格式
#include "libavformat/avformat.h"
// 渲染
#include "libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <unistd.h>
}

#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"LC",FORMAT,##__VA_ARGS__);

extern "C" JNIEXPORT void JNICALL
Java_com_joygames_joyplayer_VideoView_render(JNIEnv *env, jobject obj, jstring inputStr_,
                                             jobject surface) {

    const char *inputPath = env->GetStringUTFChars(inputStr_, 0);

    // 1. 注册组件
    av_register_all();
    LOGE("注册成功")

    // 2. 获取上下文
    AVFormatContext *avFormatContext = avformat_alloc_context();

    // 3. 打开输入文件并解复用
    if (int error = avformat_open_input(&avFormatContext, inputPath, NULL, NULL) < 0) {
        char buf[] = "";
        av_strerror(error, buf, 1024);
        LOGE("Couldn't open file %s: %d(%s)", inputPath, error, buf);
        LOGE("打开输入文件失败")
        return;
    }

    // 4. 获取编码流，并遍历出视频流
    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        LOGE("获取内容失败")
        return;
    }
    int video_index = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 标记视频流
            video_index = i;
            LOGE("成功找到视频流")
        }
    }

    AVCodecContext *avCodecContext = avFormatContext->streams[video_index]->codec;
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        LOGE("打开视频解码器失败")
        return;
    }

    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    AVFrame *frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    // 缓存区
    uint8_t *out_buffer = (uint8_t *) av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA,
                                                                   avCodecContext->width,
                                                                   avCodecContext->height));
    // 与缓存区相关联，设置rgb_frame缓存区
    avpicture_fill((AVPicture *) rgb_frame, out_buffer, AV_PIX_FMT_RGBA, avCodecContext->width,
                   avCodecContext->height);


    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_BICUBIC, NULL, NULL, NULL);

    // 获取 ANativeWindow
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == 0) {
        LOGE("nativewindow取到失败")
        return;
    }
    // 视频缓冲区
    ANativeWindow_Buffer native_outBuffer;


    int frameCount;
    int h = 0;
    LOGE("解码 ")
    while (av_read_frame(avFormatContext, packet) >= 0) {
        LOGE("解码 %d", packet->stream_index)
        LOGE("VINDEX %d", video_index)
        if (packet->stream_index == video_index) {
            LOGE("解码 hhhhh")
            // 如果是视频流，解码
            avcodec_decode_video2(avCodecContext, frame, &frameCount, packet);
            LOGE("解码中....  %d", frameCount)
            if (frameCount) {
                LOGE("转换并绘制")
                // 绘制之前配置的 ANativeWindow
                ANativeWindow_setBuffersGeometry(nativeWindow, avCodecContext->width,
                                                 avCodecContext->height, WINDOW_FORMAT_RGBA_8888);
                // 上锁
                ANativeWindow_lock(nativeWindow, &native_outBuffer, NULL);
                // 转换为rgb格式
                sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                          frame->height, rgb_frame->data,
                          rgb_frame->linesize);
                // rgb_frame是有画面数据
                uint8_t *dst = (uint8_t *) native_outBuffer.bits;
                // 拿到一行有多少个字节 RGBA
                int destStride = native_outBuffer.stride * 4;
                // 像素数据的首地址
                uint8_t *src = rgb_frame->data[0];
                // 实际内存一行数量
                int srcStride = rgb_frame->linesize[0];
                //int i=0;
                for (int i = 0; i < avCodecContext->height; ++i) {
                    // 将rgb_frame中每一行的数据复制给nativewindow
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                // 解锁
                ANativeWindow_unlockAndPost(nativeWindow);
                usleep(1000 * 5);
            }
        }
        av_free_packet(packet);
    }

    // 释放资源
    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);
    env->ReleaseStringUTFChars(inputStr_, inputPath);

}

extern "C" JNIEXPORT void JNICALL
Java_com_joygames_joyplayer_VideoView_play(JNIEnv *env, jobject obj, jstring inputStr_,
                                           jobject surface) {
    const char *inputPath = env->GetStringUTFChars(inputStr_, 0);

    // 打开输入文件
    FILE *f = fopen(inputPath, "rb");
    if (f == NULL) {
        LOGE("打开输入文件失败！");
        return;
    }

    // 获取文件大小
    struct stat info;
    stat(inputPath, &info);
    int size = info.st_size;

    // 读取文件到缓冲数组
    uint8_t *videoData = (uint8_t *) malloc(size);
    int readLength = fread(videoData, sizeof(uint8_t), size, f);

    // 关闭文件
    fclose(f);

    AVCodec *pCodec = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodecParserContext *pCodecParserCtx = NULL;
    AVCodecID codec_id = AV_CODEC_ID_H264;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();
    ANativeWindow *nativeWindow = NULL;
    ANativeWindow_Buffer native_outBuffer;
    SwsContext *swsContext;

    avcodec_register_all();
    // 初始化编解码器
    pCodec = avcodec_find_decoder(codec_id);
    if (!pCodec) {
        LOGE("找不到解码器！");
        return;
    }
    // 初始化编解码器上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        LOGE("分配视频编解码器上下文失败！");
        return;
    }
    // 初始化解析器上下文
    pCodecParserCtx = av_parser_init(codec_id);
    if (!pCodecParserCtx) {
        LOGE("分配视频解析器上下文失败！");
        return;
    }
    // 打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("打开解码器失败！");
        return;
    }
    // 初始化 AVPacket（装载视频流解析出来的帧）
    av_init_packet(&packet);
    // 初始化 ANativeWindow
    nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (!nativeWindow) {
        LOGE("初始化 ANativeWindow 失败！")
        return;
    }

    // 循环解码，并渲染
    int len, ret, frameCount;
    int first_time = 1;
    while (size) {
        len = av_parser_parse2(
                pCodecParserCtx, pCodecCtx,
                &packet.data, &packet.size,
                videoData, size,
                AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
        videoData += len;
        size -= len;

        if (packet.size == 0)
            continue;

        ret = avcodec_decode_video2(pCodecCtx, frame, &frameCount, &packet);
        if (ret < 0) {
            LOGE("解码失败！");
            return;
        }
        LOGE("width = %d, height = %d", pCodecCtx->width, pCodecCtx->height);

        if (frameCount) {
            if (first_time) {
                swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                            pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_BICUBIC, NULL, NULL, NULL);
                // 缓存区
                uint8_t *out_buffer = (uint8_t *) av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA,
                                                                               pCodecCtx->width,
                                                                               pCodecCtx->height));
                // 与缓存区相关联，设置rgb_frame缓存区
                avpicture_fill((AVPicture *) rgb_frame, out_buffer, AV_PIX_FMT_RGBA,
                               pCodecCtx->width, pCodecCtx->height);
                first_time = 0;
            }

            // 绘制之前配置的 ANativeWindow
            ANativeWindow_setBuffersGeometry(nativeWindow,
                                             pCodecCtx->width, pCodecCtx->height,
                                             WINDOW_FORMAT_RGBA_8888);
            // 上锁
            ANativeWindow_lock(nativeWindow, &native_outBuffer, NULL);
            // 转换为rgb格式
            sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                      frame->height, rgb_frame->data,
                      rgb_frame->linesize);
            // rgb_frame是有画面数据
            uint8_t *dst = (uint8_t *) native_outBuffer.bits;
            // 拿到一行有多少个字节 RGBA
            int destStride = native_outBuffer.stride * 4;
            // 像素数据的首地址
            uint8_t *src = rgb_frame->data[0];
            // 实际内存一行数量
            int srcStride = rgb_frame->linesize[0];
            //int i=0;
            for (int i = 0; i < pCodecCtx->height; ++i) {
                // 将rgb_frame中每一行的数据复制给nativewindow
                memcpy(dst + i * destStride, src + i * srcStride, srcStride);
            }
            // 解锁
            ANativeWindow_unlockAndPost(nativeWindow);
            usleep(1000);
        }
    }

    // 释放资源
    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    avcodec_close(pCodecCtx);
    av_parser_close(pCodecParserCtx);
    env->ReleaseStringUTFChars(inputStr_, inputPath);
}
