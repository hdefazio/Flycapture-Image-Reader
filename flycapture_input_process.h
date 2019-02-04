#ifndef _KWIVER_FLYCAPTURE_INPUT_PROCESS_H_
#define _KWIVER_FLYCAPTURE_INPUT_PROCESS_H_

#include <sprokit/pipeline/process.h>
#include "kwiver_processes_export.h"
#include "flycapture/FlyCapture2.h"
using namespace FlyCapture2;

namespace kwiver {

// ----------------------------------------------------------------
/**
 * \class flycapture_input_process
 *
 * \brief Reads a series of images
 *
 * \oports
 * \oport{image}
 *
 * \oport{frame}
 * \oport{time}
 */
class KWIVER_PROCESSES_NO_EXPORT flycapture_input_process
  : public sprokit::process
{
public:
  flycapture_input_process( kwiver::vital::config_block_sptr const& config );
  virtual ~flycapture_input_process();


protected:
  virtual void _configure();
  virtual void _init();
  virtual void _step();



private:
  void make_ports();
  void make_config();


  class priv;
  const std::unique_ptr<priv> d;
}; // end class flycapture_input_process

}  // end namespace

#endif /* _KWIVER_FLYCAPTURE_INPUT_PROCESS_H_ */
