#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

#include <filesystem>
#include <fstream>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
using namespace std;


struct Joint{
    //0 - 5 representing the 6 joints
    int id;

    //Current position of joint in rad
    float position;

    //Current velocity of joint in rad/s
    float velocity;

    //Current torque exerted in Nm
    float torque;


};

struct Timestampable{
     //uint64_t instead of unsigned long to keep consistency across
    //systems
    uint64_t timestamp;
};

struct ArmState: Timestampable{
    Joint jointArr[6];
};

enum class CameraAngle{
    ceiling,
    head_left,
    head_right,
    wrist_left,
    wrist_right

};

struct CameraImage: Timestampable{
    CameraAngle perspective;
    string imageLink;
};

class OpenArmSim{
    private:
        vector<ArmState> states;
    
    public:

        void simRecordData(){
            int dt = 2; //ms
            for (int i = 0; i < 25; i++){
                ArmState s;
                for (int j = 0; j < 6; j++) {
                    s.jointArr[j] = {
                        j,
                        0.0f,
                        0.0f,
                        0.0f
                    };
                }

                s.timestamp = i * dt;
                recordState(s);
            }
        }



        void recordState(ArmState state){
            states.push_back(state);
        }

        vector<ArmState> getStates(){
            return states;
        }
};





class CameraSim {
    private:
        vector<CameraImage> images;
    public:


        void simRecordData(CameraAngle angle){
            int dt = 5; //ms

            for (int i = 0; i < 10; i++){
                CameraImage image;
                image.timestamp = i * dt;
                image.perspective = angle;
                //In data.txt image name is ms
                image.imageLink =to_string(i * dt) + ".jpeg";
                takeImage(image);  
            }
        }
        
        void takeImage(CameraImage image){
            images.push_back(image);
        }

        vector<CameraImage> getImages(){
            return images;
        }

};

struct Cameras{
    CameraImage images[5];
};




template <typename T>
vector<T> nearestNeighbor(
    uint64_t startTime,
    uint64_t dt,
    const vector<T> vect)
{
    vector<T> result;

    // Advance i until vect[i] is the right bracket of targetTime
    auto advanceTo = [&](size_t& i, uint64_t targetTime) {
        while (i < vect.size() && vect[i].timestamp < targetTime) {
            i++;
        }
    };

    if (startTime < vect.front().timestamp) {
        cerr << "[nearestNeighbor] Warning: startTime (" << startTime
             << ") is before first vector timestamp ("
             << vect.front().timestamp
             << "). Results may be incorrect.\n";
    }

    if (vect.size() == 1){
        return vect;
    }

    size_t i = 1;
    for (uint64_t targetTime = startTime; i < vect.size(); targetTime += dt) {
        advanceTo(i, targetTime);
        if (i >= vect.size()) break;

        // Pick whichever neighbor (left or right) is closer
        bool preferLeft = (targetTime - vect[i - 1].timestamp)
                        <= (vect[i].timestamp - targetTime);
        result.push_back(preferLeft ? vect[i - 1] : vect[i]);
    }

    return result;
}


// std::filesystem::create_directory("data");          // single folder
//     std::filesystem::create_directories("a/b/c");       // nested folders
//g++ -std=c++17 main.cpp

// std::ofstream file("data/output.txt");

//     file << "Hello world\n";
//     file << "Robot data: " << 42 << "\n";

//     file.close();


// std::ofstream file("qpos.csv");
// file << "t,q0,q1\n";
// file << "1,0.1,1.0\n";
class DataStorage{
    public:

        static void appendEpisode
        (
            vector<Cameras> cams,
            vector<ArmState> states
        ){  
            if (states.size() != cams.size()){
                throw invalid_argument("Input vectors must have equal size");
            }

            if(!filesystem::is_directory("episodes")){
                filesystem::create_directories("episodes");
            }

            

            auto now = chrono::system_clock::now();
            std::time_t t = chrono::system_clock::to_time_t(now);

            std::tm tm = *std::localtime(&t);

            std::ostringstream oss;
            oss << "episode_"
                << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");

            string episodeDir = "episodes/" + oss.str();
            filesystem::create_directories(episodeDir);
            filesystem::create_directories(episodeDir + "/action");
            filesystem::create_directories(episodeDir + "/obs");


            filesystem::create_directories(episodeDir + "/cameras");
            filesystem::create_directories(episodeDir + "/cameras/ceiling");
            filesystem::create_directories(episodeDir + "/cameras/head_left");
            filesystem::create_directories(episodeDir + "/cameras/head_right");
            filesystem::create_directories(episodeDir + "/cameras/wrist_left");
            filesystem::create_directories(episodeDir + "/cameras/wrist_right");

            for(const Cameras& c: cams){
                for (int i =  0; i < 5; i++){

                    string folderName;

                    switch(c.images[i].perspective){
                        case CameraAngle::ceiling:
                            folderName = "ceiling/";
                            break;

                        case CameraAngle::head_left:
                            folderName = "head_left/";
                            break;

                        case CameraAngle::head_right:
                            folderName = "head_right/";
                            break;

                        case CameraAngle::wrist_left:
                            folderName = "wrist_leftt/";
                            break;

                        case CameraAngle::wrist_right:
                            folderName = "wrist_right/";
                            break;
                        
                    }

                    ofstream file(episodeDir + "/cameras/"+ folderName + c.images[i].imageLink); 
                    file.close(); 

                }
                
            }

            
            filesystem::create_directories(episodeDir + "/action");
            filesystem::create_directories(episodeDir + "/obs");

            filesystem::create_directories(episodeDir + "/action/left");
            filesystem::create_directories(episodeDir + "/obs/left");
            filesystem::create_directories(episodeDir + "/action/right");
            filesystem::create_directories(episodeDir + "/obs/right");

            //.cvs file cpp equivalent to parquet
            ofstream fileActionLeft(episodeDir + "/action/left/" + "qpos.csv");
            ofstream fileActionRight(episodeDir + "/action/right/" + "qpos.csv");
            ofstream fileObsLeft(episodeDir + "/obs/left/" + "qpos.csv");
            ofstream fileObsRight(episodeDir + "/obs/right/" + "qpos.csv");

            fileActionLeft << "timestamp,posj0,velj0,torqj0,posj1,velj1,torqj1,posj2,velj2,torqj2\n";
            fileActionRight << "timestamp,posj3,velj3,torqj3,posj4,velj4,torqj4,posj5,velj5,torqj5\n";
            fileObsLeft << "timestamp,posj0,velj0,torqj0,posj1,velj1,torqj1,posj2,velj2,torqj2\n";
            fileObsRight <<  "timestamp,posj3,velj3,torqj3,posj4,velj4,torqj4,posj5,velj5,torqj5\n";


            for (const ArmState& s: states){
                fileActionLeft << s.timestamp<<",";
                fileObsLeft << s.timestamp<<",";
                
                for(int i = 0; i < 2; i++){
                    fileActionLeft  <<s.jointArr[i].position<<"," <<s.jointArr[i].velocity<<"," <<s.jointArr[i].torque<<",";
                    fileObsLeft  <<s.jointArr[i].position<<"," <<s.jointArr[i].velocity<<"," <<s.jointArr[i].torque<<",";
                }
                fileActionLeft <<s.jointArr[2].position<<"," <<s.jointArr[2].velocity<<"," <<s.jointArr[2].torque<<endl;
                fileObsLeft <<s.jointArr[2].position<<"," <<s.jointArr[2].velocity<<"," <<s.jointArr[2].torque<<endl;
                

                fileActionRight << s.timestamp<<",";
                fileObsRight << s.timestamp<<",";
                for(int i = 3; i < 5; i++){
                    fileActionRight  <<s.jointArr[i].position<<"," <<s.jointArr[i].velocity<<"," <<s.jointArr[i].torque<<",";
                    fileObsRight <<s.jointArr[i].position<<"," <<s.jointArr[i].velocity<<"," <<s.jointArr[i].torque<<",";
                }
                fileActionRight <<s.jointArr[5].position<<"," <<s.jointArr[5].velocity<<"," <<s.jointArr[5].torque<<endl;
                fileObsRight <<s.jointArr[5].position<<"," <<s.jointArr[5].velocity<<"," <<s.jointArr[5].torque<<endl;
            }

            fileActionLeft.close();
            fileActionRight.close();
            fileObsLeft.close();
            fileObsRight.close();


            std::ofstream fileMeta(episodeDir + "/metadata.json");
            fileMeta << "{\n"
                << "  \"episodeID\": \"" << oss.str() << "\"\n"
                << "}\n";
            fileMeta.close();
            

        }
    
};



int main(){
    cout << "Starting Program"<< endl;

    OpenArmSim joints;
    joints.simRecordData();
    vector<ArmState> stateData= joints.getStates();

    for(int stateInd; stateInd < stateData.size(); stateInd++){
        
        
        ArmState state = stateData[stateInd];
        cout << "State: " << stateInd << " @ " << state.timestamp << " ms"<< endl;

        for(int i = 0; i < 6; i++){
            Joint j = state.jointArr[i];
            cout << "Joint " << j.id << " ----------- ";
            cout << "Position: " << j.position << " rad ";
            cout << "Velocity: " << j.velocity << " rad/s ";
            cout << "Torque:   " << j.torque << " Nm" << endl;
            
        }
        cout << endl;
        
    }





    //Task 3

    /*
    This is a bunch of camera classes that will return a vector
    of timestamped images that represent typical data.
    */
    
    CameraSim ceilingCam;
    ceilingCam.simRecordData(CameraAngle::ceiling);

    CameraSim head_leftCam;
    head_leftCam.simRecordData(CameraAngle::head_left);

    CameraSim head_rightCam;
    head_rightCam.simRecordData(CameraAngle::head_right);

    CameraSim wrist_leftCam;
    wrist_leftCam.simRecordData(CameraAngle::wrist_left);

    CameraSim wrist_rightCam;
    wrist_rightCam.simRecordData(CameraAngle::wrist_right);


    /*
    Dealing with multiple camera or datasets with differnt 
    frequencies/frame rages it done by choosing a timestep and 
    using nearest neighbor or interpolation (will use nearest_neighbor)
    */
    vector<Cameras> dataPairs;
    vector<CameraImage> ceilingData = ceilingCam.getImages();
    vector<CameraImage> head_leftData = head_leftCam.getImages();
    vector<CameraImage> head_rightData = head_rightCam.getImages();
    vector<CameraImage> wrist_leftData = wrist_leftCam.getImages();
    vector<CameraImage> wrist_rightData = wrist_rightCam.getImages();


    //Timestep chosen based on lowest framerate this is not only solution
    uint64_t timestep = 5;
    ceilingData = nearestNeighbor<CameraImage>(0 , timestep , ceilingData);
    head_leftData = nearestNeighbor<CameraImage>(0 , timestep , head_leftData);
    head_rightData = nearestNeighbor<CameraImage>(0 , timestep , head_rightData);
    wrist_leftData = nearestNeighbor<CameraImage>(0 , timestep , wrist_leftData);
    wrist_rightData = nearestNeighbor<CameraImage>(0 , timestep , wrist_rightData);

    stateData = nearestNeighbor<ArmState>(0 , timestep , stateData);
    
    size_t minSize = min({
        ceilingData.size(),
        head_leftData.size(),
        head_rightData.size(),
        wrist_leftData.size(),
        wrist_rightData.size(),

        stateData.size()
    });

    ceilingData.resize(minSize);
    head_leftData.resize(minSize);
    head_rightData.resize(minSize);
    wrist_leftData.resize(minSize);
    wrist_rightData.resize(minSize);

    stateData.resize(minSize);

    vector<Cameras> cams;
    for (int i = 0; i < minSize; i++){
        Cameras c;
        c.images[0] = ceilingData[i];
        c.images[1] = head_leftData[i];
        c.images[2] = head_rightData[i];
        c.images[3] = wrist_leftData[i];
        c.images[4] = wrist_rightData[i];

        cams.push_back(c);

    }

    /*
    Cams Now represents the camera images bundles by timestep.
    */




    /*
    Task 4

    The stored data will be in the format list
    */

    DataStorage::appendEpisode(cams, stateData);

    cout<< "Completed" <<endl;


}