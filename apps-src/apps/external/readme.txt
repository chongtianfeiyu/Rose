bzip2
======
bzip2-1.0.6

boost
======
1_76_0(<boost>/boost/version.hpp)
----
��Ҫ��boost::function/boost::bind������std::function/std::bind����������bind�����̽ϴ��鷳��C++11��֧��std::function/std::bind��

gettext
=====
gettext include gettext and libiconv directory. compile require Preprocessor Definitions 
-------------------------
XCode(iOS/Mac OS X)
_LIBICONV_H   avoid include system defined <iconv.h>
---------------------------

jpeg-9b
=====
ΪʲôҪ����jpeg��webrtcʱ��������ͷ������ͼ����mjpeg��ʽ��windows����Ҫ������������ʽ��Android��Ϊ���ģ������ˣ����ذ���jpeg��

--Preprocessor Definitions
  HAVE_JPEG AVOID_TABLES(Android��Ҫ)
Ҫ�����Դ�ļ��ص㣺1��һ������j��ʼ��2��û��jpegtran.c��3��jmem��ʼ���ļ��У�һ����Ҫjmemmgr.c��windows��jmemnobs.c��android��jmem-android
Ϊ����include����һ��Ŀ¼���޸�<src>\apps\external\third_party\libyuv\source\mjpeg_decoder.cc��
#include <jpeglib.h> ==> #include <jpeg-9b/jpeglib.h>

protobuf
=====
˵����http://www.libsdl.cn/bbs/forum.php?mod=viewthread&tid=149&extra=page%3D1

third_party/ffmpeg
======
Only header. Place here instead with linker/include, because webrtc quire this location.

zlib
======
zlib1.2.8

zxing
=====
zxing����https://github.com/glassechidna/zxing-cpp), Ϊʲôû����https://github.com/nu-book/zxing-cpp? --ʵ����������2017��MacBook Pro�ϡ�

ʶ��1280x720��Ƶ����û��ά�룩��nu-book/zxing-cpp��ÿ֡ƽ������80���루fast=falseʱ��Ҫ300����룩��glassechidna/zxing-cpp��ƽ��70���롣
ѭ��ʶ��ͬһ��960x1280ͼ�񣨶�ά�룺260x260����nu-book/zxing-cpp��ƽ��55���롣glassechidna/zxing-cpp��ƽ��75���롣
ѭ��ʶ��ͬһ��260x260ͼ�������Ƕ�ά�룬����ת����nu-book/zxing-cpp��ƽ��6���롣glassechidna/zxing-cpp��ƽ��5���롣
ѭ��ʶ��ͬһ��260x260ͼ�������Ƕ�ά�룬�����ţ���nu-book/zxing-cpp��ƽ��6���롣glassechidna/zxing-cpp��ƽ��5���롣

��ͼ����û�ж�ά��ʱ��glassechidna/zxing-cppҪ��nu-book��10�������ͼ�����ж�ά�룬����nu-book���ٲ����glassechidna������������ڳɹ�ʶ�����ϣ�nu-book������Ҫ�ù�glassechidna��Ϊʹ��nu-book������Ⱦ�׼�۳���ά�룬��Ἣ���

A��Ϊʲô��һ��cropʱҪ������10���ء�
Q����ת���û�в��ֻ��С���ɫ����䣬���ƻ���Щ����ɫ��������һ��������

���Һ����ƣ���android��ֱ�Ӱ�����ͷͼ������nu-book/zxing-cppʱ������ʱ����45���룬��Ϊ3�����׶Σ�Ӧ�û������ܡ�Ϊ�ˣ���QrParse::FindQrPointЧ��û������nu-book/zxing-cpp����ʱ����ֱ������nu-book/zxing-cpp��Ϊ�����㷨��

SDL/libvpx
=====
���õ���汾��ios����ܷ�����������Ŀǰ������vp8��vp9��һ��Ҫ��vp8��vp9��һ��Ҫ����webrtc����������libvpx��
