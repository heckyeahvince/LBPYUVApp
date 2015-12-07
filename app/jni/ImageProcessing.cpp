#include "io_github_melvincabatuan_lbpyuv_MainActivity.h"
#include <android/bitmap.h>
#include <stdlib.h>

#include <opencv2/imgproc/imgproc.hpp>


#define  LOG_TAG    "LBP-YUV"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
 

 
// Color Features
void extractVU( const cv::Mat &image, cv::Mat &V, cv::Mat &U ){

	int nRows = image.rows;   // number of lines
    int nCols = image.cols;   // number of columns  

    /// Convert to 1D array if Continuous
    if (image.isContinuous()) {
        nCols = nCols * nRows;
		nRows = 1;  
	}   

    for (int j=0; j<nRows; j++) {        
        uchar* data = reinterpret_cast<uchar*>(image.data);
        uchar* colorV = reinterpret_cast<uchar*>(V.data);
        uchar* colorU = reinterpret_cast<uchar*>(U.data);

		for (int i = 0; i < nCols; i += 2) {
		        // assign each pixel to V and U
                *colorV++ = *data++; //  [16,236]
                *colorU++ = *data++; //  [16,236]   
        }
    }
}



// Texture Features
cv::Mat getLBP(const cv::Mat &img){

    CV_Assert(img.depth() == CV_8U); // accept only uchar images

    cv::Mat dst;
    
    if (dst.empty())
        dst = cv::Mat::zeros(img.size(), img.type());

    for(  int j=1; j<img.rows-1; j++) {
    
        const uchar* previous = img.ptr<const uchar>(j-1); // previous row
        const uchar* current  = img.ptr<const uchar>(j);   // current row 
        const uchar* next     = img.ptr<const uchar>(j+1); // next row
        
        uchar* output          = dst.ptr<uchar>(j); // output row       
        
        for(int i = 1;i < img.cols-1; i++) {
            
            const uchar center = img.ptr<const uchar>(j)[i];
            
            uchar code = 0;

            code |= ((previous[i-1]) > center) << 7;
            code |= ((previous[i])   > center) << 6;
            code |= ((previous[i+1]) > center) << 5;
            code |= ((current[i+1])  > center) << 4;
            code |= ((next[i+1])     > center) << 3;
            code |= ((next[i])       > center) << 2;
            code |= ((next[i-1])     > center) << 1;
            code |= ((current[i-1])  > center) << 0;
            
            *output++ = code;
        }
    }
    return dst;
}




cv::Mat imageU, imageV;
cv::Mat LBP;

std::vector<cv::Mat> channels(4);
 


/*
 * Class:     io_github_melvincabatuan_yuv_MainActivity
 * Method:    process
 * Signature: (Landroid/graphics/Bitmap;[B)V
 */
JNIEXPORT void JNICALL Java_io_github_melvincabatuan_lbpyuv_MainActivity_process
  (JNIEnv * pEnv, jobject pClass, jobject pTarget, jbyteArray pSource){

   AndroidBitmapInfo bitmapInfo;
   uint32_t* bitmapContent;

   if(AndroidBitmap_getInfo(pEnv, pTarget, &bitmapInfo) < 0) abort();
   if(bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) abort();
   if(AndroidBitmap_lockPixels(pEnv, pTarget, (void**)&bitmapContent) < 0) abort();

   /// Access source array data... OK
   jbyte* source = (jbyte*)pEnv->GetPrimitiveArrayCritical(pSource, 0);
   if (source == NULL) abort();

   ///  YUV420sp source --->  cv::Mat
    cv::Mat src(bitmapInfo.height+ bitmapInfo.height/2, bitmapInfo.width, CV_8UC1, (unsigned char *)source);
  
   ///  BGRA --->  cv::Mat
    cv::Mat mbgra(bitmapInfo.height, bitmapInfo.width, CV_8UC4, (unsigned char *)bitmapContent);

/***************************************************************************************************/

   if (imageV.empty())
       imageV = cv::Mat(bitmapInfo.height/2, bitmapInfo.width/2, CV_8UC1);

   if (imageU.empty())
       imageU = cv::Mat(bitmapInfo.height/2, bitmapInfo.width/2, CV_8UC1);
              
   extractVU( src(cv::Range(bitmapInfo.height, bitmapInfo.height + bitmapInfo.height/2), cv::Range(0, bitmapInfo.width)), imageV, imageU);
   
 
   // Merge channels 
   channels[0] = src.rowRange(0, bitmapInfo.height);                                    // Y
   cv::resize( imageU,  channels[1], mbgra.size(), cv::INTER_LINEAR );                  // U
   cv::resize( imageV,  channels[2], mbgra.size(), cv::INTER_LINEAR );                  // V
   channels[3] = getLBP(src.rowRange(0, bitmapInfo.height));
 
   cv::merge(channels, mbgra); 
 

    /// YUV420spNV21 gray ---> BGRA // Sanity Check OKAY! July 29
    //cv::cvtColor(src(cv::Range(0, bitmapInfo.height), cv::Range(0, bitmapInfo.width)), mbgra, CV_GRAY2BGRA);

/***************************************************************************************************/

    /// Release Java byte buffer and unlock backing bitmap
    pEnv-> ReleasePrimitiveArrayCritical(pSource,source,0);
   if (AndroidBitmap_unlockPixels(pEnv, pTarget) < 0) abort();
}
