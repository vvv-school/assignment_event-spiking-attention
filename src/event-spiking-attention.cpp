#include "event-spiking-attention.h"
#include <iomanip>

/******************************************************************************/
//main
/******************************************************************************/

int main(int argc, char * argv[])
{
    /* initialize yarp network */
    Network yarp;
    if(!yarp.checkNetwork()) {
        yError() << "Could not connect to network";
        return -1;
    }

    /* prepare and configure the resource finder */
    yarp::os::ResourceFinder rf;
    rf.setDefaultConfigFile("spikingAttention.ini");
    rf.setDefaultContext("eventdriven");
    rf.configure(argc, argv);

    /* instantiate the module */
    spikingConfiguration mymodule;
    return mymodule.runModule(rf);
}

/******************************************************************************/
//spikingConfiguration
/******************************************************************************/
bool spikingConfiguration::configure(yarp::os::ResourceFinder &rf)
{
    spiking_model.setName(
                rf.check("name", Value("/vSpiking")).asString());
    spiking_model.setResolution(
                rf.check("height", Value(240)).asInt(),
                rf.check("width", Value(304)).asInt());
    spiking_model.setSpikingParameters(
                rf.check("tau", Value(3.0)).asDouble(),
                rf.check("Te", Value(2000)).asDouble(),
                rf.check("filterSize", Value(20)).asDouble());

    return spiking_model.start();
}

bool spikingConfiguration::interruptModule() {
    spiking_model.stop();
    return yarp::os::RFModule::interruptModule();
}

/******************************************************************************/
//spikingModel
/******************************************************************************/

void spikingModel::setName(std::string name)
{
    module_name = name;
}

void spikingModel::setResolution(unsigned int height, unsigned int width)
{
    res.height = height / downscale;
    res.width = width / downscale;
}

void spikingModel::setSpikingParameters(double tau, double Te, int filterSize)
{
    this->tau = tau * vtsHelper::vtsscaler;
    this->Te = Te;
    this->filter_size = filterSize / downscale;
}

bool spikingModel::threadInit()
{
    if(tau < 0) {
        yError() << "tau must be > 0";
        return false;
    }

    if(Te < 0) {
        yError() << "Te must be > 0";
        return false;
    }
    this->Te = Te;

    if(filter_size < 5) {
        yWarning() << "Filter size set to minimum (5)";
        filter_size = 5;
    }

    if(module_name.empty()) {
        yError() << "Please set module name";
        return false;
    }

    if(!res.height || !res.width) {
        yError() << "Please set the visual resolution";
        return false;
    }

    energymap.resize(res.width, res.height);
    energymap.zero();

    timemap.resize(res.width, res.height);
    timemap.zero();

    spikes_out.setWriteType(LabelledAE::tag);

    if(!spikes_in.open(module_name + "/vBottle:i"))
        return false;
    if(!spikes_out.open(module_name + "/vBottle:o"))
        return false;
    if(!debugPort.open(module_name + "/subthreshold:o"))
        return false;

    return true;
}

double spikingModel::updateNeuralModel(int x, int y, int ts, double inj)
{

    //update the subthreshold value of the neuron at position x, y
    // by updating the matrix energymap.
    // decay the neuron using the leaky-integrate-and-fire model
    // the last time this neuron is updated is stored int timemap
    // increase the energy of the neuron by "inj"
    // remember to update the timemap with the new update rate
    // remember to account for overflows in the timestamp values

    // FILL IN CODE HERE

    return 0.0;
}

void spikingModel::updateRegion(int x, int y, int ts)
{
    int x1 = std::max(x - filter_size, 0);
    int x2 = std::min(x + filter_size, res.width - 1);
    int y1 = std::max(y - filter_size, 0);
    int y2 = std::min(y + filter_size, res.height - 1);

    mean_x = 0, mean_y = 0, count = 0;

    //update the neuron models between the bounds x1 x2 y1 y2
    //if the neuron energy is above "Te" update the count and record the
    //neuron position (x,y) so you can calculate mean_x and mean_y

    //FILL IN THE CODE HERE
}

event<LabelledAE> spikingModel::spikeAndReset(event<AE> v)
{

    //create a new event<LabelledAE> based on the spiking location of mean_x
    //and mean_y. Set the event<LabelledAE>->ID to 1.
    //reset the region around mean_x and mean_y

    event<LabelledAE> spike;

    //FILL IN THE CODE HERE

    return spike;
}

void spikingModel::resetRegion(int x, int y)
{
    const static int scaleval = 2;
    int x1 = std::max(x - filter_size*scaleval, 0);
    int x2 = std::min(x + filter_size*scaleval, res.width - 1);
    int y1 = std::max(y - filter_size*scaleval, 0);
    int y2 = std::min(y + filter_size*scaleval, res.height - 1);

    //set the energymap value between x1, x2, y1, y2 to 0

    //FILL IN THE CODE HERE

}

void spikingModel::run()
{
    yarp::os::Stamp yarpstamp;
    while(!isStopping()) {

        //fill this vQueue with output spikes (if there are some)
        vQueue q_out;

        //get the next q
        const vQueue *q_in = spikes_in.read(yarpstamp);
        if(!q_in) break;

        // iterate through the q and update the region around the incoming event
        // using the function updateRegion()
        // if there was a spike (count != 0) create the spike using the function
        // spikeAndReset(). This will also reset the subthreshold energy of the
        // surrounding region. Add the spike to q_out.

        //FILL IN THE CODE HERE

        //send the spikes only once the entire spike packet has been processed.
        if(!q_out.empty())
            spikes_out.write(q_out, yarpstamp);

        //if we are visualising the subthreshold layer create the image and send it
        if(debugPort.getOutputCount()) {
            static double prevtime = Time::now();
            double dt = Time::now() - prevtime;
            if(dt > 0.03) {
                prevtime += dt;
                //decay and convert all pixels
                //int currentspiketime = q->back()->stamp;
                yarp::sig::ImageOf< yarp::sig::PixelMono > &img = debugPort.prepare();
                img.resize(energymap.width(), energymap.height());
                for(int y = 0; y < energymap.height(); y++) {
                    for(int x = 0; x < energymap.width(); x++) {
                        //updateNeuralModel(x, y, currentspiketime);
                        img(x, y) = (energymap(x, y) * 255.0) / Te;
                    }
                }

                //output the image
                debugPort.write();
            }
        }
    }


}

void spikingModel::onStop()
{
    spikes_in.close();
    debugPort.close();
    spikes_out.close();
}
