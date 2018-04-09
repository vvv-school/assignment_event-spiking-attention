#include <yarp/os/all.h>
#include <yarp/sig/Matrix.h>
#include <yarp/sig/Image.h>
#include <iCub/eventdriven/all.h>
#include <string>

using namespace ev;
using namespace yarp::os;

/******************************************************************************/
//spikingModel
/******************************************************************************/
class spikingModel : public Thread
{
private:

    //parameters
    int downscale;
    std::string module_name;
    double tau;
    double Te;
    int filter_size;
    ev::resolution res;


    //internal storage
    yarp::sig::ImageOf<yarp::sig::PixelFloat> energymap;
    yarp::sig::ImageOf<yarp::sig::PixelInt> timemap;
    yarp::sig::ImageOf<yarp::sig::PixelFloat> filter;
    unsigned int count, mean_x, mean_y;

    //ouput ports
    vGenReadPort spikes_in;
    vGenWritePort spikes_out;
    BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> > debugPort;

    //private functions
    void updateRegion(int x, int y, int ts);
    void resetRegion(int x, int y);
    double updateNeuralModel(int x, int y, int ts, double inj = 0);
    event<LabelledAE> spikeAndReset(event<AE> v);

public:

    spikingModel() : downscale(1) {}
    void setName(std::string name);
    void setResolution(unsigned int height, unsigned int width);
    void setSpikingParameters(double tau, double Te, int filterSize);


    virtual void run();
    virtual void onStop();
    virtual bool threadInit();

};

/******************************************************************************/
//spikingConfiguration
/******************************************************************************/
class spikingConfiguration : public yarp::os::RFModule
{

private:

    spikingModel spiking_model;

public:

    spikingConfiguration()  {}
    bool configure(yarp::os::ResourceFinder &rf);

    bool interruptModule();

    double getPeriod() { return 1.0; }
    bool updateModule() { return !isStopping(); }

};
