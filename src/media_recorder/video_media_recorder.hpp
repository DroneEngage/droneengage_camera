#ifndef CVIDEOMEDIARECORDER_H

#define CVIDEOMEDIARECORDER_H


#define MIN_CAMERA_INDEX 0
#define MAX_CAMERA_INDEX 999


namespace uavos
{
namespace media_recorder
{
namespace video
{

    class CVideoRecorder
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            static CVideoRecorder& getInstance()
            {
                static CVideoRecorder    instance; // Guaranteed to be destroyed.
                                                // Instantiated on first use.
                return instance;
            }
            CVideoRecorder(CVideoRecorder const&)           = delete;
            void operator=(CVideoRecorder const&)           = delete;

            // Note: Scott Meyers mentions in his Effective Modern
            //       C++ book, that deleted functions should generally
            //       be public as it results in better error messages
            //       due to the compilers behavior to check accessibility
            //       before deleted status

        private:

            CVideoRecorder() {}                    // Constructor? (the {} brackets) are needed here.

            // C++ 11
            // =======
            // We can use the better technique of deleting the methods we don't want.

        public:

            ~CVideoRecorder ();
            void init ();

            void processVideoRecording (const Json::Value &jMsg);
            void startImageCapturing (const Json::Value &jMsg);
            void startThread (const std::string cmd);

        protected:

            int m_StartVideoIndex = MIN_CAMERA_INDEX;
            int m_EndVideoIndex = MAX_CAMERA_INDEX;

            pthread_t m_videoRecorderThread;

            void startVideoRecording (const Json::Value &jMsg);
            void stopVideoRecording (const Json::Value &jMsg);
            
            std::string execCommand(const std::string cmd, int& out_exitStatus);        
    };
}
}
}





#endif

