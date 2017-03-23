#include <jni.h>
#include <string>
#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "Eason", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "Eason", format, ##__VA_ARGS__)

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include<sys/time.h>
#include <pthread.h>
//#include "simpltest_ffmpeg_decoder.h"

int ffmpeg_main(void)
{
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame,*pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	FILE *fp_yuv;
	int frame_cnt;
	clock_t time_start, time_finish;
    struct  timeval frame_start,frame_end,frame_scale;
	double  time_duration = 0.0;

	//char input_str[500]="rtsp://192.168.0.108:8553/erbx.mkv";
    char input_str[500]="/sdcard/kunvely.mp4";
    char output_str[500]="/sdcard/yuv/mp4_yuv";
	char info[1000]={0};
	/*sprintf(input_str,"%s",(*env)->GetStringUTFChars(env,input_jstr, NULL));
	sprintf(output_str,"%s",(*env)->GetStringUTFChars(env,output_jstr, NULL));
*/
	//FFmpeg av_log() callback
  //av_log_set_callback(custom_log);
    fp_yuv=fopen(output_str,"wb+");
    if(fp_yuv==NULL){
        LOGE("Cannot open output file.\n");
        return -1;
    }

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
    int rt;
    char buf[1000]={0};
    rt=avformat_open_input(&pFormatCtx,input_str,NULL,NULL);
	if(rt!=0){
        av_strerror(rt, buf, 1000);
		LOGE("Couldn't open input stream：%s:code=%d  (%s)\n",input_str,rt,buf);
		return -1;
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){
		LOGE("Couldn't find stream information.\n");
		return -1;
	}
	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			break;
		}
	if(videoindex==-1){
		LOGE("Couldn't find a video stream.\n");
		return -1;
	}
	pCodecCtx=pFormatCtx->streams[videoindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL){
		LOGE("Couldn't find Codec.\n");
		return -1;
	}
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		LOGE("Couldn't open codec.\n");
		return -1;
	}

	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();
	out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,  pCodecCtx->width, pCodecCtx->height,1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize,out_buffer,
		AV_PIX_FMT_YUV420P,pCodecCtx->width, pCodecCtx->height,1);


	packet=(AVPacket *)av_malloc(sizeof(AVPacket));

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
	pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


  sprintf(info,   "[Input     ]%s\n", input_str);
  sprintf(info, "%s[Output    ]%s\n",info,output_str);
  sprintf(info, "%s[Format    ]%s\n",info, pFormatCtx->iformat->name);
  sprintf(info, "%s[Codec     ]%s\n",info, pCodecCtx->codec->name);
  sprintf(info, "%s[Resolution]%dx%d\n",info, pCodecCtx->width,pCodecCtx->height);

	frame_cnt=0;
	time_start = clock();

	while(av_read_frame(pFormatCtx, packet)>=0){
		if(packet->stream_index==videoindex){
            gettimeofday(&frame_start,NULL);
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if(ret < 0){
				LOGE("Decode Error.\n");
				return -1;
			}
			if(got_picture){
#if 0
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);

				y_size=pCodecCtx->width*pCodecCtx->height;
				/*fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
				fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
				fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V*/
#endif
				//Output info
				char pictype_str[10]={0};
				switch(pFrame->pict_type){
					case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
				  case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
					case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
					default:sprintf(pictype_str,"Other");break;
				}
                gettimeofday(&frame_end,NULL);
                int sec=frame_end.tv_sec-frame_start.tv_sec;
                int ms=(int)(frame_end.tv_usec-frame_start.tv_usec)/1000;
                int mstime_duration=(sec*1000)+ms;
				LOGI("Decoder Frame Index: %5d. Type:%s,time=%d ms",frame_cnt,pictype_str,mstime_duration);
				frame_cnt++;
                if(frame_cnt>2000)
                    break;
			}
		}
		av_free_packet(packet);
	}
	//flush decoder
	//FIX: Flush Frames remained in Codec
	while (1) {
        //frame_end=clock();
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if (ret < 0)
			break;
		if (!got_picture)
			break;
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
			pFrameYUV->data, pFrameYUV->linesize);
		int y_size=pCodecCtx->width*pCodecCtx->height;
		/*fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
		fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
		fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V*/
		//Output info
		char pictype_str[10]={0};
		switch(pFrame->pict_type){
			case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
		  case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
			case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
			default:sprintf(pictype_str,"Other");break;
		}
        //frame_end=clock();
		LOGI("Decoder Remained Frame Index: %5d. Type:%s,time=%d",frame_cnt,pictype_str,frame_end.tv_usec-frame_start.tv_usec);
		frame_cnt++;
        if(frame_cnt>2000)
            break;
	}
	time_finish = clock();
	time_duration=(double)(time_finish - time_start)/CLOCKS_PER_SEC;

    LOGI("=====================total time=%f,frame_cnt=%d",time_duration,frame_cnt);

	sprintf(info, "%s[Time      ]%fms\n",info,time_duration);
	sprintf(info, "%s[Count     ]%d\n",info,frame_cnt);

	sws_freeContext(img_convert_ctx);

  fclose(fp_yuv);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}

void * ffmpeg_thread(void *arg)
{
    ffmpeg_main();
}

JNIEXPORT jstring JNICALL
Java_com_example_eason_ffmpeg_1test_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    pthread_t id_1;
    LOGI("FFMPEG START\n");
    int ret=-2;

    ret=pthread_create(&id_1,NULL,ffmpeg_thread,NULL);
    if(ret!=0)
    {
        LOGI("Create pthread error!\n");
    }
    //ret=ffmpeg_main();
    LOGI("FFMPEG END\n");
    char chr[1000]={0};
    std::string hello = "Hello eason from  ffmpeg test";
    sprintf(chr,"Hello eason from  ffmpeg test  ret=%d\n",ret);
    return env->NewStringUTF(chr);
}
}