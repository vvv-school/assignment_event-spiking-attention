/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Ugo Pattacini <ugo.pattacini@iit.it>
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
*/
#include <string>

#include <yarp/rtf/TestCase.h>
#include <rtf/dll/Plugin.h>
#include <rtf/TestAssert.h>


#include <yarp/os/all.h>
#include <iCub/eventdriven/all.h>
#include <iomanip>

using namespace std;
using namespace RTF;
using namespace yarp::os;

class spikeChecker : public yarp::os::BufferedPort< ev::vBottle >
{

private:

    double gt_ts[7] = {1516812382.50, 1516812385.97, 1516812388.07,
                       1516812390.00, 1516812393.05, 1516812395.07,
                       1516812401.15};
    int gt_x[7] = {0, 145, 200, 90, 145, 210, 145};
    int gt_y[7] = {0, 110, 80, 80, 110, 110, 110};

    void onRead(ev::vBottle &input) {

        yarp::os::Stamp yarpstamp;
        this->getEnvelope(yarpstamp);

        ev::vQueue q = input.get<ev::LabelledAE>();
        for(ev::vQueue::iterator qi = q.begin(); qi != q.end(); qi++) {
            auto v = ev::is_event<ev::LabelledAE>(*qi);
            //std::cout << std::fixed << yarpstamp.getTime() <<  " - " << gt_ts[0] << " = " << gt_ts[0] - yarpstamp.getTime() << std::endl;
            if(yarpstamp.getTime() < gt_ts[0]) {
                outlier[0]++;
                continue;
            }

            for(int i = 1; i < 7; i++) {
                if(yarpstamp.getTime() < gt_ts[i]) {
                    if(std::abs(v->x - gt_x[i]) < 30 && std::abs(v->y - gt_y[i]) < 30)
                        inlier[i]++;
                    else
                        outlier[i]++;
                    break;
                }
            }
        }

    }

public:


    int inlier[7] = {0, 0, 0, 0, 0, 0, 0};
    int outlier[7] = {0, 0, 0, 0, 0, 0, 0};

    spikeChecker()
    {
        this->useCallback();
        this->setStrict();
    }

    void printSectionsScores()
    {
        for(int i = 0; i < 7; i++) {
            std::cout << inlier[i] << " " << outlier[i] << std::endl;
        }

    }

    int getScore()
    {
        int score = 0;
        for(int i = 0; i < 7; i++) {
            if(outlier[i] < 3 && inlier[i] > 1)
                score++;
        }
        if(score > 5) score = 5;
        return score;

    }


};


/**********************************************************************/
class TestAssignmentEventSpikingAttention : public yarp::rtf::TestCase
{

private:

    spikeChecker spkchk;
    yarp::os::RpcClient playercontroller;


public:
    /******************************************************************/
    TestAssignmentEventSpikingAttention() :
        yarp::rtf::TestCase("TestAssignmentEventSpikingAttention")
    {
    }

    /******************************************************************/
    virtual ~TestAssignmentEventSpikingAttention()
    {
    }

    /******************************************************************/
    virtual bool setup(yarp::os::Property& property)
    {

        //we need to load the data file into yarpdataplayer
        std::string cntlportname = "/playercontroller/rpc";

        RTF_ASSERT_ERROR_IF_FALSE(playercontroller.open(cntlportname),
                                  "Could not open RPC to yarpdataplayer");

        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect(cntlportname, "/yarpdataplayer/rpc:i"),
                                  "Could not connect RPC to yarpdataplayer");

        //we need to check the output of yarpdataplayer is open and input of spiking model
        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/zynqGrabber/vBottle:o", "/vSacSup/vBottle:i", "udp"),
                                  "Could not connect yarpdataplayer to assignment module (events)");

        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/icub/torso/state:o", "/vSacSup/torso/state:i", "udp"),
                                  "Could not connect yarpdataplayer to assignment module (torso)");

        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/icub/head/state:o", "/vSacSup/head/state:i", "udp"),
                                  "Could not connect yarpdataplayer to assignment module (head)");

        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/vSacSup/vBottle:o", "/vPreProcess/vBottle:i", "udp"),
                                  "Could not connect saccadic suppression to vPreProcess");

        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/vPreProcess/right:o", "/vSpiking/vBottle:i", "udp"),
                                  "Could not connect vPreProcess (right) to spiking module");


        //check we can open our spike checking consumer
        RTF_ASSERT_ERROR_IF_FALSE(spkchk.open("/spikechecker/vBottle:i"),
                                  "Could not open spike checker");

        //the output of spiking model
        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/vSpiking/vBottle:o", "/spikechecker/vBottle:i", "udp"),
                                  "Could not connect spiking model to spike checker");

        RTF_TEST_REPORT("Ports successfully open and connected");

        return true;
    }

    /******************************************************************/
    virtual void tearDown()
    {
        RTF_TEST_REPORT("Closing Clients");
        playercontroller.close();
    }

    /******************************************************************/
    virtual void run()
    {

        //play the dataset
        yarp::os::Bottle cmd, reply;
        cmd.addString("play");
        playercontroller.write(cmd, reply);
        RTF_ASSERT_ERROR_IF_FALSE(reply.get(0).asString() == "ok", "Did not successfully play the dataset");

        RTF_TEST_REPORT("Playing dataset - please wait till it finishes automatically");
        yarp::os::Time::delay(30);

        cmd.clear();
        cmd.addString("stop");
        playercontroller.write(cmd, reply);
        RTF_TEST_REPORT("Stopping dataset - wait for score calculation");
        RTF_ASSERT_ERROR_IF_FALSE(reply.get(0).asString() == "ok", "Did not successfully stop the dataset");

        int score = 0;
        RTF_TEST_REPORT("Inliers | Outliers");
        for(int i = 1; i < 7; i++) {
            RTF_TEST_REPORT(Asserter::format("Section %d: %d | %d", i, spkchk.inlier[i], spkchk.outlier[i]));
            if(spkchk.inlier[i] < 1) {
                RTF_TEST_REPORT("Missed detection (0)");
            } else if(spkchk.outlier[i] > 2) {
                RTF_TEST_REPORT("Too many false detections (0)");
            } else {
                RTF_TEST_REPORT("Good attention (1)");
                score++;
            }

        }
        
        RTF_TEST_REPORT(Asserter::format("Extra Bonus: %d | %d", spkchk.inlier[0], spkchk.outlier[0]));
        if(spkchk.outlier[0] < 1 && score > 0) {
            RTF_TEST_REPORT("Good (1)");
            score++;
        }

        //spkchk.printSectionsScores();
        //int score = spkchk.getScore();
        RTF_TEST_CHECK(score>0, Asserter::format("Total score = %d", score));

//        int inliers = spkchk.getInliers();
//        int outliers = spkchk.getOutliers();
//        RTF_TEST_REPORT(Asserter::format("Inliers = %d", inliers));
//        RTF_TEST_REPORT(Asserter::format("Outliers = %d", outliers));
//        RTF_ASSERT_ERROR_IF_FALSE(inliers > 3000, "Inlier score too low (3000)");
//        RTF_ASSERT_ERROR_IF_FALSE(outliers < 500, "Outlier score too high (500)");


    }
};

PREPARE_PLUGIN(TestAssignmentEventSpikingAttention)
