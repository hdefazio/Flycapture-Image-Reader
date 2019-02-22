#include "flycapture_input_process.h"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
//#include <conio.h>

#include <vital/vital_types.h>
#include <vital/types/timestamp.h>
//#include <vital/types/image_container.h>
#include <arrows/ocv/image_container.h>
#include <vital/types/image.h>
#include <vital/algo/image_io.h>
#include <vital/exceptions.h>
#include <vital/util/data_stream_reader.h>
#include <vital/util/tokenize.h>
#include <opencv2/core/core.hpp>

#include <kwiver_type_traits.h>

#include <sprokit/pipeline/process_exception.h>
#include <sprokit/pipeline/datum.h>

#include <kwiversys/SystemTools.hxx>

#include <vector>
#include <stdint.h>
#include <fstream>
#include <iostream>


#include <opencv2/highgui/highgui.hpp>
using namespace cv;



// -- DEBUG
#if defined DEBUG


#endif

//+ TODO this process is obsoleted by the image_list_reader
// implementation of the video_input algorithm

namespace algo = kwiver::vital::algo;

namespace kwiver {


// (config-key, value-type, default-value, description )


create_config_trait( camera_serial, int, "15444951", "serial number of the camera" );

create_config_trait( frame_time, double, "3.0", "inter frame time in seconds");





//----------------------------------------------------------------
// Private implementation class
class flycapture_input_process::priv
{
public:
  priv();
  ~priv();

  // Configuration values
  int m_config_camera_serial;
  kwiver::vital::time_usec_t m_config_frame_time;
  

  // process local data
  kwiver::vital::frame_id_t m_frame_number;
  kwiver::vital::time_usec_t m_frame_time;



}; // end priv class


// ================================================================

flycapture_input_process
::flycapture_input_process( kwiver::vital::config_block_sptr const& config )
  : process( config ),
    d( new flycapture_input_process::priv )
{
  make_ports();
  make_config();
}


flycapture_input_process
::~flycapture_input_process()
{}


// ----------------------------------------------------------------
void flycapture_input_process
::_configure()
{
  scoped_configure_instrumentation();

  // Examine the configuration
  d->m_config_frame_time          = config_value_using_trait( frame_time ) * 1e6; // in usec
  
  kwiver::vital::config_block_sptr algo_config = get_config(); // config for process

}


// ----------------------------------------------------------------
// Post connection initialization
void flycapture_input_process
::_init()
{
  scoped_init_instrumentation();

  d->m_frame_number = 1;
}


// ----------------------------------------------------------------
void flycapture_input_process
::_step()
{

     scoped_step_instrumentation();
    
     int serial = 0;
     double frame_rate = d->m_config_frame_time/1e6; //fps
     std::cout << "frame rate: " << frame_rate << std::endl;
    
     FlyCapture2::BusManager busMgr;
     FlyCapture2::Error error;
     unsigned int numCameras;
     error = busMgr.GetNumOfCameras(&numCameras);
     if(error != PGRERROR_OK){
       error.PrintErrorTrace();
     return;
     }

     PGRGuid guid;
     if ( serial == 0 ){
      error = busMgr.GetCameraFromIndex(0, &guid);
      if(error != PGRERROR_OK){
        error.PrintErrorTrace();
      return;
      }
     }
     else{
      serial = d->m_config_camera_serial;
      error = busMgr.GetCameraFromSerialNumber(serial, &guid);
      if (error != PGRERROR_OK){
        std::stringstream serial_string;
        serial_string << serial;
        std::string msg = "PointGreyCamera::connect Could not find camera with serial number: "
          + serial_string.str() + ". Is that camera plugged in?";
        std::cerr << msg << std::endl;
        error.PrintErrorTrace();
        return;
      } //end if
     } //end else if

     Camera cam;
     error = cam.Connect(&guid);
     if(error != PGRERROR_OK){
       error.PrintErrorTrace();
     return;
     }

     CameraInfo camInfo;
     error = cam.GetCameraInfo(&camInfo);
     if(error != PGRERROR_OK){
        error.PrintErrorTrace();
     return;
     }
     //std::cout << "DEBUG -- printing camera info" << std::endl;
     
     CameraInfo* pCamInfo = &camInfo;
     std::cout << std::endl;
     std::cout << "*** CAMERA INFORMATION ***" << std::endl;
     std::cout << "Serial number -" << pCamInfo->serialNumber << std::endl;
     std::cout << "Camera model - " << pCamInfo->modelName << std::endl;
     std::cout << "Camera vendor - " << pCamInfo->vendorName << std::endl;
     std::cout << "Sensor - " << pCamInfo->sensorInfo << std::endl;
     std::cout << "Resolution - " << pCamInfo->sensorResolution << std::endl;
     std::cout << "Firmware version - " << pCamInfo->firmwareVersion << std::endl;
     std::cout << "Firmware build time - " << pCamInfo->firmwareBuildTime << std::endl << std::endl;


     Property frameRateProp(FRAME_RATE);
     error = cam.GetProperty(&frameRateProp);
     if(error != PGRERROR_OK){
       error.PrintErrorTrace();
     return;
     }
     std::cerr << "Initial framerate:" <<  frameRateProp.absValue << std::endl;
     frameRateProp.absControl = frame_rate;
     frameRateProp.absValue = frame_rate;
     frameRateProp.autoManualMode = false;
     frameRateProp.onOff = true;

     error = cam.SetProperty(&frameRateProp);
     if(error != PGRERROR_OK){
         error.PrintErrorTrace();
     return;
     }

     error = cam.GetProperty(&frameRateProp);
     if(error != PGRERROR_OK){
        error.PrintErrorTrace();
     return;
     }
     std::cerr << "Final framerate:" <<  frameRateProp.absValue << std::endl;
    
     std::cout << "DEBUG -- set RGB8" << std::endl;

     // Set RGB8
     Format7ImageSettings fmt7ImageSettings;
     fmt7ImageSettings.mode = MODE_0;
     fmt7ImageSettings.offsetX = 0;
     fmt7ImageSettings.offsetY = 0;
     fmt7ImageSettings.width = 1920;
     fmt7ImageSettings.height = 1200;
     fmt7ImageSettings.pixelFormat = PIXEL_FORMAT_RGB;

     bool isValid;
     Format7PacketInfo fmt7PacketInfo;
     //std::cout << "DEBUG -- validate settings" << std::endl;
     // Validate the settings to make sure that they are valid
     error = cam.ValidateFormat7Settings(
         &fmt7ImageSettings,
         &isValid,
         &fmt7PacketInfo );
     if(error != PGRERROR_OK){
        error.PrintErrorTrace();
     return;
     }

     if ( !isValid ){
     // Settings are not valid
         std::cout << "Format7 settings are not valid" << std::endl;
         return;
     }
     std::cout << std::endl << std::endl << "DEBUG -- set settings" << std::endl;
     // Set the settings to the camera
     error = cam.SetFormat7Configuration(
         &fmt7ImageSettings,
         fmt7PacketInfo.recommendedBytesPerPacket );
     if(error != PGRERROR_OK){
        error.PrintErrorTrace();
     return;
     }
     std::cout << "DEBUG -- capturing images" << std::endl;
     // Start capturing images
     error = cam.StartCapture();
     if(error != PGRERROR_OK){
        error.PrintErrorTrace();
     return;
     }
     std::cout << "DEBUG -- setting variables" << std::endl << std::endl;
    
     Image raw_image;
     Image rgb_image;
     

     //while(key != 113){
     int num_images = 3;
     //for(int image_count=0; image_count < num_images; ++image_count){
	std::cout << "DEBUG -- loop" << std::endl;
        raw_image = Image();
        //std::cout << "DEBUG -- retrieve buffer" << std::endl;
        error = cam.RetrieveBuffer(&raw_image);
        if(error != PGRERROR_OK){
                error.PrintErrorTrace();
                return;
        }
     
        //convert to rgb
        std::cout << "DEBUG -- convert to RGB" << std::endl;
        rgb_image = Image();
        raw_image.Convert(FlyCapture2::PIXEL_FORMAT_RGB, &rgb_image);

	//convert to cv::Mat
	std::cout << "DEBUG -- convert to Mat" << std::endl;
        unsigned int rowBytes = (double)rgb_image.GetReceivedDataSize()/(double)rgb_image.GetRows();
        cv::Mat mat_image = cv::Mat(rgb_image.GetRows(), rgb_image.GetCols(), CV_8UC3, rgb_image.GetData(), rowBytes);
        //cv::imshow("image", test_image);

	//convert to image
	 vital::image k_image = arrows::ocv::image_container::ocv_to_vital(mat_image, arrows::ocv::image_container::BGR_COLOR);

	//put into image container
        std::cout << "DEBUG -- convert to image container" << std::endl;
        std::shared_ptr<kwiver::vital::image_container> image_c = std::make_shared<arrows::ocv::image_container>(k_image);
//(mat_image, arrows::ocv::image_container::BGR_COLOR); 
        
	if(image_c != NULL){
		
        kwiver::vital::timestamp frame_ts( d->m_frame_time, d->m_frame_number );

	
        // update timestampstd::cout << "DEBUG -- update timestamp" << std::endl;
        ++d->m_frame_number;
        d->m_frame_time += d->m_config_frame_time;
	
	std::cout << "DEBUG -- push to port" << std::endl;
        push_to_port_using_trait( timestamp, frame_ts );
        push_to_port_using_trait( image, image_c );
	//std::cout << "DEBUG -- 2" << std::endl << std::endl;
	std::cout << std::endl;
}else std::cout << "Try again" << std::endl;
    // }//end loop  
          
     std::cout << "DEBUG -- out of loop" << std::endl;
     error = cam.StopCapture();
     if(error != PGRERROR_OK){
	error.PrintErrorTrace();
        return;
     }
     cam.Disconnect();
     
     LOG_DEBUG( logger(), "End of input reached, process terminating" );

     // indicate done
     mark_process_as_complete();
     const sprokit::datum_t dat= sprokit::datum::complete_datum();

     push_datum_to_port_using_trait( timestamp, dat );
     push_datum_to_port_using_trait( image, dat );
     std::cout << "DEBUG -- done" << std::endl;
}
    

// ----------------------------------------------------------------
void flycapture_input_process
::make_ports()
{
  // Set up for required ports
  sprokit::process::port_flags_t optional;

  declare_output_port_using_trait( timestamp, optional );
  declare_output_port_using_trait( image, optional );

}


// ----------------------------------------------------------------
void flycapture_input_process
::make_config()
{


  declare_config_using_trait( camera_serial );
  declare_config_using_trait( frame_time );
  

}

// ================================================================
flycapture_input_process::priv
::priv()
  : m_frame_number( 1 )
  , m_frame_time( 0 )
{}


flycapture_input_process::priv
::~priv()
{}


// ----------------------------------------------------------------



} // end namespace
