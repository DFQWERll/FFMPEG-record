
#include<libavutil/log.h>
#include<libavformat/avformat.h>


/* 
  h264 ------ decode ------- yuv

ps: h264可以从 mp4文件中抽取
  ffmpeg -i test.mp4 -codec copy -bsf:v h254_mp4toannexb -an out.h264


    1.分配packet空间
    AVPacket* pkt = av_packet_alloc();
    
    2.寻找解码器
    AVCodec* codec = avcode_find_decoder(AV_CODEC_ID_H264);

    3. 解码器解析上下文初始化
    AVCodecParserContext* parser = av_parser_init(codec->id);

    4.分配解码器上下文空间
    AVCodecContext*  c = avcodec_alloc_context3(codec);

    5.打开解码器
    avcodec_open2（c,codec,NULL);

    6.打开文件
    f=fopen(filename,"rb");

    7.分配frame空间
    AVFrame* frame = av_frame_alloc();

    8.循环读取文件
    fread(inbuf,1,INBUF_SIZE,f);

    9.解析文件，划分数据
     et = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
                            data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

    10.进行解码
        avcodec_send_packet(dec_ctx,pkt)
        avcodec_receive_frame(dec_ctx,frame)                      


编译：
gcc -g -o decode_h264 decode_h264.c `pkg-config --libs libavutil libavformat`

*/

#define INBUF_SIZE 4096

int main(int argc,char* argv[])
{
    AVPacket* pkt = NULL;
    AVCodec* codec = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVCodecParserContext* parser = NULL;
    AVFrame* frame = NULL;

    const char* filename,*outfilename;
    FILE* f;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t* data;
    size_t data_size;
    int ret = -1;

    av_log_set_level(AV_LOG_DEBUG);
    if(argc < 2)
    {
        av_log(NULL,AV_LOG_DEBUG,"usage:%s h264file outfilename\n",argv[0]);
        return -1;
    }

    filename = argv[1];
    outfilename = argv[2];

    //1. 分配packet空间
    pkt = av_packet_alloc();
    if(!pkt)
    {
        av_log(NULL,AV_LOG_ERROR,"AVPacket alloc failure\n");
        return -1;
    }

    //2.寻找解码器
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(!codec)
    {
        av_log(NULL,AV_LOG_ERROR,"find avcodec failure\n");
        return -1;
    }

    //3.初始化解析器
    parser = av_parser_init(codec->id);
    if(!parser)
    {
        av_log(NULL,AV_LOG_ERROR,"av parser failure\n");
        return -1;
    }

    //4.解码器上下文分配空间
    codec_ctx = avcodec_alloc_context3(codec);
    if(!codec_ctx)
    {
        av_log(NULL,AV_LOG_ERROR,"avcodec alloc failure\n");
        return -1;
    }

    
    //5.打开解码器
    ret = avcodec_open2(codec_ctx,codec,NULL);
    if(ret < 0 )
    {
        av_log(NULL,AV_LOG_ERROR,"avcodec open2 failure\n");
        return -1;
    }

    //6.打开文件
    f = fopen(filename,"rb");
    if(!f)
    {
        av_log(NULL,AV_LOG_ERROR,"open srcfile failure\n");
        return -1;
    }

    //7.分配Frame 空间
    frame = av_frame_alloc();
    
    av_log(NULL,AV_LOG_DEBUG,"frame alloc \n");

    //8.读文件
    while(!feof(f)){
            data_size = fread(inbuf,1,INBUF_SIZE,f); 
            if(!data_size)
            {
              break;
            }
            
            av_log(NULL,AV_LOG_DEBUG,"read file data_size =%d \n",data_size);

            data = inbuf;
            while (data_size > 0)
            {
                ret = av_parser_parse2(parser,codec_ctx,&pkt->data,&pkt->size,
                            data,data_size,AV_NOPTS_VALUE,AV_NOPTS_VALUE,0);
                if(ret < 0)
                {
                    av_log(NULL,AV_LOG_ERROR,"parser error\n");
                    return -1;
                }


                data += ret;
                data_size -= ret;

                if(pkt->size)
                {
                  //decoder
                    ret = avcodec_send_packet(codec_ctx,pkt);
                    if(ret <  0)
                    {
                        av_log(NULL,AV_LOG_ERROR,"senf packet decode error!\n");
                        return -1;
                    }

                    while (ret >= 0 )
                    {
                        ret  = avcodec_receive_frame(codec_ctx,frame);
                        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        {
                            break;
                        }
                        else if(ret < 0)
                        {
                            av_log(NULL,AV_LOG_ERROR,"receive frame  error!\n");
                            return -1;
                        }

                         printf(" frame %3d\n", codec_ctx->frame_number);
                         fflush(stdout);
                      
                      
                         //frame->data   中是原始数据，可以对原始数据进行渲染。注意数据是何种格式，packed plane sp.
                         //frame->format 表明了原始数据是何种格式，video数据对标AVPixelFormat  audio对标AVSampleFormat。
                    }
                }            
            }
            

    }

    return 0;

}
